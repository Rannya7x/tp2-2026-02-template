// findtitle <Titulo> — busca por titulo percorrendo sequencialmente o arquivo de dados (Parte 1)
//
// Deve informar, ao final, a quantidade de blocos lidos na operação e os
// totais de blocos dos arquivos envolvidos. Todo acesso a disco deve passar
// pela camada blockio.

#include <cstdio>
#include <string>
#include <vector>

#include "blockio.hpp"
#include "hash_file.hpp"
#include "paths.hpp"
#include "print_utils.hpp"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "uso: %s \"<Titulo>\"\n", argv[0]);
        return 2;
    }
    const std::string titulo = argv[1];
    using ull = unsigned long long;
 
    try {
        HashFile hf;
        hf.open(kDataPath);
 
        std::vector<Article> found;
        hf.scan([&](const DiskRecord& d, HashFile::Rid) {
            if (titulo_equals(d, titulo)) {
                Article a;
                from_disk(d, a);
                found.push_back(a);
            }
        });
 
        std::printf("== findtitle \"%s\" ==\n", titulo.c_str());
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
