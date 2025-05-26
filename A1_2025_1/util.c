#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include "diretorio.h"

time_t obter_data_mod(const char *caminho) { // ref: https://pubs.opengroup.org/onlinepubs/7908799/xsh/sysstat.h.html
    struct stat arq;
    if (stat(caminho, &arq) == 0)
        return arq.st_mtime; // retorna um time_t com a data da ultima modificação ref: https://pubs.opengroup.org/onlinepubs/7908799/xsh/stat.html
    else
        return (time_t)-1; // erro ao obter
}

void printar_info(const membro_t *m) {
    printf("Nome: %s\n", m->nome);
    printf("ID: %d\n", m->uid);
    printf("Tamanho original: %zu KB\n", (m->tamanho_orig / 1024));
    printf("Tamanho em disco: %zu KB\n", (m->tamanho_disco / 1024));
    printf("Data de Modificacao: %s", asctime(localtime(&m->data_mod))); // ref: https://petbcc.ufscar.br/timefuncoes/
    printf("Ordem no arquivo: %d\n", m->ordem);
    printf("Offset: %ld\n", m->offset);
}

void imprimir_ajuda(const char *nome_prog) {
    printf("Uso: %s <opcao> <arquivo> [membros...]\n", nome_prog);
    printf("Opções disponíveis:\n");
    printf("  -ip  <arquivo> [membros...]  Inserir membros no arquivo (sem compressão)\n");
    printf("  -ic  <arquivo> [membros...]  Inserir membros no arquivo (com compressão)\n");
    printf("  -x   <arquivo> [membros...]  Extrair membros do arquivo (extrai todos se não informar membros)\n");
    printf("  -r   <arquivo> [membros...]  Remover membros do arquivo\n");
    printf("  -m   <arquivo> <orig> <dest> Mover membro 'orig' após 'dest'\n");
    printf("  -c   <arquivo>               Listar conteúdo do arquivo (membros)\n");
    printf("  -t   <arquivo>               Inspecionar membros (detalhes)\n");
    printf("  -ss  <arquivo>               Ordenar membros por tamanho\n");
    printf("  -sn  <arquivo>               Ordenar membros por nome\n");
    printf("  -v   <arquivo>               Verificar integridade do arquivo\n");
    printf("  -cbt <arquivo> <tamanho>     Listar conteúdo do arquivo (membros) listando somente membros maiores que tamanho\n");
    printf("  -cst <arquivo> <tamanho>     Listar conteúdo do arquivo (membros) listando somente membros menores que tamanho\n");
    printf("  -help                        Mostrar esta mensagem de ajuda\n");
}

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

void listar_membros_filtrados(membro_t *m, int qtd, size_t tamanho_min, size_t tamanho_max) {
    printf("%-20s %-5s %-10s %-10s %-20s %-6s %-8s\n",
        "Nome", "UID", "TamOrig", "TamDisco", "DataMod", "Ordem", "Offset");
    for (int i = 0; i < qtd; i++) {
        if (m[i].tamanho_orig >= tamanho_min && m[i].tamanho_orig <= tamanho_max) {
            char data[64];
            strftime(data, sizeof(data), "%d/%m/%Y %H:%M:%S", localtime(&m[i].data_mod));
            printf("%-20s %-5d %-10zu %-10zu %-20s %-6d %-8ld\n",
                m[i].nome, m[i].uid, m[i].tamanho_orig,
                m[i].tamanho_disco, data, m[i].ordem, m[i].offset);
        }
    }
}
