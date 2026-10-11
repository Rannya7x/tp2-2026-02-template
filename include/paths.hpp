// Caminhos dos arquivos gerados em /data (documentar no README).
#ifndef PATHS_HPP
#define PATHS_HPP

constexpr const char* kDbDir        = "/data/db";
constexpr const char* kDataPath     = "/data/db/artigos.dat";   // arquivo de dados (hash)
constexpr const char* kPrimaryPath  = "/data/db/primario.idx";  // B+Tree por ID      (Parte 2)
constexpr const char* kSecondPath   = "/data/db/secundario.idx";// B+Tree por Titulo  (Parte 2)

#endif  // PATHS_HPP