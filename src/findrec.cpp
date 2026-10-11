// findrec <ID> — busca no arquivo de dados pela organizacao por hashing (Parte 1)
//
// Deve informar, ao final, a quantidade de blocos lidos na operação e os
// totais de blocos dos arquivos envolvidos. Todo acesso a disco deve passar
// pela camada blockio.

#include <cstdio>
#include <cstdlib>

#include "blockio.hpp"
#include "hash_file.hpp"
#include "paths.hpp"
#include "print_utils.hpp"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "uso: %s <ID>\n", argv[0]);
        return 2;
    }
    const int id = std::atoi(argv[1]);
    using ull = unsigned long long;
 
    try {
        HashFile hf;
        hf.open(kDataPath);
        Article a;
        const bool found = hf.find(id, a);
 
        std::printf("== findrec %d ==\n", id);
        std::printf("arquivo de dados : %s\n", kDataPath);
        if (found) {
            std::printf("registro encontrado:\n");
            print_article(a);
        } else {
            std::printf("registro com ID %d NAO encontrado.\n", id);
        }
        std::printf("blocos lidos     : %llu\n", (ull)hf.stats().reads);
        std::printf("total de blocos  : %llu (arquivo de dados, incl. 1 de metadados)\n",
                    (ull)hf.block_count());
        return found ? 0 : 3;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "erro: %s\n", e.what());
        return 1;
    }
}
