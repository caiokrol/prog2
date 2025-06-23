#ifndef PROTAGONISTA_H
#define PROTAGONISTA_H

#include <allegro5/allegro.h>
#include <stdbool.h>

// NOVO: Definindo MAX_GARRAFAS para uso em protagonista.c
#define MAX_GARRAFAS 5

// --- Estrutura para propriedades das garrafas ---
typedef struct {
    float x, y;
    float vel_x;
    float angulo;
    bool ativa;
} Garrafa;

// --- Enumeração dos Estados do Protagonista ---
typedef enum {
    PROT_NORMAL,
    PROT_MORRENDO
} ProtagonistState;

// --- Estrutura de Dados do Protagonista ---
typedef struct {
    // Posição e Física
    float x, y;
    float y_base;
    float y_chao_atual;
    float vel_y;
    float vel_x_pulo;
    float velocidade_atual; // Para acompanhar se está andando ou correndo

    // Estados de Ação
    bool pulando;
    bool agachando;
    bool animacao_agachar_finalizada; // Flag para a animação de agachar ter terminado
    bool atacando;
    bool arremessando;
    bool especial_ativo;
    bool especial_finalizado;

    // Atributos de Combate e Status
    ProtagonistState estado;
    int vida;
    bool invulneravel;
    float timer_invulnerabilidade;
    bool visivel;
    float timer_piscar;
    int dinheiro;

    // Animação
    int frame_atual;
    float acc_animacao;
    bool animacao_morte_completa; // NOVO: Flag para a animação de morte ter terminado

    // Ponteiros para os sprites (carregados na função principal)
    ALLEGRO_BITMAP *sprite_parado;
    ALLEGRO_BITMAP *sprite_andando;
    ALLEGRO_BITMAP *sprite_correndo;
    ALLEGRO_BITMAP *sprite_pulando;
    ALLEGRO_BITMAP *sprite_agachado;
    ALLEGRO_BITMAP *sprite_especial;
    ALLEGRO_BITMAP *sprite_ataque1;
    ALLEGRO_BITMAP *sprite_arremessando;
    ALLEGRO_BITMAP *sprite_morte;

    // Dimensões dos frames para não precisar recalcular
    int frame_h; // Altura padrão de todos os frames
    // Correções de declaração: cada um em sua própria declaração ou separados por vírgula corretamente.
    int frame_w_parado;
    int frame_w_andando;
    int frame_w_correndo;
    int frame_w_pulando;
    int frame_w_agachado;
    int frame_w_especial;
    int frame_w_ataque1;
    int frame_w_arremessando; // AGORA DECLARADO CORRETAMENTE
    int frame_w_morte;

    // Contagem de frames para as animações
    // Correções de declaração
    int frame_total_parado;
    int frame_total_andando;
    int frame_total_correndo;
    int frame_total_pulando;
    int frame_total_agachado;
    int frame_total_especial;
    int frame_total_ataque1;
    int frame_total_arremessando; // AGORA DECLARADO CORRETAMENTE
    int frame_total_morte;

    float escala_sprites; // Escala dos sprites do protagonista

    // Propriedades da Hitbox do Protagonista
    float prot_hitbox_offset_x;
    float prot_hitbox_offset_y;
    float prot_hitbox_width;
    float prot_hitbox_height;
    float prot_crouch_hitbox_height;

    // Propriedades da Hitbox de ATAQUE do Protagonista
    float prot_attack_hitbox_offset_x;
    float prot_attack_hitbox_offset_y;
    float prot_attack_hitbox_width;
    float prot_attack_hitbox_height;

} Protagonista;


// --- Funções Públicas do Módulo ---

void protagonista_inicializar(Protagonista *prot, ALLEGRO_BITMAP *sprites[]);

void protagonista_processar_evento(Protagonista *prot, ALLEGRO_EVENT *ev, Garrafa garrafas[], float deslocamento_x);

void protagonista_atualizar(Protagonista *prot, ALLEGRO_KEYBOARD_STATE *teclado, float *deslocamento_x, float velocidade_andar, float velocidade_correr);

void protagonista_desenhar(Protagonista *prot);

#endif // PROTAGONISTA_H