#ifndef UTIL_H
#define UTIL_H

#include <time.h>
#include "diretorio.h"

//utiliza <sys/stat.h> para retornar a data da ultima modificação de um arquivo no formato ISO de C
time_t obter_data_mod(const char *caminho);
void printar_info(const membro_t *m);
void gerar_nome_temporario(char *buffer, size_t tamanho);

#endif
