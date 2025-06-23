// inimigo.h

#ifndef INIMIGO_H
#define INIMIGO_H

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <stdbool.h>

// --- CONSTANTES GLOBAIS DO MÓDULO ---
#define MAX_INIMIGOS 10 // Número máximo de inimigos que podem existir no jogo ao mesmo tempo

// --- TIPOS E ESTRUTURAS ---

// Define os possíveis estados de um inimigo para organizar a lógica
typedef enum {
    INIMIGO_ANDANDO,
    INIMIGO_PARADO,
    INIMIGO_MORRENDO,
    INIMIGO_MORTO
} InimigoEstado;

// Estrutura que guarda todas as informações de uma instância de inimigo
typedef struct {
    // Propriedades de Jogo
    float x;
    float y;
    float vel_x;
    float vel_y;
    int vida;
    int direcao; // -1 para esquerda, 1 para direita
    bool ativo;  // O inimigo está sendo usado no jogo?
    InimigoEstado estado;

    // Propriedades da Hitbox (a causa do seu erro original)
    float hb_offset_x; // Deslocamento da hitbox em relação à posição (x) do sprite
    float hb_offset_y; // Deslocamento da hitbox em relação à posição (y) do sprite
    float hb_w;        // Largura (width) da hitbox
    float hb_h;        // Altura (height) da hitbox

    // Propriedades de Animação
    ALLEGRO_BITMAP *sprite_atual;
    int frame_w;             // Largura de um único frame do sprite
    int frame_h;             // Altura de um único frame do sprite
    int frame_atual;         // O frame atual da animação (0, 1, 2...)
    int frame_total;         // Quantidade total de frames na animação
    float tempo_por_frame;   // Velocidade da animação (ex: 1.0 / 10.0 para 10 FPS)
    float acumulador_anim;   // Acumula o tempo para saber quando trocar de frame

    // Propriedades da IA de Patrulha
    float patrol_start_x;
    float patrol_end_x;

} Inimigo;


// --- PROTÓTIPOS DAS FUNÇÕES PÚBLICAS ---
// (Funções que podem ser chamadas a partir do main.c)

/**
 * @brief Carrega os sprites dos inimigos na memória. Deve ser chamada uma vez na inicialização.
 * @return true se os sprites foram carregados com sucesso, false caso contrário.
 */
bool inimigos_carregar_sprites();

/**
 * @brief Libera os sprites dos inimigos da memória. Deve ser chamada no final do jogo.
 */
void inimigos_destruir_sprites();

/**
 * @brief Inicializa o array de inimigos, posicionando alguns e definindo suas propriedades.
 * @param inimigos O array de inimigos a ser inicializado.
 */
void inimigos_inicializar(Inimigo inimigos[]);

/**
 * @brief Atualiza a lógica de todos os inimigos ativos (movimento, IA, física, animação).
 * @param inimigos O array de inimigos.
 * @param y_chao A posição Y do chão, para aplicar a gravidade corretamente.
 */
void inimigos_atualizar(Inimigo inimigos[], float y_chao);

/**
 * @brief Desenha todos os inimigos ativos na tela.
 * @param inimigos O array de inimigos.
 */
void inimigos_desenhar(Inimigo inimigos[]);

/**
 * @brief Aplica dano a um inimigo específico.
 * @param inimigo Ponteiro para o inimigo que receberá o dano.
 * @param dano A quantidade de dano a ser aplicada.
 */
void inimigo_receber_dano(Inimigo *inimigo, int dano);


#endif // INIMIGO_H