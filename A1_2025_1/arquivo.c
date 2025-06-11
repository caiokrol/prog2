#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <unistd.h>
#include "arquivo.h"
#include "util.h"
#include "lz.h"
#include "diretorio.h"


void move(FILE *archive, long i_m, unsigned long tamanho, long l_m) {
    // Aloca buffer para armazenar temporariamente o bloco a ser movido
    char *buffer = malloc(tamanho);
    if (!buffer) {
        fprintf(stderr, "Erro: falha ao alocar buffer na função move()\n");
        return;
    }

    // Posiciona o ponteiro de leitura no início do bloco original
    if (fseek(archive, i_m, SEEK_SET) != 0) {
        fprintf(stderr, "Erro: fseek falhou na leitura em move()\n");
        free(buffer);
        return;
    }

    // Lê o bloco para o buffer
    if (fread(buffer, 1, tamanho, archive) != tamanho) {
        fprintf(stderr, "Erro: fread falhou na função move()\n");
        free(buffer);
        return;
    }

    // Posiciona o ponteiro de escrita no novo local
    if (fseek(archive, l_m, SEEK_SET) != 0) {
        fprintf(stderr, "Erro: fseek falhou na escrita em move()\n");
        free(buffer);
        return;
    }

    // Escreve o conteúdo do buffer na nova posição
    if (fwrite(buffer, 1, tamanho, archive) != tamanho) {
        fprintf(stderr, "Erro: fwrite falhou na função move()\n");
        free(buffer);
        return;
    }

    // Libera o buffer
    free(buffer);
}

int inserir_membro(FILE *archive, membro_t **membros, int *qtd, int *cap, const char *caminho, int compressao) {
    // Extrai nome do arquivo a partir do caminho
    const char *barra = strrchr(caminho, '/');
    const char *nome_arquivo = barra ? barra + 1 : caminho;

    // Verifica se o membro já existe
    int pos = buscar_membro(*membros, *qtd, nome_arquivo);

        // Abre arquivo original
    FILE *entrada = fopen(caminho, "rb");
    if (!entrada) return -1;

    fseek(entrada, 0, SEEK_END);
    size_t tamanho = ftell(entrada);
    rewind(entrada);
    if (!(tamanho > 0)) {
        fclose(entrada);
        return -1;
    }
    
    if (pos >= 0) {
            remover_membro(archive, *membros, qtd, nome_arquivo);
            pos = (*qtd)++;
    } else {
        // Realoca vetor se necessário
        if (*qtd >= *cap) {
            int nova_cap = (*cap == 0) ? 10 : (*cap * 2);
            membro_t *novo = realloc(*membros, nova_cap * sizeof(membro_t));
            if (!novo) return -1;
            *membros = novo;
            *cap = nova_cap;
        }
        pos = (*qtd)++;
    }

    // Inicializa novo membro
    membro_t *m = &(*membros)[pos];
    memset(m, 0, sizeof(membro_t));
    strcpy(m->nome, nome_arquivo);
    m->uid = gerar_uid(*membros, *qtd);
    m->data_mod = obter_data_mod(caminho);
    m->ordem = pos;





    // Lê conteúdo do arquivo
    unsigned char *buffer_in = malloc(tamanho);
    if (!buffer_in) {
        fclose(entrada);
        return -1;
    }

    size_t lidos = fread(buffer_in, 1, tamanho, entrada);
    if (lidos != (size_t)tamanho) {
        fprintf(stderr, "Erro ao ler o arquivo %s\n", caminho);
        free(buffer_in);
        fclose(entrada);
        return -1;
    }
    fclose(entrada);
    m->tamanho_orig = tamanho;

    // Prepara buffer para compressão
    unsigned char *buffer_out = malloc(tamanho * 2);
    if (!buffer_out) {
        free(buffer_in);
        return -1;
    }

    // Aplica compressão se solicitada e vantajosa
    int usar_compressao = 0;
    size_t tamanho_final = tamanho;
    if (compressao) {
        size_t comprimido = LZ_Compress(buffer_in, buffer_out, tamanho);
        if (comprimido > 0 && comprimido < tamanho) {
            tamanho_final = comprimido;
            usar_compressao = 1;
        }
    }

    // Calcula novo tamanho do diretório
    long novo_tam_dir = sizeof(int) + (*qtd) * sizeof(membro_t);

    // Reposiciona os dados dos membros existentes (caso haja)
    if (*qtd > 1) {
        for (int i = *qtd - 2; i >= 0; i--) {
            long antigo_offset = (*membros)[i].offset;
            long novo_offset = antigo_offset + sizeof(membro_t);
            move(archive, antigo_offset, (*membros)[i].tamanho_disco, novo_offset);
            (*membros)[i].offset = novo_offset;
        }
    }

    // Define offset do novo membro
    if (*qtd == 1) {
        m->offset = novo_tam_dir;
    } else {
        membro_t *ultimo = &(*membros)[*qtd - 2];
        m->offset = ultimo->offset + ultimo->tamanho_disco;
    }

    // Grava diretório atualizado
    rewind(archive);
    salvar_diretorio(archive, *membros, *qtd);

    // Grava os dados do novo membro
    fseek(archive, m->offset, SEEK_SET);
    if (usar_compressao) {
        fwrite(buffer_out, 1, tamanho_final, archive);
    } else {
        fwrite(buffer_in, 1, tamanho, archive);
    }
    m->tamanho_disco = tamanho_final;

    // Regrava o diretório no início
    rewind(archive);
    salvar_diretorio(archive, *membros, *qtd);

    free(buffer_in);
    free(buffer_out);

    return 0;
}

