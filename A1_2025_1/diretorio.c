#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "diretorio.h"

int carregar_diretorio(FILE *arq, membro_t **membros, int *qtd) {
    if (!arq || !membros || !qtd)
        return -1;

    // volta para o início do arquivo
    rewind(arq); // ref: https://man7.org/linux/man-pages/man3/rewind.3p.html

    // lê a quantidade de membros
    int total;
    if (fread(&total, sizeof(int), 1, arq) != 1) //ref: https://man7.org/linux/man-pages/man3/fread.3p.html
        return -1;

    // aloca vetor para armazenar os membros
    membro_t *lista = malloc(sizeof(membro_t) * total);
    if (!lista)
        return -1;

    // lê os membros do diretório
    if (fread(lista, sizeof(membro_t), total, arq) != (size_t)total) {
        free(lista);
        return -1;
    }

    *membros = lista;
    *qtd = total;
    return 0;
}


int salvar_diretorio(FILE *arq, membro_t *membros, int qtd) {
    if (!arq || !membros || qtd < 0)
        return -1;

    // Vai pro início do arquivo
    rewind(arq);

    // Grava a quantidade de membros
    if (fwrite(&qtd, sizeof(int), 1, arq) != 1)
        return -1;

    // Grava os membros
    if (fwrite(membros, sizeof(membro_t), qtd, arq) != (size_t)qtd)
        return -1;

    return 0;
}

int buscar_membro(membro_t *membros, int qtd, const char *nome) {
    if (!membros || !nome || qtd <= 0){
        return -1;
    };
    for (int i = 0; i < qtd; i++) {
        if (strcmp(membros[i].nome, nome) == 0)
            return i;
    }

    return -1; // não encontrado
}


__uid_t gerar_uid(membro_t *membros, int qtd) {
    __uid_t max_uid = 0;

    for (int i = 0; i < qtd; i++) {
        if (membros[i].uid > max_uid)
            max_uid = membros[i].uid;
    }

    return max_uid + 1;
}

int comparar_ordem(const void *a, const void *b) {
    const membro_t *ma = (const membro_t *)a;
    const membro_t *mb = (const membro_t *)b;

    return ma->ordem - mb->ordem;
}