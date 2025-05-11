#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include "arquivo.h"
#include "util.h"
#include "lz.h"
#include "diretorio.h"


void move(FILE *archive, long i_m, unsigned long tamanho, long l_m) {
    char *buffer = malloc(tamanho);
    if (!buffer) {
        fprintf(stderr, "Erro: falha ao alocar buffer na função move()\n");
        return;
    }

    if (fseek(archive, i_m, SEEK_SET) != 0) {
        fprintf(stderr, "Erro: fseek falhou na leitura em move()\n");
        free(buffer);
        return;
    }

    if (fread(buffer, 1, tamanho, archive) != tamanho) {
        fprintf(stderr, "Erro: fread falhou na função move()\n");
        free(buffer);
        return;
    }

    if (fseek(archive, l_m, SEEK_SET) != 0) {
        fprintf(stderr, "Erro: fseek falhou na escrita em move()\n");
        free(buffer);
        return;
    }

    if (fwrite(buffer, 1, tamanho, archive) != tamanho) {
        fprintf(stderr, "Erro: fwrite falhou na função move()\n");
        free(buffer);
        return;
    }

    free(buffer);
}


int inserir_membro(FILE *archive, membro_t **membros, int *qtd, int *cap, const char *caminho, int compressao) {
    // Extrai nome do arquivo
    const char *barra = strrchr(caminho, '/');
    const char *nome_arquivo = barra ? barra + 1 : caminho;

    // Verifica se já existe
    int pos = buscar_membro(*membros, *qtd, nome_arquivo);
    printf("POS: %d\n", pos);
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

// 1. Calcula novo tamanho do diretório
long novo_tam_dir = sizeof(int) + (*qtd) * sizeof(membro_t);

// 2. Realoca os dados dos membros antigos para abrir espaço para o novo diretório
if (*qtd > 1) {
    for (int i = *qtd - 2; i >= 0; i--) {
        long antigo_offset = (*membros)[i].offset;
        long novo_offset = antigo_offset + sizeof(membro_t);  // espaço extra para novo membro no diretório
        move(archive, antigo_offset, (*membros)[i].tamanho_disco, novo_offset);
        (*membros)[i].offset = novo_offset;  // atualiza offset só depois do move
    }
}

// 3. Atualiza offset do novo membro
if (*qtd == 1) {
    m->offset = novo_tam_dir;  // primeiro membro, logo após diretório
} else {
    membro_t *ultimo = &(*membros)[*qtd - 2];
    m->offset = ultimo->offset + ultimo->tamanho_disco;
}

// 4. Escreve o diretório atualizado (após os realocamentos)
rewind(archive);
salvar_diretorio(archive, *membros, *qtd);

// 5. Grava os dados do novo membro
fseek(archive, m->offset, SEEK_SET);
if (usar_compressao) {
    fwrite(buffer_out, 1, tamanho_final, archive);
} else {
    fwrite(buffer_in, 1, tamanho, archive);
}

m->tamanho_disco = tamanho_final;

// 6. Escreve novamente o diretório no início (opcional se já gravou no passo 4)
    rewind(archive);
    salvar_diretorio(archive, *membros, *qtd);

    free(buffer_in);
    free(buffer_out);

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
    printf("Chegou a chamar a funcao\n");
    if (!archive || !membros || !qtd || !nome || *qtd <= 0)
        return -1;
    printf("Viu se tudo eh != NULL\n");
    // 1) encontra índice
    int idx = buscar_membro(membros, *qtd, nome);
    if (idx < 0){
        printf("idx do membro: %d", idx);
        return -1;  // não existe

    }
    printf("Chegou a encontrar o membro\n");
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

int mover_membro(FILE *archive, membro_t *membros, int qtd,
                 const char *orig, const char *dest) {
    if (!archive || !membros || qtd <= 0 || !orig || !dest)
        return -1;

    // 1) localiza orig e dest
    int idx_o = buscar_membro(membros, qtd, orig);
    int idx_d = buscar_membro(membros, qtd, dest);
    if (idx_o < 0 || idx_d < 0)
        return -1;

    // 2) salva uma cópia temporária do membro a mover
    membro_t temp = membros[idx_o];

    // 3) remove o elemento orig do vetor
    memmove(&membros[idx_o],
            &membros[idx_o + 1],
            sizeof(membro_t) * (qtd - idx_o - 1));

    // 4) ajusta índice de inserção de dest, caso venha depois de orig
    if (idx_d > idx_o) 
        idx_d--;

    // 5) insere temp logo após idx_d: desloca o resto e coloca temp
    int insert_pos = idx_d + 1;
    memmove(&membros[insert_pos + 1],
            &membros[insert_pos],
            sizeof(membro_t) * (qtd - insert_pos - 1));
    membros[insert_pos] = temp;

    // 6) atualiza campo .ordem em todo o vetor
    for (int i = 0; i < qtd; i++) {
        membros[i].ordem = i;
    }

    // 7) regrava o diretório no início do arquivo
    rewind(archive);
    if (salvar_diretorio(archive, membros, qtd) != 0)
        return -1;

    return 0;
}


int inspecionar_membro(FILE *archive, membro_t *m) {
    if (!archive || !m) return -1;

    printf("==> Inspecionando membro: %s\n", m->nome);
    printf("  Offset: %ld\n", m->offset);
    printf("  Tamanho em disco: %zu\n", m->tamanho_disco);

    if (fseek(archive, m->offset, SEEK_SET) != 0) {
        perror("fseek");
        return -1;
    }

    unsigned char *buffer = malloc(m->tamanho_disco);
    if (!buffer) {
        perror("malloc");
        return -1;
    }

    size_t lidos = fread(buffer, 1, m->tamanho_disco, archive);
    if (lidos != m->tamanho_disco) {
        printf("Erro ao ler dados do membro. Esperado %zu, lido %zu\n", m->tamanho_disco, lidos);
        free(buffer);
        return -1;
    }

    printf("  Primeiros bytes do conteúdo:\n    ");
    for (size_t i = 0; i < (m->tamanho_disco < 16 ? m->tamanho_disco : 16); i++)
        printf("%02X ", buffer[i]);
    printf("\n");

    free(buffer);
    return 0;
}
