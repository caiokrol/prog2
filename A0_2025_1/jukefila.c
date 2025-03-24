#include <stdio.h>
#include <stdlib.h>
#include "jukefila.h"
#include "pedido.h"

jukefila *criar_jukefila(){
    jukefila *fila = (jukefila*) malloc(sizeof(jukefila));
    if(!fila){
        return NULL;
    }
    fila->inicio = NULL;
    fila->final = NULL;
}

void inserir_jukefila(pedido* elemento, jukefila* fila) {
    if (fila == NULL || elemento == NULL) {
        return;
    }
    if (!fila->inicio) {
        fila->inicio = elemento;
        fila->final = elemento;
        elemento->anterior = NULL;
        elemento->proximo = NULL;
        return;
    }

    pedido* dot = fila->inicio;

    while (dot->proximo && dot->valor >= elemento->valor) {
        dot = dot->proximo;
    }

    if (dot == fila->inicio && dot->valor < elemento->valor) {
        elemento->proximo = dot;
        elemento->anterior = NULL;
        dot->anterior = elemento;
        fila->inicio = elemento;
        return;
    } 
    pedido* aux = dot->proximo;
    dot->proximo = elemento;
    elemento->anterior = dot;
    elemento->proximo = aux;

    if (aux != NULL) {
        aux->anterior = elemento;
    } else {
        fila->final = elemento;
    }
}


pedido* consumir_jukefila(jukefila* fila){
    printf("ta chamando");
    if(!fila->inicio){
        return NULL;
    }
    pedido *daVez = fila->inicio;
    fila->inicio = daVez->proximo;
    daVez->proximo->anterior = NULL;

    if (daVez == fila->final){
        fila->inicio == NULL;
        fila->final == NULL;
    }

    return daVez;
}

unsigned int contar_jukefila(jukefila* fila){
    pedido *dot = fila->inicio;
    int cont = 0;
    while (dot->proximo){
        cont++;
    }
}

void destruir_jukefila(jukefila *fila){
    if(!fila) {
        return;
    }

    while (fila->inicio)
    {
        pedido *aux = fila->inicio;
        fila->inicio = fila->inicio->proximo;
        if(!aux->proximo){
            fila->final = NULL;
        }
        free(aux);
    }
    free(fila);
}
