// Layout fisico do registro no arquivo de dados: TAMANHO FIXO, com truncamento
// dos campos de texto

#ifndef DISK_RECORD_HPP
#define DISK_RECORD_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

#include "record.hpp"

constexpr std::size_t kTituloMax      = 300;
constexpr std::size_t kAutoresMax     = 150;
constexpr std::size_t kSnippetMax     = 1024;
constexpr std::size_t kAtualizacaoLen = 19;  // "AAAA-MM-DD HH:MM:SS"

struct DiskRecord {
    std::int32_t id;
    std::int32_t ano;
    std::int32_t citacoes;
    char atualizacao[kAtualizacaoLen + 1];
    char titulo[kTituloMax + 1];
    char autores[kAutoresMax + 1];
    char snippet[kSnippetMax + 1];
};
static_assert(std::is_trivially_copyable<DiskRecord>::value, "DiskRecord deve ser POD");

// Indica quais campos foram truncados na conversao
struct TruncInfo {
    bool titulo = false, autores = false, snippet = false;
};

void to_disk(const Article& a, DiskRecord& d, TruncInfo* t = nullptr);
void from_disk(const DiskRecord& d, Article& a);

// Compara o titulo gravado com a consulta (a consulta e truncada a kTituloMax
// bytes, igual ao que foi gravado)
bool titulo_equals(const DiskRecord& d, const std::string& query);

#endif  // DISK_RECORD_HPP