#ifndef DIR_H
#define DIR_H

#include <stdio.h>
#include <time.h>

typedef struct {
    char name[1024];         // nome do membro (sem espaços)
    __uid_t uid;                // identificador único
    size_t size_orig;    // tamanho original
    size_t size_disk;   // tamanho após compressão (ou igual se não comprimido)
    time_t data_mod;        // data de modificação
    int order;              // ordem no arquivo
    long offset;            // onde os dados começam no archive
} member_t;

int load_dir(FILE *archive, member_t **members, int *qtt);
int save_dir(FILE *archive, member_t *members, int qtt);
int read_dir(FILE *archive, member_t ***members, int *size);
int print_dir(FILE *archive);
size_t seek_member(member_t *members, int qtt, const char *name);
int generate_uid(member_t *members, int qtt);
int compare_order(const void *a, const void *b);

#endif
