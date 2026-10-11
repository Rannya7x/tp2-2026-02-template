// Arquivo de dados organizado por HASHING estatico com encadeamento de overflow.
//
// Bloco 0            : metadados (nao conta nas leituras da operacao; ver open()).
// Blocos 1..N        : buckets primarios (bucket = 1 + id % N, N primo).
// Blocos > N         : blocos de overflow, alocados no fim e encadeados.
// Cada bloco de dados: [BlockHeader{count,next}] [registros DiskRecord...]

#ifndef HASH_FILE_HPP
#define HASH_FILE_HPP

#include <cstdint>
#include <functional>
#include <string>

#include "blockio.hpp"
#include "disk_record.hpp"

class HashFile {
  public:
    // Endereco fisico de um registro (usado pelos indices da Parte 2).
    struct Rid {
        std::uint32_t block;
        std::uint16_t slot;
    };

    struct BlockHeader {
        std::uint32_t count;  // registros neste bloco
        std::uint32_t next;   // proximo bloco de overflow (0 = nenhum)
    };

    static constexpr std::size_t kRecordsPerBlock =
        (blockio::kBlockSize - sizeof(BlockHeader)) / sizeof(DiskRecord);

    // --- carga ---
    // Cria o arquivo com N buckets, N = menor primo >= esperado/(kRecordsPerBlock*target_load).
    void create(const std::string& path, std::uint64_t expected_records, double target_load = 0.8);
    void insert(const Article& a, TruncInfo* t = nullptr);
    void finish();  // grava metadados (chamar apos o ultimo insert)

    // --- consulta ---
    // Abre para leitura. Le o bloco 0 (metadados) e ZERA os contadores: as
    // leituras reportadas por cada operacao NAO incluem o bloco de metadados.
    void open(const std::string& path);
    void close() { file_.close(); }

    bool find(int id, Article& out);                       // busca por hash (le o bucket + overflow)
    bool read_at(Rid rid, Article& out);                   // 1 leitura de bloco
    using Visitor = std::function<void(const DiskRecord&, Rid)>;
    void scan(const Visitor& visit);                       // varredura sequencial: blocos 1..fim

    // --- estatisticas ---
    const blockio::Stats& stats() const { return file_.stats(); }
    void reset_stats() { file_.reset_stats(); }
    const std::string& path() const { return file_.path(); }
    std::uint64_t block_count() const { return file_.block_count(); }       // inclui bloco 0
    std::uint64_t data_blocks() const { return file_.block_count() - 1; }   // sem metadados
    std::uint64_t num_buckets() const { return num_buckets_; }
    std::uint64_t num_records() const { return num_records_; }
    // (validos apenas na execucao que fez a carga)
    std::uint64_t overflow_blocks() const { return overflow_blocks_; }
    std::uint64_t buckets_with_overflow() const { return buckets_with_overflow_; }
    std::uint64_t max_chain_blocks() const { return max_chain_blocks_; }

  private:
    struct Meta {
        std::uint32_t magic;
        std::uint32_t block_size;
        std::uint32_t record_size;
        std::uint32_t records_per_block;
        std::uint64_t num_buckets;
        std::uint64_t num_records;
    };
    static constexpr std::uint32_t kMagic = 0x48415348;  // "HASH"

    void read_into_buf(std::uint64_t b) { file_.read_block(b, buf_); }
    BlockHeader get_header() const;
    void set_header(const BlockHeader& h);
    void get_record(std::size_t slot, DiskRecord& d) const;
    void set_record(std::size_t slot, const DiskRecord& d);

    blockio::BlockFile file_;
    alignas(8) unsigned char buf_[blockio::kBlockSize];
    std::uint64_t num_buckets_ = 0;
    std::uint64_t num_records_ = 0;
    std::uint64_t overflow_blocks_ = 0;
    std::uint64_t buckets_with_overflow_ = 0;
    std::uint64_t max_chain_blocks_ = 1;
};

#endif  // HASH_FILE_HPP