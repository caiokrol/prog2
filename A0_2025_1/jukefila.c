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
    return fila;
}

void inserir_jukefila(pedido* elemento, jukefila* fila) {
    if (fila == NULL || elemento == NULL) {
        return;
    }

    if (fila->inicio == NULL) {
        fila->inicio = elemento;
        fila->final = elemento;
        elemento->anterior = NULL;
        elemento->proximo = NULL;
        return;
    }

    if (elemento->valor > fila->inicio->valor) {
        elemento->proximo = fila->inicio;
        elemento->anterior = NULL;
        fila->inicio->anterior = elemento;
        fila->inicio = elemento;
        return;
    }
    pedido* dot = fila->inicio;
    while (dot->proximo != NULL && dot->proximo->valor > elemento->valor) {
        dot = dot->proximo;
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
    if(!fila->inicio){
        return NULL;
    }
    pedido *daVez = fila->inicio;
    fila->inicio = daVez->proximo;
    if(fila->inicio != NULL) {
        fila->inicio->anterior = NULL;
    } else {
        fila->final = NULL; 
    }
    return daVez;
}

unsigned int contar_jukefila(jukefila* fila){
    pedido *dot = fila->inicio;
    int cont = 0;
    while (dot != NULL){
        cont++;
        dot = dot->proximo;
    }
    return cont;
}

void destruir_jukefila(jukefila *fila){
    if(!fila) {
        return;
    }

    while (fila->inicio)
    {
        pedido *aux = fila->inicio;
        fila->inicio = fila->inicio->proximo;
        if(!fila->inicio){ 
            fila->final = NULL;
        }
        free(aux); 
    }
    free(fila);
}
