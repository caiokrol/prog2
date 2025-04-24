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
    printf("Data de Modificacao: %s\n", (ctime(m->data_mod))); // ref: https://petbcc.ufscar.br/timefuncoes/
    printf("Ordem no arquivo: %d\n", m->ordem);
    printf("Offset: %ld\n", m->offset);
}

void gerar_nome_temporario(char *buffer, size_t tamanho){
    
}