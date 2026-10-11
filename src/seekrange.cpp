// seekrange <ID-inicial> <ID-final> — busca por intervalo percorrendo as folhas encadeadas do indice primario (Parte 2)
//
// Deve informar, ao final, a quantidade de blocos lidos na operação e os
// totais de blocos dos arquivos envolvidos. Todo acesso a disco deve passar
// pela camada blockio.

// localiza a primeira chave e percorre as folhas encadeadas ate passar do limite.
// Resultado em ordem crescente de ID.

#include <cstdio>
#include <cstdlib>
#include <vector>

#include "blockio.hpp"
#include "bptree.hpp"
#include "hash_file.hpp"
#include "keys.hpp"
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
        BPlusTree<IdKey> idx;
        idx.open(kPrimaryPath);
        HashFile hf;
        hf.open(kDataPath);
 
        std::vector<Article> found;
        idx.scan_from(IdKey{lo}, [&](const IdKey& k, HashFile::Rid rid) {
            if (k.id > hi) return false;
            Article a;
            if (hf.read_at(rid, a)) found.push_back(a);  // 1 bloco de dados por registro
            return true;
        });
 
        std::printf("== seekrange %d %d ==\n", lo, hi);
        std::printf("registros encontrados: %zu\n", found.size());
        print_results(found);
        std::printf("blocos lidos no indice  : %llu\n", (ull)idx.stats().reads);
        std::printf("blocos lidos nos dados  : %llu\n", (ull)hf.stats().reads);
        std::printf("total de blocos do indice primario: %llu (incl. 1 de metadados)\n",
                    (ull)idx.block_count());
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "erro: %s\n", e.what());
        return 1;
    }
}
