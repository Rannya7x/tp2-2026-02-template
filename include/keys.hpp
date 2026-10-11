// Chaves dos indices (tamanho fixo, para caber nos nos da B+Tree).
//  - IdKey   : indice primario (ID).
//  - TitleKey: indice secundario, chave COMPOSTA (titulo, id). O id desempata
//              titulos repetidos, entao todas as chaves sao unicas e a busca
//              por titulo vira "lower_bound (titulo, -inf) e varrer enquanto
//              o titulo for igual".

#ifndef KEYS_HPP
#define KEYS_HPP

#include <cstdint>
#include <cstring>
#include <limits>
#include <string_view>

#include "disk_record.hpp"

struct IdKey {
    std::int32_t id;
};
inline bool operator<(const IdKey& a, const IdKey& b) { return a.id < b.id; }

struct TitleKey {
    char titulo[kTituloMax + 1];
    std::int32_t id;
};
inline bool operator<(const TitleKey& a, const TitleKey& b) {
    int c = std::strcmp(a.titulo, b.titulo);
    return c != 0 ? c < 0 : a.id < b.id;
}

// Monta a chave; o titulo e truncado a kTituloMax bytes (igual ao gravado).
inline TitleKey make_title_key(std::string_view titulo, std::int32_t id) {
    TitleKey k;
    std::memset(&k, 0, sizeof(k));
    std::size_t n = titulo.size() < kTituloMax ? titulo.size() : kTituloMax;
    std::memcpy(k.titulo, titulo.data(), n);
    k.id = id;
    return k;
}
// Menor chave possivel para um titulo (usada como ponto de partida da busca).
inline TitleKey title_lower_key(std::string_view titulo) {
    return make_title_key(titulo, std::numeric_limits<std::int32_t>::min());
}

#endif  // KEYS_HPP