#ifndef ARQUIVO_H
#define ARQUIVO_H

#include <stdio.h>
#include "diretorio.h"

/* Move o bloco [i_m...i_m + tamanho]archive para [l_m]archive */
void move(FILE *archive, long i_m, unsigned long tamanho, long l_m);

/**
 * Insere um novo membro no arquivo .vc.
 * Pode aplicar compressão se `compressao` for diferente de 0.
 * Atualiza o vetor de membros `membros`, seu tamanho `*qtd` e capacidade `*cap`.
 * Retorna 0 em sucesso ou -1 em caso de erro (como erro de leitura/escrita ou alocação).
 */
int inserir_membro(FILE *archive, membro_t **membros, int *qtd, int *cap, const char *caminho, int compressao);

/**
 * Extrai um membro do archive .vc para um arquivo no disco.
 * Se o membro estiver comprimido, ele é descomprimido antes.
 */
int extrair_membro(FILE *archive, membro_t *membro);

/**
 * Remove um membro do archive .vc:
 * - Exclui sua entrada do vetor `membros`
 * - Decrementa *qtd
 * - Salva o diretório atualizado no início de `archive`
 *
 * Retorna 0 em sucesso ou -1 em caso de erro (arquivo inválido ou membro não encontrado).
 */
int remover_membro(FILE *archive, membro_t *membros, int *qtd, const char *nome);

/**
 * Move um membro existente dentro do archive .vc, alterando sua posição.
 * O membro com nome `orig` será movido para a posição do membro `dest`.
 * Atualiza os offsets no vetor de membros conforme necessário.
 * Retorna 0 em caso de sucesso ou -1 se algum dos membros não for encontrado.
 */
int mover_membro(FILE *archive, membro_t *membros, int qtd, const char *orig, const char *dest);

/**
 *Inspeciona um membro
 *Imprime seu nome, offset, tamanho em disco, e seus 16 primeiros Bytes
 */
int inspecionar_membro(FILE *archive, membro_t *m);

#endif
