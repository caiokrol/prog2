#ifndef UTIL_H
#define UTIL_H

#include <time.h>
#include "diretorio.h"

//utiliza <sys/stat.h> para retornar a data da ultima modificação de um arquivo no formato ISO de C
time_t obter_data_mod(const char *path);
size_t size_of_file(FILE *file);

//Checa se o arquivo está vazio, retorna um booleano
bool is_blank(FILE *file);

void printar_info(const membro_t *m);
void gerar_nome_temporario(char *buffer, size_t size);

#endif
