// B+Tree em memoria secundaria, generica na chave (Key deve ser POD e ter
// operator<). Arquivo paginado via blockio:
//
//   bloco 0 : metadados (raiz, altura, capacidades)
//   demais  : nos. Folhas primeiro (blocos 1..L, em ordem de chave, encadeadas
//             por 'next'), depois os niveis internos; a raiz e o ultimo bloco.
//
//   Cabecalho do no (8 bytes): {is_leaf, count, next}
//   Folha   : count x [Key | Rid(block,slot)]            capacidade kLeafCap
//   Interno : keys[kIntCap] seguido de children[kIntCap+1] (uint32)
//             filho i cobre chaves k com keys[i-1] <= k < keys[i]
//
// Construcao por BULK LOAD (entradas ordenadas -> folhas -> niveis internos),
// pois o indice e construido uma vez, a partir do arquivo de dados.

#ifndef BPTREE_HPP
#define BPTREE_HPP

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

#include "blockio.hpp"
#include "hash_file.hpp"

template <class Key>
class BPlusTree {
  public:
    using Rid = HashFile::Rid;
    struct Entry {
        Key key;
        Rid rid;
    };

    static constexpr std::size_t kHeader = 8;
    static constexpr std::size_t kRidSize = 8;
    static constexpr std::size_t kLeafEntry = sizeof(Key) + kRidSize;
    // maximo de entradas por folha
    static constexpr std::size_t kLeafCap = (blockio::kBlockSize - kHeader) / kLeafEntry;
    // maximo de CHAVES por no interno (filhos = chaves + 1; "ordem" = kIntCap + 1)
    static constexpr std::size_t kIntCap = (blockio::kBlockSize - kHeader - 4) / (sizeof(Key) + 4);
    static_assert(kLeafCap >= 2 && kIntCap >= 2, "BLOCK_SIZE pequeno demais para a chave");

    // ---------- construcao ----------
    // 'entries' deve estar ORDENADO por chave.
    void build(const std::string& path, const std::vector<Entry>& entries) {
        file_.open(path, blockio::Mode::kTruncate);
        file_.alloc_block();  // bloco 0: metadados

        struct Ref {
            std::uint32_t block;
            Key min;
        };
        std::vector<Ref> level;

        const std::size_t n = entries.size();
        const std::size_t nleaves = n == 0 ? 1 : (n + kLeafCap - 1) / kLeafCap;
        for (std::size_t i = 0; i < nleaves; ++i) {
            const std::size_t from = i * kLeafCap;
            const std::size_t to = std::min(n, from + kLeafCap);
            const std::uint32_t block = static_cast<std::uint32_t>(file_.alloc_block());
            std::memset(buf_, 0, sizeof(buf_));
            put_header(make_header(true, to - from, i + 1 < nleaves ? block + 1 : 0));
            for (std::size_t j = from; j < to; ++j) {
                put_leaf_key(j - from, entries[j].key);
                put_leaf_rid(j - from, entries[j].rid);
            }
            file_.write_block(block, buf_);
            Key mk;
            std::memset(&mk, 0, sizeof(mk));
            if (n > 0) mk = entries[from].key;
            level.push_back({block, mk});
        }
        height_ = 1;
        num_leaves_ = nleaves;
        num_entries_ = n;

        while (level.size() > 1) {
            std::vector<Ref> up;
            std::size_t i = 0;
            while (i < level.size()) {
                std::size_t take = std::min(kIntCap + 1, level.size() - i);
                if (level.size() - i - take == 1) --take;  // nao deixar no com 1 filho so
                const std::uint32_t block = static_cast<std::uint32_t>(file_.alloc_block());
                std::memset(buf_, 0, sizeof(buf_));
                put_header(make_header(false, take - 1, 0));
                for (std::size_t j = 0; j < take; ++j) {
                    put_child(j, level[i + j].block);
                    if (j > 0) put_int_key(j - 1, level[i + j].min);
                }
                file_.write_block(block, buf_);
                up.push_back({block, level[i].min});
                i += take;
            }
            level.swap(up);
            ++height_;
        }
        root_ = level[0].block;

        std::memset(buf_, 0, sizeof(buf_));
        Meta m{kMagic, static_cast<std::uint32_t>(blockio::kBlockSize),
               static_cast<std::uint32_t>(sizeof(Key)), root_, height_,
               static_cast<std::uint32_t>(kLeafCap), static_cast<std::uint32_t>(kIntCap),
               0, num_entries_, num_leaves_};
        std::memcpy(buf_, &m, sizeof(m));
        file_.write_block(0, buf_);
    }

    // ---------- consulta ----------
    // Le os metadados (bloco 0) e ZERA os contadores: as leituras reportadas por
    // cada operacao nao incluem o bloco de metadados.
    void open(const std::string& path) {
        file_.open(path, blockio::Mode::kRead);
        file_.read_block(0, buf_);
        Meta m;
        std::memcpy(&m, buf_, sizeof(m));
        if (m.magic != kMagic) throw std::runtime_error("indice invalido: " + path);
        if (m.block_size != blockio::kBlockSize || m.key_size != sizeof(Key))
            throw std::runtime_error("indice gerado com outro BLOCK_SIZE/chave; refaca o upload");
        root_ = m.root;
        height_ = m.height;
        num_entries_ = m.num_entries;
        num_leaves_ = m.num_leaves;
        file_.reset_stats();
    }
    void close() { file_.close(); }

