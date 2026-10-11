// Formatacao de saida compartilhada pelos programas.
#ifndef PRINT_UTILS_HPP
#define PRINT_UTILS_HPP

#include <cstdio>
#include <vector>

#include "record.hpp"

// Acima deste numero de resultados, a listagem passa para o formato compacto
// (uma linha por registro) para nao inundar o terminal.
constexpr std::size_t kMaxFullPrint = 20;
 
inline void print_article(const Article& a) {
    std::printf("  ID: %d\n  Titulo: %s\n  Ano: %d\n  Autores: %s\n  Citacoes: %d\n"
                "  Atualizacao: %s\n  Snippet: %s\n",
                a.id, a.titulo.c_str(), a.ano, a.autores.c_str(), a.citacoes,
                a.atualizacao.c_str(), a.snippet.c_str());
}
 
inline void print_article_line(const Article& a) {
    std::printf("  [%d] %s (%d) | autores: %s | citacoes: %d\n", a.id, a.titulo.c_str(),
                a.ano, a.autores.c_str(), a.citacoes);
}
 
// Lista os resultados: completos se forem poucos, compactos se forem muitos.
inline void print_results(const std::vector<Article>& rs) {
    if (rs.size() <= kMaxFullPrint) {
        for (std::size_t i = 0; i < rs.size(); ++i) {
            std::printf("registro %zu de %zu:\n", i + 1, rs.size());
            print_article(rs[i]);
        }
    } else {
        std::printf("(%zu registros: listagem compacta)\n", rs.size());
        for (const Article& a : rs) print_article_line(a);
    }
}
 
#endif  // PRINT_UTILS_HPP