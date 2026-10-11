#include "index_builder.hpp"

#include <algorithm>
#include <cstdio>
#include <vector>

#include "bptree.hpp"
#include "hash_file.hpp"
#include "keys.hpp"
#include "paths.hpp"

template <class Key>
static void report(const char* nome, const char* path, const BPlusTree<Key>& t,
                   unsigned long long escritos) {
    using T = BPlusTree<Key>;
    using ull = unsigned long long;
    std::printf("-- indice %s (%s) --\n", nome, path);
    std::printf("tamanho da chave        : %zu bytes\n", sizeof(Key));
    std::printf("entradas por folha      : %zu\n", T::kLeafCap);
    std::printf("ordem (max. filhos)     : %zu  (max. chaves por no interno: %zu)\n",
                T::kIntCap + 1, T::kIntCap);
    std::printf("folhas                  : %llu\n", (ull)t.num_leaves());
    std::printf("altura (niveis)         : %u\n", t.height());
    std::printf("blocos do indice        : %llu (+1 de metadados)\n", (ull)t.block_count() - 1);
    std::printf("blocos escritos (build) : %llu\n", escritos);
}

void build_all_indexes(const char* data_path) {
    using ull = unsigned long long;
    HashFile hf;
    hf.open(data_path);

    std::vector<BPlusTree<IdKey>::Entry> pe;
    std::vector<BPlusTree<TitleKey>::Entry> se;
    pe.reserve(hf.num_records());
    se.reserve(hf.num_records());
    hf.scan([&](const DiskRecord& d, HashFile::Rid rid) {
        pe.push_back({IdKey{d.id}, rid});
        se.push_back({make_title_key(d.titulo, d.id), rid});
    });
    std::printf("== construcao dos indices ==\n");
    std::printf("blocos lidos do arquivo de dados (varredura): %llu\n", (ull)hf.stats().reads);

    std::sort(pe.begin(), pe.end(), [](const auto& a, const auto& b) { return a.key < b.key; });
    std::sort(se.begin(), se.end(), [](const auto& a, const auto& b) { return a.key < b.key; });

    BPlusTree<IdKey> prim;
    prim.build(kPrimaryPath, pe);
    report("primario (ID)", kPrimaryPath, prim, prim.stats().writes);

    BPlusTree<TitleKey> sec;
    sec.build(kSecondPath, se);
    report("secundario (Titulo, ID)", kSecondPath, sec, sec.stats().writes);
}