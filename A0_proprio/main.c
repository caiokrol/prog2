#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct musica {
    char autor[50];
    char titulo[50];
    int duracao;
};

struct fpnodo_t {
    struct musica musica;
    int prio;
    struct fpnodo_t *prox;
};

struct fprio_t {
    struct fpnodo_t *prim;  // primeiro nodo da fila
    int num;                 // número de itens na fila
};

int aleat(int min, int max) {
  return (rand() % (max - min + 1)) + min;
}

void requisita(char *autor, char *titulo){
getchar();
printf("\nDigite o Autor: ");
fgets(autor, 50, stdin);
printf("Digite o titulo: ");
fgets(titulo, 50, stdin);
}

struct fprio_t *cria_fila() {
    // Aloca memória para a estrutura da fila de prioridades
    struct fprio_t *fila = (struct fprio_t *)malloc(sizeof(struct fprio_t));
    if (fila == NULL) {
        // Retorna NULL se a alocação de memória falhar
        return NULL;
    }
    // Inicializa os campos da estrutura
    fila->prim = NULL;  // A fila começa vazia, sem nós
    fila->num = 0;      // Número de itens é zero

    return fila;        // Retorna o ponteiro para a fila criada
}

struct fprio_t *fprio_destroi(struct fprio_t *fila) {
    if (fila == NULL) {
        return NULL;  // Retorna NULL se a fila já for NULL
    }

    // Ponteiro auxiliar para iterar pelos nós
    struct fpnodo_t *atual = fila->prim;
    while (atual != NULL) {
        struct fpnodo_t *prox = atual->prox;  // Armazena o próximo nodo
        free(atual);  // Libera o nodo
        atual = prox; // Move para o próximo nodo
    }

    // Libera a estrutura da fila
    free(fila);

    return NULL;  // Retorna NULL conforme especificado
}

int fprio_insere(struct fprio_t *fila, struct musica *musica, int prio) {
    if (fila == NULL) {
        return -1;  // Erro se a fila for NULL
    }

    // Cria um novo nodo
    struct fpnodo_t *novo = (struct fpnodo_t *)malloc(sizeof(struct fpnodo_t));
    if (novo == NULL) {
        return -1;  // Erro se não conseguir alocar memória para o novo nodo
    }

    // Inicializa o novo nodo
    novo->musica = *musica;
    novo->prio = prio;
    novo->prox = NULL;

    // Caso especial: a fila está vazia ou o novo nodo tem a menor prioridade
    if (fila->prim == NULL || prio < fila->prim->prio) {
        novo->prox = fila->prim;  // O novo nodo se torna o primeiro
        fila->prim = novo;
    } else {
        // Percorre a fila para encontrar a posição correta
        struct fpnodo_t *atual = fila->prim;
        while (atual->prox != NULL && atual->prox->prio <= prio) {
            atual = atual->prox;
        }

        // Insere o novo nodo na posição correta
        novo->prox = atual->prox;
        atual->prox = novo;
    }

    // Incrementa o número de itens na fila
    fila->num++;

    return fila->num;  // Retorna o número de itens na fila
}

struct musica *fprio_retira(struct fprio_t *f, int *prio) {
    if (f == NULL || f->prim == NULL) {
        return NULL;  // Retorna NULL se a fila for NULL ou vazia
    }

    // Ponteiro para o primeiro nodo da fila
    struct fpnodo_t *primeiro = f->prim;

    // Atualiza o tipo e a prioridade
    if (prio != NULL) {
        *prio = primeiro->prio;
    }

    // Obtém o item e remove o nodo da fila
    struct musica *musica = (struct musica *)malloc(sizeof(struct musica));
    *musica = primeiro->musica;
    f->prim = primeiro->prox;
    free(primeiro);
    f->num--;

    return musica;
}

int fprio_tamanho(struct fprio_t *f) {
    if (f == NULL) {
        return -1;  // Retorna -1 se a fila for NULL (erro)
    }

    return f->num;  // Retorna o número de itens na fila
}

int fprio_imprime(struct fprio_t *f) {
    if (f == NULL || f->prim == NULL) {
        return 0;  // Não imprime nada se a fila for NULL ou estiver vazia
    }

    struct fpnodo_t *atual = f->prim;
    int primeiro = 1;  // Flag para controlar o espaço entre os itens
    int pos = 1;
    while (atual != NULL) {
        if (!primeiro) {
            printf(" ");  // Imprime espaço antes dos itens seguintes
        }
        printf("(%d. %s %s)",pos , atual->musica.autor, atual->musica.titulo);
        primeiro = 0;  // Depois do primeiro item, o flag é desativado
        atual = atual->prox;  // Avança para o próximo nodo
        pos++;
    }

    return 1;  // Retorna 1 para indicar que a impressão foi bem-sucedida
}

struct musica *adicionar_musica() {
    struct musica *novaMusica = (struct musica *)malloc(sizeof(struct musica));
    if (!novaMusica) {
        return NULL;
    }

    requisita(novaMusica->autor, novaMusica->titulo);
    novaMusica->duracao = aleat(100, 600);
    return novaMusica;
}

int main() {
    struct fprio_t *filaDeMusicas = cria_fila();
    int prioridade = 0;

    while (1) {
        printf("Qual sera a prioridade da musica? de 0 a R$19.999\n");
        scanf("%d", &prioridade);

        fprio_insere(filaDeMusicas, adicionar_musica(), (20000 - prioridade));

        struct musica *atual = fprio_retira(filaDeMusicas, &prioridade);
        if (atual) {
            printf("\nMusica atual: %sAutor: %s", atual->titulo, atual->autor);
        }
        
        printf("\nProximas musicas:\n");
        if (!fprio_imprime(filaDeMusicas)) {
            printf("Fila Vazia\n\n");
        }
        sleep(atual->duracao);
    }

    return 0;
}
