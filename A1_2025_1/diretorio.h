#ifndef DIRETORIO_H
#define DIRETORIO_H

#include <stdio.h>
#include <time.h>

typedef struct {
    char nome[256];         // nome do membro (sem espaços)
    int uid;                // identificador único
    size_t tamanho_orig;    // tamanho original
    size_t tamanho_disco;   // tamanho após compressão (ou igual se não comprimido)
    time_t data_mod;        // data de modificação
    int ordem;              // ordem no arquivo
    long offset;            // onde os dados começam no archive
} membro_t;

int carregar_diretorio(FILE *archive, membro_t **membros, int *qtd);
int salvar_diretorio(FILE *archive, membro_t *membros, int qtd);
int buscar_membro(membro_t *membros, int qtd, const char *nome);
int gerar_uid(membro_t *membros, int qtd);
int comparar_ordem(const void *a, const void *b);

#endif
