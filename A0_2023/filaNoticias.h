#ifndef listaNoticias
#define listaNoticias

#include <stdlib.h>
#include <stdio.h>

// estrutura de um item da lista
typedef struct noticia{
  char titulo[33];			// valor do item
  char texto[513];		// item anterior
  int idade;	
  struct noticia *prox;
  struct noticia *ant;
  // próximo item
} noticia;

// estrutura de uma lista
typedef struct lista{
  struct noticia *prim ;	// primeiro item
  struct noticia *ult ;		// último item
  int tamanho ;		// número de itens da lista
} lista;

struct noticia * cria_noticia();

struct lista * lista_cria();

int lista_destroi();

int remove_noticia();

int insere_noticia();

void atualiza_lista();

#endif