    // Busca exata: desce da raiz ate a folha (custo = altura, em blocos).
    bool find_exact(const Key& key, Rid& out) {
        descend(key);
        const Header h = get_header();
        for (std::size_t i = 0; i < h.count; ++i) {
            Key k = get_leaf_key(i);
            if (!(k < key) && !(key < k)) {
                out = get_leaf_rid(i);
                return true;
            }
        }
        return false;
    }

    // Varredura em ordem crescente a partir da primeira chave >= 'key',
    // percorrendo as folhas encadeadas. 'visit' devolve false para parar.
    void scan_from(const Key& key, const std::function<bool(const Key&, Rid)>& visit) {
        descend(key);
        Header h = get_header();
        std::size_t i = 0;
        while (i < h.count && get_leaf_key(i) < key) ++i;
        for (;;) {
            for (; i < h.count; ++i)
                if (!visit(get_leaf_key(i), get_leaf_rid(i))) return;
            if (h.next == 0) return;
            file_.read_block(h.next, buf_);
            h = get_header();
            i = 0;
        }
    }

    // ---------- estatisticas ----------
    const blockio::Stats& stats() const { return file_.stats(); }
    void reset_stats() { file_.reset_stats(); }
    std::uint64_t block_count() const { return file_.block_count(); }  // inclui bloco 0
    std::uint32_t height() const { return height_; }                   // niveis, folha inclusa
    std::uint64_t num_entries() const { return num_entries_; }
    std::uint64_t num_leaves() const { return num_leaves_; }

  private:
    struct Header {
        std::uint8_t is_leaf;
        std::uint8_t pad;
        std::uint16_t count;
        std::uint32_t next;
    };
    struct Meta {
        std::uint32_t magic, block_size, key_size, root;
        std::uint32_t height, leaf_cap, int_cap, reserved;
        std::uint64_t num_entries, num_leaves;
    };
    static constexpr std::uint32_t kMagic = 0x42505453;  // "BPTS"

    // Desce da raiz ate a folha que pode conter 'key'; deixa a folha em buf_.
    void descend(const Key& key) {
        std::uint32_t b = root_;
        for (std::uint32_t lvl = height_; lvl > 1; --lvl) {
            file_.read_block(b, buf_);
            const Header h = get_header();
            std::size_t i = 0;
            while (i < h.count && !(key < get_int_key(i))) ++i;
            b = get_child(i);
        }
        file_.read_block(b, buf_);
    }

    static Header make_header(bool leaf, std::size_t count, std::uint32_t next) {
        Header h;
        h.is_leaf = leaf ? 1 : 0;
        h.pad = 0;
        h.count = static_cast<std::uint16_t>(count);
        h.next = next;
        return h;
    }

    // --- acesso aos campos do bloco em buf_ ---
    Header get_header() const {
        Header h;
        std::memcpy(&h, buf_, sizeof(h));
        return h;
    }
    void put_header(const Header& h) { std::memcpy(buf_, &h, sizeof(h)); }

    Key get_leaf_key(std::size_t i) const {
        Key k;
        std::memcpy(&k, buf_ + kHeader + i * kLeafEntry, sizeof(Key));
        return k;
    }
    void put_leaf_key(std::size_t i, const Key& k) {
        std::memcpy(buf_ + kHeader + i * kLeafEntry, &k, sizeof(Key));
    }
    Rid get_leaf_rid(std::size_t i) const {
        std::uint32_t v[2];
        std::memcpy(v, buf_ + kHeader + i * kLeafEntry + sizeof(Key), kRidSize);
        return Rid{v[0], static_cast<std::uint16_t>(v[1])};
    }
    void put_leaf_rid(std::size_t i, const Rid& r) {
        std::uint32_t v[2] = {r.block, r.slot};
        std::memcpy(buf_ + kHeader + i * kLeafEntry + sizeof(Key), v, kRidSize);
    }

    static constexpr std::size_t kChildrenOff = kHeader + kIntCap * sizeof(Key);
    Key get_int_key(std::size_t i) const {
        Key k;
        std::memcpy(&k, buf_ + kHeader + i * sizeof(Key), sizeof(Key));
        return k;
    }
    void put_int_key(std::size_t i, const Key& k) {
        std::memcpy(buf_ + kHeader + i * sizeof(Key), &k, sizeof(Key));
    }
    std::uint32_t get_child(std::size_t i) const {
        std::uint32_t c;
        std::memcpy(&c, buf_ + kChildrenOff + i * 4, 4);
        return c;
    }
    void put_child(std::size_t i, std::uint32_t c) { std::memcpy(buf_ + kChildrenOff + i * 4, &c, 4); }

    blockio::BlockFile file_;
    alignas(8) unsigned char buf_[blockio::kBlockSize];
    std::uint32_t root_ = 0;
    std::uint32_t height_ = 0;
    std::uint64_t num_entries_ = 0;
    std::uint64_t num_leaves_ = 0;
};

#endif  // BPTREE_HPP