int extrair_membro(FILE *archive, membro_t *membro) {
    if (!archive || !membro)
        return -1;

    // Caso o arquivo esteja vazio, cria o arquivo de saída vazio
    if (membro->tamanho_disco == 0) {
        FILE *out = fopen(membro->nome, "wb");
        if (out) fclose(out);
        return 0;
    }

    // Aloca buffer para ler os dados compactados ou não
    unsigned char *buffer_in = malloc(membro->tamanho_disco);
    if (!buffer_in) return -1;

    // Lê os dados do arquivo archive a partir do offset
    if (fseek(archive, membro->offset, SEEK_SET) != 0 ||
        fread(buffer_in, 1, membro->tamanho_disco, archive) != membro->tamanho_disco) {
        free(buffer_in);
        return -1;
    }

    // Abre o arquivo de saída
    FILE *saida = fopen(membro->nome, "wb");
    if (!saida) {
        free(buffer_in);
        return -1;
    }

    // Verifica se o conteúdo está comprimido
    int precisa_descomprimir = (membro->tamanho_orig > membro->tamanho_disco);

    if (precisa_descomprimir) {
        // Aloca buffer para dados descomprimidos
        unsigned char *buffer_out = malloc(membro->tamanho_orig);
        if (!buffer_out) {
            fclose(saida);
            free(buffer_in);
            return -1;
        }

        // Descomprime e escreve no arquivo de saída
        LZ_Uncompress(buffer_in, buffer_out, membro->tamanho_disco);
        fwrite(buffer_out, 1, membro->tamanho_orig, saida);
        free(buffer_out);
    } else {
        // Escreve dados diretamente no arquivo (sem compressão)
        fwrite(buffer_in, 1, membro->tamanho_disco, saida);
    }

    fclose(saida);
    free(buffer_in);

    // Restaura a data de modificação original
    struct utimbuf tempos;
    tempos.actime = membro->data_mod;
    tempos.modtime = membro->data_mod;
    utime(membro->nome, &tempos);

    return 0;
}

