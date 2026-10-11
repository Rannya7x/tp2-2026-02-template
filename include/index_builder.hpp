// Construcao dos dois indices (B+Tree por ID e por Titulo) a partir do arquivo
// de dados: varre o arquivo, ordena as entradas em memoria e faz o bulk load.
#ifndef INDEX_BUILDER_HPP
#define INDEX_BUILDER_HPP

// Constroi kPrimaryPath e kSecondPath e imprime o custo da construcao.
void build_all_indexes(const char* data_path);

#endif  // INDEX_BUILDER_HPP