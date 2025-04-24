#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "arquivo.h"
#include "util.h"
#include "lz.h"
#include "diretorio.h"

int inserir_membro(FILE *archive, membro_t **membros, int *qtd, int *cap, const char *caminho, int compressao) {
    // Extrai nome do arquivo
    const char *barra = strrchr(caminho, '/');
    const char *nome_arquivo = barra ? barra + 1 : caminho;

    // Verifica se já existe
    int pos = buscar_membro(*membros, *qtd, nome_arquivo);

    // Se já existe, substitui
    if (pos >= 0) {
        (*membros)[pos].offset = -1;  // marca o anterior como inválido, se quiser
    } else {
        // Expande vetor se necessário
        if (*qtd >= *cap) {
            int nova_cap = (*cap == 0) ? 10 : (*cap * 2);
            membro_t *novo = realloc(*membros, nova_cap * sizeof(membro_t));
            if (!novo) return -1;
            *membros = novo;
            *cap = nova_cap;
        }
        pos = (*qtd)++;
    }

    membro_t *m = &(*membros)[pos];
    memset(m, 0, sizeof(membro_t));

    strcpy(m->nome, nome_arquivo);
    m->uid = gerar_uid(*membros, *qtd);
    m->data_mod = obter_data_mod(caminho);
    m->ordem = pos;  // ou algum contador global de ordem crescente

    // Abre arquivo original
    FILE *entrada = fopen(caminho, "rb");
    if (!entrada) return -1;

    fseek(entrada, 0, SEEK_END);
    long tamanho = ftell(entrada);
    rewind(entrada);

    if (tamanho < 0) {
        fclose(entrada);
        return -1;
    }

    unsigned char *buffer_in = malloc(tamanho);
    if (!buffer_in) {
        fclose(entrada);
        return -1;
    }

    fread(buffer_in, 1, tamanho, entrada);
    fclose(entrada);
    m->tamanho_orig = tamanho;

    unsigned char *buffer_out = malloc(tamanho * 2);
    if (!buffer_out) {
        free(buffer_in);
        return -1;
    }

    int usar_compressao = 0;
    int tamanho_final = tamanho;

    if (compressao) {
        int comprimido = LZ_Compress(buffer_in, buffer_out, tamanho);
        if (comprimido > 0 && comprimido < tamanho) {
            tamanho_final = comprimido;
            usar_compressao = 1;
        }
    }

    fseek(archive, 0, SEEK_END);
    m->offset = ftell(archive);
    m->tamanho_disco = tamanho_final;

    if (usar_compressao) {
        fwrite(buffer_out, 1, tamanho_final, archive);
    } else {
        fwrite(buffer_in, 1, tamanho, archive);
    }

    free(buffer_in);
    free(buffer_out);

    // Volta ao início e salva o diretório atualizado
    rewind(archive);
    salvar_diretorio(archive, *membros, *qtd);

    return 0;
}


/**
 * Extrai um membro do archive .vc para um arquivo no disco.
 * Se o membro estiver comprimido, ele é descomprimido antes.
 */
int extrair_membro(FILE *archive, membro_t *membro) {
    if (!archive || !membro)
        return -1;

    // Se o arquivo for vazio, apenas cria o arquivo
    if (membro->tamanho_disco == 0) {
        FILE *out = fopen(membro->nome, "wb");
        if (out) fclose(out);
        return 0;
    }

    // Aloca buffer para ler os dados do .vc
    unsigned char *buffer_in = malloc(membro->tamanho_disco);
    if (!buffer_in) return -1;

    // Vai até o offset correto e lê os dados
    if (fseek(archive, membro->offset, SEEK_SET) != 0 ||
        fread(buffer_in, 1, membro->tamanho_disco, archive) != membro->tamanho_disco) {
        free(buffer_in);
        return -1;
    }

    // Abre arquivo de saída
    FILE *saida = fopen(membro->nome, "wb");
    if (!saida) {
        free(buffer_in);
        return -1;
    }

    // Verifica se precisa descomprimir
    int precisa_descomprimir = (membro->tamanho_orig > membro->tamanho_disco);

    if (precisa_descomprimir) {
        unsigned char *buffer_out = malloc(membro->tamanho_orig);
        if (!buffer_out) {
            fclose(saida);
            free(buffer_in);
            return -1;
        }

        LZ_Uncompress(buffer_in, buffer_out, membro->tamanho_disco);

        // Grava dados descomprimidos no disco
        fwrite(buffer_out, 1, membro->tamanho_orig, saida);
        free(buffer_out);
    } else {
        // Grava dados diretamente (não comprimido)
        fwrite(buffer_in, 1, membro->tamanho_disco, saida);
    }

    fclose(saida);
    free(buffer_in);

    // Opcional: restaurar a data de modificação original
    struct utimbuf tempos;
    tempos.actime = membro->data_mod;    // tempo de acesso
    tempos.modtime = membro->data_mod;   // tempo de modificação
    utime(membro->nome, &tempos);

    return 0;
}

/**
 * Remove um membro do archive .vc:
 * - Exclui sua entrada do vetor `membros`
 * - Decrementa *qtd
 * - Salva o diretório atualizado no início de `archive`
 *
 * Retorna 0 em sucesso ou -1 em caso de erro (arquivo inválido ou membro não encontrado).
 */
int remover_membro(FILE *archive, membro_t *membros, int *qtd, const char *nome) {
    if (!archive || !membros || !qtd || !nome || *qtd <= 0)
        return -1;

    // 1) encontra índice
    int idx = buscar_membro(membros, *qtd, nome);
    if (idx < 0)
        return -1;  // não existe

    // 2) desloca o restante do vetor para “apagar” o elemento idx
    memmove(&membros[idx],
            &membros[idx + 1],
            sizeof(membro_t) * ((*qtd - 1) - idx));

    // 3) decrementa quantidade
    (*qtd)--;

    // 4) regrava o diretório no início do arquivo
    rewind(archive);
    if (salvar_diretorio(archive, membros, *qtd) != 0)
        return -1;

    return 0;
}