int remover_membro(FILE *archive, membro_t *membros, int *qtd, const char *nome) {
    if (!archive || !membros || !qtd || *qtd <= 0 || !nome)
        return -1;

    // Localiza o membro
    int idx = buscar_membro(membros, *qtd, nome);
    if (idx < 0) {
        return -1;  // Membro não encontrado
    }

    long tamanho_removido = membros[idx].tamanho_disco;
    long offset_removido = membros[idx].offset;
    int ordem_removida = membros[idx].ordem;

    // Calcula o tamanho atual do diretório
    long tam_dir_atual = sizeof(int) + (*qtd) * sizeof(membro_t);

    // Pega tamanho atual do arquivo
    fseek(archive, 0, SEEK_END);
    long fim_arquivo = ftell(archive);
    if (fim_arquivo < 0) {
        perror("ftell");
        return -1;
    }

    // Verifica se há dados após o membro removido
    long bytes_apos = (offset_removido + tamanho_removido < fim_arquivo)
                       ? (fim_arquivo - (offset_removido + tamanho_removido))
                       : 0;

    // Move dados seguintes (se houver)
    if (bytes_apos > 0) {
        move(archive,
             offset_removido + tamanho_removido,
             bytes_apos,
             offset_removido);
    }

    // Atualiza offsets dos membros após o removido
    for (int i = 0; i < *qtd; i++) {
        if (membros[i].offset > offset_removido) {
            membros[i].offset -= tamanho_removido;
        }
    }

        // Atualiza ordem dos membros
    for (int i = 0; i < *qtd; i++) {
        if (membros[i].ordem > ordem_removida) {
            membros[i].ordem--;
        }
    }

    // Remove membro do vetor (desloca elementos)
    memmove(&membros[idx],
            &membros[idx + 1],
            sizeof(membro_t) * ((*qtd - 1) - idx));
    (*qtd)--;

    // Calcula novo tamanho do diretório
    long tam_dir_novo = sizeof(int) + (*qtd) * sizeof(membro_t);
    long delta_dir = tam_dir_novo - tam_dir_atual;

    // Se o diretório diminuiu, move os dados dos membros para cima
    if (delta_dir < 0 && *qtd > 0) {
        long deslocamento = -delta_dir;
        for (int i = 0; i < *qtd; i++) {
            move(archive,
                 membros[i].offset,
                 membros[i].tamanho_disco,
                 membros[i].offset - deslocamento);
            membros[i].offset -= deslocamento;
        }
    }

    // Trunca o arquivo
    long novo_tamanho = fim_arquivo - tamanho_removido;
    if (delta_dir < 0) {
        novo_tamanho -= -delta_dir;
    }

    if (ftruncate(fileno(archive), novo_tamanho) != 0) {
        perror("ftruncate");
        return -1;
    }

    // Salva diretório atualizado
    rewind(archive);
    if (salvar_diretorio(archive, membros, *qtd) != 0) {
        fprintf(stderr, "Erro ao salvar diretório após remoção\n");
        return -1;
    }

    return 0;
}

int mover_membro(FILE *archive, membro_t *membros, int qtd, const char *orig, const char *dest) {
    if (!archive || !membros || qtd <= 1 || !orig || !dest)
        return -1;

    int idx_o = buscar_membro(membros, qtd, orig);
    int idx_d = buscar_membro(membros, qtd, dest);
    if (idx_o < 0 || idx_d < 0 || idx_o == idx_d)
        return -1;

    if ((idx_o == idx_d) || (idx_o == idx_d + 1)) {
        printf("O objeto já está na posição correta\n");
        return -1; // Já está na posição certa
    }

    // Salvar membro a ser movido
    membro_t temp = membros[idx_o];

    // Remoção e inserção na nova posição
    if (idx_o < idx_d) {
        memmove(&membros[idx_o],
                &membros[idx_o + 1],
                sizeof(membro_t) * (idx_d - idx_o));
        membros[idx_d] = temp;
    } else {
        memmove(&membros[idx_d + 1],
                &membros[idx_d],
                sizeof(membro_t) * (idx_o - idx_d));
        membros[idx_d] = temp;
    }

    // Calcular espaço total necessário
    long dados_totais = 0;
    for (int i = 0; i < qtd; i++) {
        dados_totais += membros[i].tamanho_disco;
    }

    // Buffer pra segurar todos os dados
    unsigned char *buffer = malloc(dados_totais);
    if (!buffer) {
        fprintf(stderr, "Erro de memória.\n");
        return -1;
    }

    // Lê os dados de cada membro, na nova ordem
    long pos_buffer = 0;
    for (int i = 0; i < qtd; i++) {
        fseek(archive, membros[i].offset, SEEK_SET);
        size_t lidos = fread(buffer + pos_buffer, 1, membros[i].tamanho_disco, archive);
        if (lidos != (size_t)membros[i].tamanho_disco) {
            fprintf(stderr, "Erro ao ler dados do membro %s\n", membros[i].nome);
            free(buffer);
            return -1;
        }
        membros[i].offset = sizeof(int) + qtd * sizeof(membro_t) + pos_buffer;
        membros[i].ordem = i;
        pos_buffer += membros[i].tamanho_disco;
    }

    // Regrava diretório
    rewind(archive);
    if (salvar_diretorio(archive, membros, qtd) != 0) {
        free(buffer);
        return -1;
    }

    // Grava dados reorganizados
    pos_buffer = 0;
    for (int i = 0; i < qtd; i++) {
        fseek(archive, membros[i].offset, SEEK_SET);
        fwrite(buffer + pos_buffer, 1, membros[i].tamanho_disco, archive);
        pos_buffer += membros[i].tamanho_disco;
    }

    free(buffer);
    return 0;
}

