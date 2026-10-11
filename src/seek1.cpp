// seek1 <ID> — busca pelo indice primario em B+Tree (Parte 2)
//
// Deve informar, ao final, a quantidade de blocos lidos na operação e os
// totais de blocos dos arquivos envolvidos. Todo acesso a disco deve passar
// pela camada blockio.

#include <cstdio>
#include <cstdlib>

#include "blockio.hpp"
#include "bptree.hpp"
#include "hash_file.hpp"
#include "keys.hpp"
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
        BPlusTree<IdKey> idx;
        idx.open(kPrimaryPath);
        HashFile hf;
        hf.open(kDataPath);
 
        HashFile::Rid rid;
        Article a;
        bool found = idx.find_exact(IdKey{id}, rid);
        if (found) found = hf.read_at(rid, a);
 
        std::printf("== seek1 %d ==\n", id);
        if (found) {
            std::printf("registro encontrado:\n");
            print_article(a);
        } else {
            std::printf("registro com ID %d NAO encontrado.\n", id);
        }
        std::printf("blocos lidos no indice  : %llu\n", (ull)idx.stats().reads);
        std::printf("blocos lidos nos dados  : %llu\n", (ull)hf.stats().reads);
        std::printf("total de blocos do indice primario: %llu (incl. 1 de metadados)\n",
                    (ull)idx.block_count());
        return found ? 0 : 3;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "erro: %s\n", e.what());
        return 1;
    }
}
