// upload <arquivo.csv> — carga inicial dos dados (Parte 1 do enunciado).
//
// Deve criar o arquivo de dados organizado por hashing e informar ao final:
// registros carregados, blocos do arquivo de dados, registros por bloco no
// layout adotado e taxa de ocupação dos blocos.
//
// O trecho abaixo existe apenas para mostrar o uso das duas peças fornecidas
// (CsvReader e blockio::BlockFile). Substitua-o pela sua implementação.

#include <cstdio>
#include <string>
#include <chrono>
#include <filesystem>

#include "blockio.hpp"
#include "csv_reader.hpp"
#include "hash_file.hpp"
#include "paths.hpp"
#include "index_builder.hpp"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "uso: %s <arquivo.csv>\n", argv[0]);
        return 2;
    }
    const std::string csv_path = argv[1];
    using ull = unsigned long long;
 
    try {
        auto t0 = std::chrono::steady_clock::now();
 
        // passada 1: contar registros validos (para dimensionar N buckets)
        std::uint64_t n = 0, skipped = 0;
        {
            CsvReader r(csv_path);
            Article a;
            while (r.next(a)) ++n;
            skipped = r.skipped();
        }
 
        std::filesystem::create_directories(kDbDir);
        HashFile hf;
        hf.create(kDataPath, n);
 
        // passada 2: inserir
        std::uint64_t tr_t = 0, tr_a = 0, tr_s = 0;
        {
            CsvReader r(csv_path);
            Article a;
            while (r.next(a)) {
                TruncInfo t;
                hf.insert(a, &t);
                tr_t += t.titulo; tr_a += t.autores; tr_s += t.snippet;
            }
        }
        hf.finish();
 
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now() - t0).count();
        const std::uint64_t db = hf.data_blocks();
        const double occ = 100.0 * hf.num_records() / (static_cast<double>(db) * HashFile::kRecordsPerBlock);
 
        std::printf("== upload ==\n");
        std::printf("arquivo de entrada      : %s\n", csv_path.c_str());
        std::printf("arquivo de dados        : %s\n", kDataPath);
        std::printf("linhas ignoradas (CSV)  : %llu\n", (ull)skipped);
        std::printf("registros carregados    : %llu\n", (ull)hf.num_records());
        std::printf("tamanho do bloco        : %zu bytes\n", blockio::kBlockSize);
        std::printf("tamanho do registro     : %zu bytes\n", sizeof(DiskRecord));
        std::printf("registros por bloco     : %zu\n", HashFile::kRecordsPerBlock);
        std::printf("buckets (N, primo)      : %llu\n", (ull)hf.num_buckets());
        std::printf("blocos de overflow      : %llu\n", (ull)hf.overflow_blocks());
        std::printf("buckets com overflow    : %llu\n", (ull)hf.buckets_with_overflow());
        std::printf("maior cadeia (blocos)   : %llu\n", (ull)hf.max_chain_blocks());
        std::printf("blocos do arq. de dados : %llu (+1 de metadados)\n", (ull)db);
        std::printf("taxa de ocupacao        : %.2f%%\n", occ);
        std::printf("campos truncados        : titulo=%llu autores=%llu snippet=%llu\n",
                    (ull)tr_t, (ull)tr_a, (ull)tr_s);
        std::printf("blocos escritos (carga) : %llu\n", (ull)hf.stats().writes);
        std::printf("tempo                   : %lld ms\n", (long long)ms);
 
        hf.close();
        build_all_indexes(kDataPath);  // indices da Parte 2
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "erro: %s\n", e.what());
        return 1;
    }
}