int inspecionar_membro(FILE *archive, membro_t *m) {
    if (!archive || !m) return -1;

    // Exibe informações sobre o membro
    printf("==> Inspecionando membro: %s\n", m->nome);
    printf("  Offset: %ld\n", m->offset);
    printf("  Tamanho em disco: %zu\n", m->tamanho_disco);

    // Vai até o offset do membro no arquivo
    if (fseek(archive, m->offset, SEEK_SET) != 0) {
        perror("fseek");
        return -1;
    }

    // Aloca buffer para ler os dados
    unsigned char *buffer = malloc(m->tamanho_disco);
    if (!buffer) {
        perror("malloc");
        return -1;
    }

    // Lê os dados do membro do arquivo
    size_t lidos = fread(buffer, 1, m->tamanho_disco, archive);
    if (lidos != m->tamanho_disco) {
        printf("Erro ao ler dados do membro. Esperado %zu, lido %zu\n", m->tamanho_disco, lidos);
        free(buffer);
        return -1;
    }

    // Exibe os primeiros bytes dos dados lidos
    printf("  Primeiros bytes do conteúdo:\n    ");
    for (size_t i = 0; i < (m->tamanho_disco < 16 ? m->tamanho_disco : 16); i++)
        printf("%02X ", buffer[i]);
    printf("\n");

    free(buffer);
    return 0;
}

// Função de comparação para qsort — ordena por tamanho crescente
int comparar_por_tamanho(const void *a, const void *b) {
    const membro_t *m1 = (const membro_t *)a;
    const membro_t *m2 = (const membro_t *)b;
    return (m1->tamanho_disco - m2->tamanho_disco);
}

// Função para ordenar os membros por tamanho
int ordenar_membros_por_tamanho(FILE *archive, membro_t *membros, int qtd) {
    if (!archive || !membros || qtd <= 1)
        return -1;

    // Ordenar vetor na RAM
    qsort(membros, qtd, sizeof(membro_t), comparar_por_tamanho);

    // Calcular espaço total dos dados
    long total_dados = 0;
    for (int i = 0; i < qtd; i++) {
        total_dados += membros[i].tamanho_disco;
    }

    // Alocar buffer para os dados
    unsigned char *buffer = malloc(total_dados);
    if (!buffer) {
        fprintf(stderr, "Erro de memória.\n");
        return -1;
    }

    // Ler os dados dos membros (na ordem antiga)
    long pos_buffer = 0;
    for (int i = 0; i < qtd; i++) {
        fseek(archive, membros[i].offset, SEEK_SET);
        size_t lidos = fread(buffer + pos_buffer, 1, membros[i].tamanho_disco, archive);
        if (lidos != (size_t)membros[i].tamanho_disco) {
            fprintf(stderr, "Erro ao ler dados do membro %s\n", membros[i].nome);
            free(buffer);
            return -1;
        }
        pos_buffer += membros[i].tamanho_disco;
    }

    // Atualizar offsets e ordem
    long offset_inicial = sizeof(int) + qtd * sizeof(membro_t);
    pos_buffer = 0;
    for (int i = 0; i < qtd; i++) {
        membros[i].offset = offset_inicial + pos_buffer;
        membros[i].ordem = i;
        pos_buffer += membros[i].tamanho_disco;
    }

    // Gravar diretório atualizado
    rewind(archive);
    if (salvar_diretorio(archive, membros, qtd) != 0) {
        fprintf(stderr, "Erro ao salvar diretório.\n");
        free(buffer);
        return -1;
    }

    // Gravar os dados reorganizados
    pos_buffer = 0;
    for (int i = 0; i < qtd; i++) {
        fseek(archive, membros[i].offset, SEEK_SET);
        size_t escritos = fwrite(buffer + pos_buffer, 1, membros[i].tamanho_disco, archive);
        if (escritos != (size_t)membros[i].tamanho_disco) {
            fprintf(stderr, "Erro ao escrever dados do membro %s\n", membros[i].nome);
            free(buffer);
            return -1;
        }
        pos_buffer += membros[i].tamanho_disco;
    }

    free(buffer);
    return 0;
}

