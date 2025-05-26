#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "arquivo.h"
#include "diretorio.h"
#include "util.h"


#define EXTENSAO ".vc"
#define MAX_SIZE ((size_t)-1)

int termina_com(const char *str, const char *sufixo) {
    size_t len_str = strlen(str);
    size_t len_sufixo = strlen(sufixo);
    if (len_str < len_sufixo) return 0;
    return strcmp(str + len_str - len_sufixo, sufixo) == 0;
}


int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <opcao> <archive> [membros...]\nDigite %s -help <archive> para ver ajuda\n", argv[0], argv[0]);
        return 1;
    }

    const char *opcao = argv[1];
    char arquivo_vc[1024];

    if (termina_com(argv[2], EXTENSAO)) {
        strncpy(arquivo_vc, argv[2], sizeof(arquivo_vc));
        arquivo_vc[sizeof(arquivo_vc) - 1] = '\0';
    } else {
        snprintf(arquivo_vc, sizeof(arquivo_vc), "%s%s", argv[2], EXTENSAO);
    }

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

    // Modo -t (Inspecionar Membro)
    else if (strcmp(opcao, "-t") == 0) {
        for (int i = 0; i < qtd; i++) {
            inspecionar_membro(arq, &membros[i]);
      }
    }

    // Modo -ss (ordenar por tamanho)
    else if (strcmp(opcao, "-ss") == 0) {
        if (ordenar_membros_por_tamanho(arq, membros, qtd) == 0) {
        printf("Membros ordenados por tamanho.\n");
        listar_membros(membros, qtd);
        } else {
        fprintf(stderr, "Erro ao ordenar por tamanho.\n");
        }
}

    // Modo -sn (ordenar por nome)
    else if (strcmp(opcao, "-sn") == 0) {
        if (ordenar_membros_por_nome(arq, membros, qtd) == 0) {
        printf("Membros ordenados por nome.\n");
        listar_membros(membros, qtd);
        } else {
        fprintf(stderr, "Erro ao ordenar por nome.\n");
        }
    }
    // Modo -v (Verificar integridade)
    else if (strcmp(opcao, "-v") == 0) {
        verificar_integridade(arq, membros, qtd);
    }

    // -cbt <valor> lista membros com tamanho >= valor
    else if (strcmp(opcao, "-cbt") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Uso: %s -cbt <archive> <tamanho_min>\n", argv[0]);
            return 1;
        }
        size_t tamanho_min = (size_t) atol(argv[3]);
        listar_membros_filtrados(membros, qtd, tamanho_min, MAX_SIZE);
        fclose(arq);
        free(membros);
        return 0;
    }
    // -cst <valor> lista membros com tamanho <= valor
    else if (strcmp(opcao, "-cst") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Uso: %s -cst <archive> <tamanho_max>\n", argv[0]);
            return 1;
        }
        size_t tamanho_max = (size_t) atol(argv[3]);
        listar_membros_filtrados(membros, qtd, 0, tamanho_max);
        fclose(arq);
        free(membros);
        return 0;
    }

    else if (strcmp(argv[1], "-help") == 0) {
        imprimir_ajuda(argv[0]);
        return 0;
    }

    else {
        fprintf(stderr, "Opção inválida: %s\nDigite %s -help <archive> para ver ajuda\n", opcao, argv[0]);
    }

    free(membros);
    fclose(arq);
    return 0;
}
