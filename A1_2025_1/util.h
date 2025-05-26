#ifndef UTIL_H
#define UTIL_H

#include <time.h>
#include "diretorio.h"


/**
 * Obtém a data de modificação do arquivo no caminho `path`.
 * Retorna a data como `time_t`, ou (time_t)-1 em caso de erro.
 */
time_t obter_data_mod(const char *path);

/**
 * Imprime as informações do membro `m`:
 * nome, id, tamanho original, tamanho em disco, data de modificacao, 
 * ordem no arquivo e offset.
 */
void printar_info(const membro_t *m);

void listar_membros(membro_t *m, int qtd);

void imprimir_ajuda(const char *nome_prog);

void listar_membros_filtrados(membro_t *m, int qtd, size_t tamanho_min, size_t tamanho_max);

#endif
