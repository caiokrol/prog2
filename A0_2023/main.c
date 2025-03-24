#include <stdlib.h>
#include <stdio.h>

typedef struct noticia {
    char titulo[33];
    char texto[513];

    char idade;

    struct noticia *p;
} noticia;

typedef struct fila_jornal {
    noticia *inicio;
    noticia *final;

    int tam;
} fila_jornal;

void requisita(char *titulo, char *texto){
    getchar();
    printf("\nDigite o título: ");
    fgets(titulo, 33, stdin);
    printf("Digite o texto: ");
    fgets(texto, 513, stdin);
}

fila_jornal *cria_fila() {
    fila_jornal *fila = malloc(sizeof(fila_jornal));

    fila->inicio = NULL;
    fila->final = NULL;
    fila->tam = 0;

    return fila;
}

noticia *cria_noticia() {
     noticia *novaNoticia = (noticia*) malloc(sizeof(noticia));
    if (!novaNoticia) return NULL;

    requisita(novaNoticia->titulo, novaNoticia->texto);

    novaNoticia->idade = 0;
    novaNoticia->p = NULL;

    return novaNoticia;
}

int insere_noticia(noticia *noticia, fila_jornal *fila) {
    if(!noticia || !fila) return 0;
    
    if(fila->final) fila->final->p = noticia;
    else fila->inicio = noticia;
    
    fila->final = noticia;
    fila->tam++;

    return 1;
}

noticia *remove_noticia(fila_jornal *fila) {
    if(!fila) return NULL;

    if(!fila->inicio) return NULL;

    noticia *aux = fila->inicio;
    fila->inicio = fila->inicio->p;

    if(!aux->p) fila->final = NULL;

    fila->tam--;

    return aux;
}

void atualiza_fila(fila_jornal *fila) {
    if (!fila || !(fila->inicio)) return;

    noticia *aux = fila->inicio;
    int i = 0;

    while (aux) {
        if (aux->idade == 3) i++;

        aux->idade += 1;

        aux = aux->p;
    }

    for (int j = 0; j < i; j++) free(remove_noticia(fila));
}

void *destroi_fila(fila_jornal *fila) {
    while (fila->inicio) free(remove_noticia(fila));

    free(fila);
}

void print_noticia(noticia *noticia) {
    printf("%s", noticia->titulo);
    printf("");
    printf("%s", noticia->texto);
    printf("=====================================================\n");
}

int main() {
    int opc = 0;
    int tipo;

    fila_jornal *fila_breaking = cria_fila();
    fila_jornal *fila_informe = cria_fila();

    while (opc != 3) {
        printf(" - (1) Cadastrar noticia\n");
        printf(" - (2) Fechar edicao\n");
        printf(" - (3) Sair\n");
        scanf("%d", &opc);

        if (opc == 1) {
                printf("Digite o tipo da noticia (0-breaknews | 1-informe) ");
                scanf("%d", &tipo);

                if (tipo) insere_noticia(cria_noticia(), fila_informe);
                else insere_noticia(cria_noticia(), fila_breaking);
        }

        if (opc == 2) {
            printf("");
            printf("=====================================================\n");


            if (fila_breaking->tam >= 2) {
                print_noticia(remove_noticia(fila_breaking));
                print_noticia(remove_noticia(fila_breaking));
            } else if (fila_breaking->tam && fila_informe->tam) {
                print_noticia(remove_noticia(fila_breaking));
                print_noticia(remove_noticia(fila_informe));
            } else if (fila_breaking->tam && !fila_informe->tam)
                print_noticia(remove_noticia(fila_breaking));
            else if (fila_informe->tam >= 2) {
                print_noticia(remove_noticia(fila_informe));
                print_noticia(remove_noticia(fila_informe));
            } else if (fila_informe->tam)
                print_noticia(remove_noticia(fila_informe));
            else printf("Esta edição foi pulada por falta de notícias!\n");

            atualiza_fila(fila_breaking);
            atualiza_fila(fila_informe);
        }  
    }

    return 0;
}
