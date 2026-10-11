#include "hash_file.hpp"

#include <cstring>
#include <stdexcept>

static bool is_prime(std::uint64_t n) {
    if (n < 2) return false;
    for (std::uint64_t d = 2; d * d <= n; ++d)
        if (n % d == 0) return false;
    return true;
}

static std::uint64_t next_prime(std::uint64_t n) {
    while (!is_prime(n)) ++n;
    return n;
}

HashFile::BlockHeader HashFile::get_header() const {
    BlockHeader h;
    std::memcpy(&h, buf_, sizeof(h));
    return h;
}
void HashFile::set_header(const BlockHeader& h) { std::memcpy(buf_, &h, sizeof(h)); }

void HashFile::get_record(std::size_t slot, DiskRecord& d) const {
    std::memcpy(&d, buf_ + sizeof(BlockHeader) + slot * sizeof(DiskRecord), sizeof(d));
}
void HashFile::set_record(std::size_t slot, const DiskRecord& d) {
    std::memcpy(buf_ + sizeof(BlockHeader) + slot * sizeof(DiskRecord), &d, sizeof(d));
}

void HashFile::create(const std::string& path, std::uint64_t expected_records, double target_load) {
    file_.open(path, blockio::Mode::kTruncate);
    double buckets = static_cast<double>(expected_records) / (kRecordsPerBlock * target_load);
    num_buckets_ = next_prime(static_cast<std::uint64_t>(buckets) + 1);
    num_records_ = 0;
    file_.alloc_block();  // bloco 0: metadados
    for (std::uint64_t i = 0; i < num_buckets_; ++i) file_.alloc_block();
}

void HashFile::insert(const Article& a, TruncInfo* t) {
    DiskRecord d;
    to_disk(a, d, t);

    std::uint64_t b = 1 + static_cast<std::uint64_t>(static_cast<std::uint32_t>(a.id)) % num_buckets_;
    std::uint64_t depth = 0;  // numero de overflows ja percorridos
    for (;;) {
        read_into_buf(b);
        BlockHeader h = get_header();
        if (h.count < kRecordsPerBlock) {
            set_record(h.count, d);
            ++h.count;
            set_header(h);
            file_.write_block(b, buf_);
            break;
        }
        if (h.next != 0) {
            b = h.next;
            ++depth;
            continue;
        }
        // cadeia cheia: novo bloco de overflow no fim do arquivo
        std::uint64_t nb = file_.alloc_block();
        h.next = static_cast<std::uint32_t>(nb);
        set_header(h);
        file_.write_block(b, buf_);

        std::memset(buf_, 0, sizeof(buf_));
        BlockHeader nh{1, 0};
        set_header(nh);
        set_record(0, d);
        file_.write_block(nb, buf_);

        ++overflow_blocks_;
        if (depth == 0) ++buckets_with_overflow_;
        if (depth + 2 > max_chain_blocks_) max_chain_blocks_ = depth + 2;
        break;
    }
    ++num_records_;
}

void HashFile::finish() {
    std::memset(buf_, 0, sizeof(buf_));
    Meta m{kMagic, static_cast<std::uint32_t>(blockio::kBlockSize),
           static_cast<std::uint32_t>(sizeof(DiskRecord)),
           static_cast<std::uint32_t>(kRecordsPerBlock), num_buckets_, num_records_};
    std::memcpy(buf_, &m, sizeof(m));
    file_.write_block(0, buf_);
}

void HashFile::open(const std::string& path) {
    file_.open(path, blockio::Mode::kRead);
    read_into_buf(0);
    Meta m;
    std::memcpy(&m, buf_, sizeof(m));
    if (m.magic != kMagic) throw std::runtime_error("arquivo de dados invalido: " + path);
    if (m.block_size != blockio::kBlockSize || m.record_size != sizeof(DiskRecord))
        throw std::runtime_error("arquivo gerado com outro BLOCK_SIZE/layout; refaca o upload");
    num_buckets_ = m.num_buckets;
    num_records_ = m.num_records;
    file_.reset_stats();
}

bool HashFile::find(int id, Article& out) {
    std::uint64_t b = 1 + static_cast<std::uint64_t>(static_cast<std::uint32_t>(id)) % num_buckets_;
    while (b != 0) {
        read_into_buf(b);
        BlockHeader h = get_header();
        for (std::size_t s = 0; s < h.count; ++s) {
            DiskRecord d;
            get_record(s, d);
            if (d.id == id) {
                from_disk(d, out);
                return true;
            }
        }
        b = h.next;
    }
    return false;
}

bool HashFile::read_at(Rid rid, Article& out) {
    read_into_buf(rid.block);
    BlockHeader h = get_header();
    if (rid.slot >= h.count) return false;
    DiskRecord d;
    get_record(rid.slot, d);
    from_disk(d, out);
    return true;
}

void HashFile::scan(const Visitor& visit) {
    std::uint64_t total = file_.block_count();
    for (std::uint64_t b = 1; b < total; ++b) {
        read_into_buf(b);
        BlockHeader h = get_header();
        for (std::size_t s = 0; s < h.count; ++s) {
            DiskRecord d;
            get_record(s, d);
            visit(d, Rid{static_cast<std::uint32_t>(b), static_cast<std::uint16_t>(s)});
        }
    }
}