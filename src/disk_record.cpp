#include "disk_record.hpp"

#include <algorithm>
#include <cstring>

static bool copy_field(char* dst, std::size_t max, const std::string& src) {
    std::size_t n = std::min(max, src.size());
    std::memcpy(dst, src.data(), n);
    dst[n] = '\0';
    return src.size() > max;
}

void to_disk(const Article& a, DiskRecord& d, TruncInfo* t) {
    std::memset(&d, 0, sizeof(d));
    d.id = a.id;
    d.ano = a.ano;
    d.citacoes = a.citacoes;
    copy_field(d.atualizacao, kAtualizacaoLen, a.atualizacao);
    bool tt = copy_field(d.titulo, kTituloMax, a.titulo);
    bool ta = copy_field(d.autores, kAutoresMax, a.autores);
    bool ts = copy_field(d.snippet, kSnippetMax, a.snippet);
    if (t) { t->titulo = tt; t->autores = ta; t->snippet = ts; }
}

void from_disk(const DiskRecord& d, Article& a) {
    a.id = d.id;
    a.ano = d.ano;
    a.citacoes = d.citacoes;
    a.atualizacao = d.atualizacao;
    a.titulo = d.titulo;
    a.autores = d.autores;
    a.snippet = d.snippet;
}

bool titulo_equals(const DiskRecord& d, const std::string& query) {
    std::size_t n = std::min(kTituloMax, query.size());
    return std::strlen(d.titulo) == n && std::memcmp(d.titulo, query.data(), n) == 0;
}