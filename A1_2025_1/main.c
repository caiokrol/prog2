#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "arquivo.h"
#include "diretorio.h"
#include "util.h"

void listar_membros(membro_t *m, int qtd) {
    printf("%-20s %-5s %-10s %-10s %-20s %-6s %-8s\n",
        "Nome", "UID", "TamOrig", "TamDisco", "DataMod", "Ordem", "Offset");
    for (int i = 0; i < qtd; i++) {
        char data[64];
        strftime(data, sizeof(data), "%d/%m/%Y %H:%M:%S", localtime(&m[i].data_mod));
        printf("%-20s %-5d %-10zu %-10zu %-20s %-6d %-8ld\n",
            m[i].nome, m[i].uid, m[i].tamanho_orig,
            m[i].tamanho_disco, data, m[i].ordem, m[i].offset);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <opcao> <archive> [membros...]\n", argv[0]);
        return 1;
    }

    const char *opcao = argv[1];
    const char *arquivo_vc = argv[2];

    FILE *arq = fopen(arquivo_vc, "r+b");
    if (!arq) arq = fopen(arquivo_vc, "w+b");
    if (!arq) {
        perror("Erro ao abrir/criar arquivo");
        return 1;
    }

    membro_t *membros = NULL;
    int qtd = 0, cap = 0;

    // Tenta carregar o diretório
    carregar_diretorio(arq, &membros, &qtd);

    // Modo -ip ou -ic (inserção com ou sem compressão)
    if (strcmp(opcao, "-ip") == 0 || strcmp(opcao, "-ic") == 0) {
        int compressao = (strcmp(opcao, "-ic") == 0);
        for (int i = 3; i < argc; i++) {
            if (inserir_membro(arq, &membros, &qtd, &cap, argv[i], compressao) == 0)
                printf("Inserido: %s\n", argv[i]);
            else
                fprintf(stderr, "Erro ao inserir: %s\n", argv[i]);
        }
    }

    // Modo -x (extrair)
    else if (strcmp(opcao, "-x") == 0) {
        if (argc == 3) {
            for (int i = 0; i < qtd; i++) {
                if (extrair_membro(arq, &membros[i]) == 0)
                    printf("Extraído: %s\n", membros[i].nome);
            }
        } else {
            for (int i = 3; i < argc; i++) {
                int idx = buscar_membro(membros, qtd, argv[i]);
                if (idx >= 0 && extrair_membro(arq, &membros[idx]) == 0)
                    printf("Extraído: %s\n", membros[idx].nome);
                else
                    fprintf(stderr, "Erro ao extrair: %s\n", argv[i]);
            }
        }
    }

    // Modo -r (remover)
    else if (strcmp(opcao, "-r") == 0) {
        for (int i = 3; i < argc; i++) {
            if (remover_membro(arq, membros, &qtd, argv[i]) == 0)
                printf("Removido: %s\n", argv[i]);
            else
                fprintf(stderr, "Erro ao remover: %s\n", argv[i]);
        }
    }

    // Modo -m (mover)
    else if (strcmp(opcao, "-m") == 0) {
        if (argc != 5) {
            fprintf(stderr, "Uso: %s -m <archive> <orig> <dest>\n", argv[0]);
            return 1;
        }
        const char *orig = argv[3];
        const char *dest = argv[4];
        if (mover_membro(arq, membros, qtd, orig, dest) == 0)
            printf("Movido %s após %s\n", orig, dest);
        else
            fprintf(stderr, "Erro ao mover %s após %s\n", orig, dest);
    }

    // Modo -c (listar conteúdo)
    else if (strcmp(opcao, "-c") == 0) {
        listar_membros(membros, qtd);
    }

    else if (strcmp(opcao, "-t") == 0) {
        for (int i = 0; i < qtd; i++) {
            inspecionar_membro(arq, &membros[i]);
      }
    }


    else {
        fprintf(stderr, "Opção inválida: %s\n", opcao);
    }

    free(membros);
    fclose(arq);
    return 0;
}
