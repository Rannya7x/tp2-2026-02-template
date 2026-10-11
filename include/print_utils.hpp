// Formatacao de saida compartilhada pelos programas.
#ifndef PRINT_UTILS_HPP
#define PRINT_UTILS_HPP

#include <cstdio>
#include "record.hpp"

inline void print_article(const Article& a) {
    std::printf("  ID: %d\n  Titulo: %s\n  Ano: %d\n  Autores: %s\n  Citacoes: %d\n"
                "  Atualizacao: %s\n  Snippet: %s\n",
                a.id, a.titulo.c_str(), a.ano, a.autores.c_str(), a.citacoes,
                a.atualizacao.c_str(), a.snippet.c_str());
}

#endif  // PRINT_UTILS_HPP