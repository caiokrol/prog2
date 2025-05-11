#ifndef ARQUIVO_H
#define ARQUIVO_H

#include <stdio.h>
#include "diretorio.h"

int inserir_membro(FILE *archive, membro_t **membros, int *qtd, int *cap, const char *caminho, int compressao);
int extrair_membro(FILE *archive, membro_t *membro);
int remover_membro(FILE *archive, membro_t *membros, int *qtd, const char *nome);
int mover_membro(FILE *archive, membro_t *membros, int qtd, const char *orig, const char *dest);

int inspecionar_membro(FILE *archive, membro_t *m);

#endif
