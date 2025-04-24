#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "diretorio.h"

int main() {
    const char *nome_arquivo = "teste.vc";

    // Criando dois membros fictícios
    membro_t membros[2];

    strcpy(membros[0].nome, "arquivo1.txt");
    membros[0].uid = 1001;
    membros[0].tamanho_orig = 1234;
    membros[0].tamanho_disco = 1100;
    membros[0].data_mod = time(NULL);
    membros[0].ordem = 0;
    membros[0].offset = 1024;

    strcpy(membros[1].nome, "imagem.png");
    membros[1].uid = 1002;
    membros[1].tamanho_orig = 4321;
    membros[1].tamanho_disco = 4000;
    membros[1].data_mod = time(NULL);
    membros[1].ordem = 1;
    membros[1].offset = 2048;

    // Grava os membros no arquivo
    FILE *f = fopen(nome_arquivo, "wb");
    if (!f) {
        perror("Erro abrindo para escrita");
        return 1;
    }

    if (salvar_diretorio(f, membros, 2) != 0) {
        fprintf(stderr, "Erro ao salvar diretório\n");
        fclose(f);
        return 1;
    }
    fclose(f);

    // Agora vamos carregar e imprimir pra testar
    membro_t *carregados = NULL;
    int qtd = 0;

    f = fopen(nome_arquivo, "rb");
    if (!f) {
        perror("Erro abrindo para leitura");
        return 1;
    }

    if (carregar_diretorio(f, &carregados, &qtd) != 0) {
        fprintf(stderr, "Erro ao carregar diretório\n");
        fclose(f);
        return 1;
    }

    printf("Foram carregados %d membros:\n", qtd);
    for (int i = 0; i < qtd; i++) {
        printf("- %s (UID: %d, Orig: %zu, Disco: %zu, Ordem: %d, Offset: %ld)\n",
            carregados[i].nome,
            carregados[i].uid,
            carregados[i].tamanho_orig,
            carregados[i].tamanho_disco,
            carregados[i].ordem,
            carregados[i].offset
        );
    }

    free(carregados);
    fclose(f);

    return 0;
}
