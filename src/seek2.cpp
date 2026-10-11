// seek2 <Titulo> — busca pelo indice secundario em B+Tree (Parte 2)
//
// Deve informar, ao final, a quantidade de blocos lidos na operação e os
// totais de blocos dos arquivos envolvidos. Todo acesso a disco deve passar
// pela camada blockio.

// chave composta (titulo, id)). Titulos repetidos ficam adjacentes nas folhas:
// lower_bound em (titulo, -inf) e varredura enquanto o titulo for igual.

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "blockio.hpp"
#include "bptree.hpp"
#include "hash_file.hpp"
#include "keys.hpp"
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
        BPlusTree<TitleKey> idx;
        idx.open(kSecondPath);
        HashFile hf;
        hf.open(kDataPath);
 
        const TitleKey start = title_lower_key(titulo);
        std::vector<Article> found;
        idx.scan_from(start, [&](const TitleKey& k, HashFile::Rid rid) {
            if (std::strcmp(k.titulo, start.titulo) != 0) return false;  // outro titulo: fim
            Article a;
            if (hf.read_at(rid, a)) found.push_back(a);
            return true;
        });
 
        std::printf("== seek2 \"%s\" ==\n", titulo.c_str());
        std::printf("registros encontrados: %zu\n", found.size());
        print_results(found);
        std::printf("blocos lidos no indice  : %llu\n", (ull)idx.stats().reads);
        std::printf("blocos lidos nos dados  : %llu\n", (ull)hf.stats().reads);
        std::printf("total de blocos do indice secundario: %llu (incl. 1 de metadados)\n",
                    (ull)idx.block_count());
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "erro: %s\n", e.what());
        return 1;
    }
}
