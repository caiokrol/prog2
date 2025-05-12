#ifndef DIRETORIO_H
#define DIRETORIO_H

#include <stdio.h>
#include <time.h>
#include <string.h>


typedef struct {
    char nome[1024];         // nome do membro
    __uid_t uid;            // identificador único
    size_t tamanho_orig;    // tamanho original
    size_t tamanho_disco;   // tamanho após compressão (ou igual se não comprimido)
    time_t data_mod;        // data de modificação
    int ordem;              // ordem no arquivo
    long offset;            // onde os dados começam no archive    
} membro_t;

/**
 * Carrega o diretório (metadados dos membros) do início do archive para memória.
 * Aloca dinamicamente o vetor `membros`, e preenche `qtd` com a quantidade de membros.
 * Retorna 0 em sucesso, ou -1 em caso de erro de leitura ou formato inválido.
 */
int carregar_diretorio(FILE *archive, membro_t **membros, int *qtd);

/**
 * Salva o diretório (metadados dos membros) no início do archive.
 * Substitui o diretório anterior no arquivo.
 * Retorna 0 em sucesso, ou -1 em caso de erro na escrita.
 */
int salvar_diretorio(FILE *archive, membro_t *membros, int qtd);

/**
 * Busca um membro pelo nome no vetor `membros`.
 * Retorna o índice do membro encontrado, ou -1 se não encontrado.
 */
int buscar_membro(membro_t *membros, int qtd, const char *nome);

/**
 * Gera um UID único que não colide com os membros existentes no vetor.
 * Retorna o novo UID.
 */
__uid_t gerar_uid(membro_t *membros, int qtd);

/**
 * Função de comparação para `qsort` com base no campo `ordem` dos membros.
 * Retorna negativo, zero ou positivo conforme a ordem de `a` e `b`.
 */
int comparar_ordem(const void *a, const void *b);

#endif
