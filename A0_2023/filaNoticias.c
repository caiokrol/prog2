#include <stdlib.h>
#include <stdio.h>
#include "filaNoticias.h"


void requisita(char *titulo, char *texto){
getchar();
printf("\nDigite o título: ");
fgets(titulo, 33, stdin);
printf("Digite o texto: ");
fgets(texto, 513, stdin);
}

struct noticia * cria_noticia(){
    noticia *novaNoticia = (noticia*) malloc(sizeof(noticia));
    if(!novaNoticia){
        return;
    }
    novaNoticia->idade = 0;
    novaNoticia->prox = NULL;
    novaNoticia->ant = NULL;

    return novaNoticia;
};


struct lista * lista_cria(){
    lista *listaNova = (lista*) malloc(sizeof(lista));
    if(!listaNova){
        return;
    }
    listaNova->prim = NULL;
    listaNova->ult = NULL;
    listaNova->tamanho = 0;
};

int lista_destroi(){
    
}