// Função de comparação para qsort — ordena por ordem alfabetica
int comparar_por_nome(const void *a, const void *b) {
    const membro_t *m1 = (const membro_t *)a;
    const membro_t *m2 = (const membro_t *)b;
    return strcmp(m1->nome, m2->nome);
}

// Função para ordenar os membros por ordem alfabetica
int ordenar_membros_por_nome(FILE *archive, membro_t *membros, int qtd) {
    if (!archive || !membros || qtd <= 1)
        return -1;

    // Ordena alfabeticamente pelo nome
    qsort(membros, qtd, sizeof(membro_t), comparar_por_nome);

    // Calcula o espaço total necessário
    long dados_totais = 0;
    for (int i = 0; i < qtd; i++) {
        dados_totais += membros[i].tamanho_disco;
    }

    // Cria buffer para armazenar todos os dados
    unsigned char *buffer = malloc(dados_totais);
    if (!buffer) {
        fprintf(stderr, "Erro de memória.\n");
        return -1;
    }

    // Lê os dados na ordem atual
    long pos_buffer = 0;
    for (int i = 0; i < qtd; i++) {
        fseek(archive, membros[i].offset, SEEK_SET);
        size_t lidos = fread(buffer + pos_buffer, 1, membros[i].tamanho_disco, archive);
        if (lidos != (size_t)membros[i].tamanho_disco) {
            fprintf(stderr, "Erro ao ler dados do membro %s\n", membros[i].nome);
            free(buffer);
            return -1;
        }
        pos_buffer += membros[i].tamanho_disco;
    }

    // Atualiza offsets e ordem com base na nova sequência
    pos_buffer = 0;
    for (int i = 0; i < qtd; i++) {
        membros[i].offset = sizeof(int) + qtd * sizeof(membro_t) + pos_buffer;
        membros[i].ordem = i;
        pos_buffer += membros[i].tamanho_disco;
    }

    // Regrava o diretório
    rewind(archive);
    if (salvar_diretorio(archive, membros, qtd) != 0) {
        free(buffer);
        return -1;
    }

    // Grava os dados reorganizados
    pos_buffer = 0;
    for (int i = 0; i < qtd; i++) {
        fseek(archive, membros[i].offset, SEEK_SET);
        fwrite(buffer + pos_buffer, 1, membros[i].tamanho_disco, archive);
        pos_buffer += membros[i].tamanho_disco;
    }

    free(buffer);
    return 0;
}

// Retorna o tamanho do arquivo
long tamanho_arquivo(FILE *f) {
    long pos_atual = ftell(f);
    fseek(f, 0, SEEK_END);
    long tamanho = ftell(f);
    fseek(f, pos_atual, SEEK_SET);
    return tamanho;
}
// Funcao para verificar a integridade dos arquivos
int verificar_integridade(FILE *arquivo, membro_t *membros, int qtd) {
    if (!arquivo || !membros || qtd <= 0) return -1;

    long tamanho_total = tamanho_arquivo(arquivo);
    int erros = 0;

    for (int i = 0; i < qtd; i++) {
        long fim_dado = membros[i].offset + membros[i].tamanho_disco;
        if (membros[i].offset < 0 || fim_dado > tamanho_total) {
            printf("Erro: membro '%s' com offset/tamanho inválidos (offset: %ld, tamanho: %zu, arquivo: %ld)\n",
                   membros[i].nome, membros[i].offset, membros[i].tamanho_disco, tamanho_total);
            erros++;
            continue;
        }

        // Testar leitura
        unsigned char *buffer = malloc(membros[i].tamanho_disco);
        if (!buffer) {
            fprintf(stderr, "Erro de memória ao verificar membro '%s'\n", membros[i].nome);
            return -1;
        }

        fseek(arquivo, membros[i].offset, SEEK_SET);
        size_t lidos = fread(buffer, 1, membros[i].tamanho_disco, arquivo);
        free(buffer);

        if (lidos != membros[i].tamanho_disco) {
            printf("Erro: leitura incompleta para membro '%s' (esperado: %zu, lido: %zu)\n",
                   membros[i].nome, membros[i].tamanho_disco, lidos);
            erros++;
        }
    }

    if (erros == 0) {
        printf("Arquivo íntegro: todos os membros com offsets e tamanhos válidos.\n");
    } else {
        printf("Foram encontrados %d erro(s) de integridade.\n", erros);
    }

    return erros;
}
