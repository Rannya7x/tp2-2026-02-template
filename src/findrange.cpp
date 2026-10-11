// findrange <ID-inicial> <ID-final> — busca por intervalo de IDs percorrendo sequencialmente o arquivo de dados (Parte 1)
//
// Deve informar, ao final, a quantidade de blocos lidos na operação e os
// totais de blocos dos arquivos envolvidos. Todo acesso a disco deve passar
// pela camada blockio.

#include <cstdio>
#include <cstdlib>
#include <vector>

#include "blockio.hpp"
#include "hash_file.hpp"
#include "paths.hpp"
#include "print_utils.hpp"

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "uso: %s <ID-inicial> <ID-final>\n", argv[0]);
        return 2;
    }
    const int lo = std::atoi(argv[1]);
    const int hi = std::atoi(argv[2]);
    using ull = unsigned long long;
    if (lo > hi) {
        std::fprintf(stderr, "erro: ID-inicial (%d) maior que ID-final (%d)\n", lo, hi);
        return 2;
    }
 
    try {
        HashFile hf;
        hf.open(kDataPath);
 
        std::vector<Article> found;
        hf.scan([&](const DiskRecord& d, HashFile::Rid) {
            if (d.id >= lo && d.id <= hi) {
                Article a;
                from_disk(d, a);
                found.push_back(a);
            }
        });
 
        std::printf("== findrange %d %d ==\n", lo, hi);
        std::printf("arquivo de dados : %s\n", kDataPath);
        std::printf("registros encontrados: %zu\n", found.size());
        print_results(found);
        std::printf("blocos lidos     : %llu\n", (ull)hf.stats().reads);
        std::printf("total de blocos  : %llu (arquivo de dados, incl. 1 de metadados)\n",
                    (ull)hf.block_count());
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "erro: %s\n", e.what());
        return 1;
    }
}