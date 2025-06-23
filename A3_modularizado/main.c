#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_video.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_primitives.h>
#include <math.h>
#include <time.h> // Necessário para srand(time(NULL))

// Inclui o cabeçalho do protagonista, que contém a definição da struct Protagonista e suas funções
#include "protagonista.h"
#include "inimigo.h"
// --- Novas Definições para o Chefe ---
#define BOSS_SPAWN_X 15000.0f
#define BOSS_INTERACTION_DISTANCE 200.0f

#define BOSS_PROSTITUTA_PARADA_FRAME_COUNT 5
#define BOSS_PROSTITUTA_TRANS1_FRAME_COUNT 8
#define BOSS_DEMON_TRANS2_FRAME_COUNT 4
#define BOSS_DEMON_IDLE_FRAME_COUNT 5
// --- NOVO: Definições de combate do chefe ---
#define BOSS_DEMON_ATTACK_FRAME_COUNT 7
#define BOSS_DEMON_DANO_FRAME_COUNT 3
#define BOSS_DEMON_MORTE_FRAME_COUNT 4
#define MAX_BOSS_HEALTH 500 // Vida máxima do chefe
#define BOSS_ATTACK_COOLDOWN 3.0 // Tempo em segundos entre os ataques
#define BOSS_PROJECTILE_DAMAGE 25 // Dano do projétil do chefe
#define PLAYER_ATTACK_DAMAGE_TO_BOSS 35 // Dano que o jogador causa ao chefe

// --- ALTERADO: Enumeração dos Estados do Chefe ---
typedef enum {
    BOSS_INVISIVEL,      // O chefe ainda não apareceu
    BOSS_PARADA,         // A forma de prostituta, parada
    BOSS_TRANSFORMANDO,  // A primeira animação de transformação
    BOSS_TRANSFORMANDO_2, // A segunda animação de transformação
    BOSS_DEMONIO_IDLE,   // O chefe transformado, em modo de espera
    // --- NOVOS ESTADOS DE COMBATE ---
    BOSS_DEMONIO_ATACANDO,
    BOSS_DEMONIO_DANO,
    BOSS_DEMONIO_MORRENDO
} BossState;

// --- Estrutura para a Entrada do Chefe ---
typedef struct {
    float x, y;
    ALLEGRO_BITMAP *sprite_bitmap;
    float width, height;
} BossEntrance;

// --- NOVO: Estrutura para os Projéteis do Chefe ---
#define MAX_BOSS_PROJECTILES 5
typedef struct {
    float x, y;
    float vel_x;
    float angulo;
    bool ativa;
} BossProjectile;


// --- ALTERADO: Estrutura para o Chefe ---
typedef struct {
    float x, y;
    bool ativa;
    BossState estado;
    int frame_atual;
    float acc_animacao;
    float escala;

    // --- NOVO: Atributos de Combate ---
    int health;
    float attack_cooldown;
    float invulnerability_timer;

    // Sprites para as diferentes formas e animações
    ALLEGRO_BITMAP *sprite_parada;
    ALLEGRO_BITMAP *sprite_trans1;
    ALLEGRO_BITMAP *sprite_trans2;
    ALLEGRO_BITMAP *sprite_demonio_idle;
    // --- NOVOS SPRITES ---
    ALLEGRO_BITMAP *sprite_demonio_ataque;
    ALLEGRO_BITMAP *sprite_demonio_dano;
    ALLEGRO_BITMAP *sprite_demonio_morte;

    // Propriedades de animação
    int frame_largura_parada;
    int frame_altura_parada;
    int frame_largura_trans1;
    int frame_altura_trans1;
    int frame_largura_trans2;
    int frame_altura_trans2;
    int frame_largura_demonio_idle;
    int frame_altura_demonio_idle;
    // --- NOVAS PROPRIEDADES ---
    int frame_largura_demonio_ataque;
    int frame_altura_demonio_ataque;
    int frame_largura_demonio_dano;
    int frame_altura_demonio_dano;
    int frame_largura_demonio_morte;
    int frame_altura_demonio_morte;
} Boss;


// --- Constantes da Janela ---
#define LARGURA 1536
#define ALTURA 864

#define NUM_OPCOES 3 // Não usado diretamente na função jogo, mas mantido

const int VIDEO_LARGURA = 1920;
const int VIDEO_ALTURA = 1080;

// --- Estrutura para propriedades das garrafas ---
// Já definida em protagonista.h, mas incluída aqui para clareza no contexto do main.c
// typedef struct {
//     float x, y;
//     float vel_x;
//     float angulo;
//     bool ativa;
// } Garrafa;

// --- Estrutura para a granada ---
typedef struct {
    float x, y;
    float vel_x, vel_y;
    bool ativa;
    bool explodindo;
    int frame_explosao;
    float acc_animacao;
    bool dano_aplicado;
} Granada;

// --- Define número máximo de garrafas e granadas ---
#define MAX_GARRAFAS 5
#define MAX_GRANADAS 10 // Pool de granadas

// --- Enumeração dos Estados do Inimigo ---
typedef enum {
    INIMIGO_PARADO,
    INIMIGO_ANDANDO,
    INIMIGO_ATACANDO,
    INIMIGO_MORRENDO
} EnemyState;

// --- Enumeração dos Tipos de Inimigo ---
typedef enum {
    NOIA,
    POLICIAL
} InimigoType;

// --- Enumeração dos Estados do Protagonista ---
// Já definida em protagonista.h
// typedef enum {
//     PROT_NORMAL,  // Estado normal (parado, andando, pulando, agachando, atacando, arremessando)
//     PROT_MORRENDO // Estado de morte do protagonista
// } ProtagonistState;

// Estrutura para propriedades do inimigo (ATUALIZADA)
typedef struct {
    float x, y;  // Posição do inimigo no MUNDO (não na TELA)
    float vel_x;   // Velocidade horizontal RELATIVA AO MUNDO
    int frame_atual;
    float acc_animacao;
    bool ativa;
    EnemyState estado;
    bool animacao_morte_finalizada; // Flag para saber se a animação de morte foi concluída
    float hitbox_offset_x;
    float hitbox_offset_y;
    float hitbox_width;
    float hitbox_height;
    bool inimigo_pode_dar_dano; // Controla se o inimigo pode dar dano neste momento
    InimigoType type; // NOVO: Tipo de inimigo (NOIA ou POLICIAL)
    float attack_cooldown; // NOVO: Cooldown para ataques
} Inimigo;

// --- Define número máximo de inimigos ---
#define MAX_INIMIGOS 5 // Aumentado para mais variedade

// --- Definições de Vida do Protagonista ---
#define MAX_PROTAGONIST_HEALTH 100
#define PROTAGONIST_DAMAGE_PER_HIT 20 // Dano do Noia
#define GRENADE_DAMAGE 20 // Dano da granada
// --- Definições de Invulnerabilidade ---
#define PROTAGONIST_INVULNERABILITY_DURATION 1.0 // 1 segundo de invulnerabilidade após levar dano
#define PROTAGONIST_BLINK_INTERVAL 0.1 // Pisca a cada 0.1 segundos durante a invulnerabilidade

// --- Estrutura NPC ---
typedef struct {
    float x, y; // Coordenadas no mundo
    int frame_atual;
    float acc_animacao;
    bool ativa; // Sempre true para um NPC permanente
    int largura_sprite;
    int altura_sprite;
} NPC;

// --- Definições NPC ---
#define NPC_TRAFICANTE_FRAME_COUNT 7
#define NPC_TRAFICANTE_TPF (1.0 / 8.0) // Velocidade de animação do NPC
#define NPC_HEAL_AMOUNT 20 // Quantidade de HP restaurada
#define NPC_INTERACTION_DISTANCE 100.0 // Distância para interagir com o NPC
#define NPC_HEAL_COST 20 // Custo para comprar do NPC

// --- Definições de Fade de Áudio ---
#define AUDIO_MAX_DISTANCE 600.0 // Distância máxima em pixels para o som ser audível
#define AUDIO_MIN_DISTANCE 50.0  // Distância mínima em pixels para volume total

// --- ESTRUTURA ATUALIZADA: Obstáculo para incluir bitmap do sprite ---
typedef struct {
    float x, y; // Coordenadas do mundo (não da tela)
    float width, height; // Dimensões da hitbox
    bool ativa;
    ALLEGRO_BITMAP *sprite_bitmap; // Ponteiro para o bitmap real
    bool only_crouch_pass; // Flag para obstáculos que só podem ser passados agachado
} Obstaculo;

// --- ATUALIZADO: Define número máximo de obstáculos ---
#define MAX_OBSTACULOS 40

// --- Estrutura de Nota de Dinheiro ---
typedef struct {
    float x, y;
    int value; // 2, 5, ou 10
    ALLEGRO_BITMAP *sprite_bitmap;
    bool ativa;
    float hitbox_width;
    float hitbox_height;
    float spawn_time; // Tempo em que a nota foi spawnada
} MoneyNote;

// --- Definições de Nota de Dinheiro ---
#define MAX_MONEY_NOTES 5
#define MONEY_NOTE_SPAWN_INTERVAL 5.0 // Tempo em segundos entre spawns
#define MONEY_NOTE_LIFETIME 10.0 // Tempo em segundos antes de uma nota desaparecer
#define MONEY_NOTE_SCALE 0.3 // Escala para sprites de notas de dinheiro

// --- NOVO: Variável global para rastrear se o chefe foi permanentemente derrotado ---
bool boss_defeated_permanently = false;


// --- Funções Auxiliares ---

// Função para esperar uma tecla (útil para menus)
void esperar_tecla(ALLEGRO_EVENT_QUEUE *fila) {
    ALLEGRO_EVENT ev;
    while (1) {
        al_wait_for_event(fila, &ev);
        if (ev.type == ALLEGRO_EVENT_KEY_DOWN) break;
    }
}

// Função para desenhar o vídeo redimensionado e centralizado mantendo a proporção (não usada na função jogo, mas mantida)
void desenhar_video_proporcional(ALLEGRO_BITMAP *frame) {
    if (!frame) return;

    float proporcao_janela = (float)LARGURA / ALTURA;
    float proporcao_video = (float)VIDEO_LARGURA / VIDEO_ALTURA;

    float dw, dh, dx, dy;

    if (proporcao_janela > proporcao_video) {
        // Janela é mais larga, a altura é limitante
        dh = ALTURA;
        dw = dh * proporcao_video;
    } else {
        // Janela é mais alta ou igual, a largura é limitante
        dw = LARGURA;
        dh = dw / proporcao_video;
    }

    dx = (LARGURA - dw) / 2.0;
    dy = (ALTURA - dh) / 2.0;

    al_draw_scaled_bitmap(frame,
                         0, 0,
                         VIDEO_LARGURA, VIDEO_ALTURA,
                         dx, dy,
                         dw, dh,
                         0);
}

// Função para verificar colisão entre dois retângulos
bool check_collision(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2) {
    return x1 < x2 + w2 &&
           x1 + w1 > x2 &&
           y1 < y2 + h2 &&
           y1 + h1 > y2;
}

// --- NOVO: Função da Tela de Vitória ---
// Retorna true se o jogador escolher tentar novamente, false se escolher sair
bool victory_screen(ALLEGRO_DISPLAY *janela, ALLEGRO_EVENT_QUEUE *fila, ALLEGRO_FONT *fonte) {
    ALLEGRO_BITMAP *victory_image = al_load_bitmap("vitoria.png"); // Certifique-se de ter esta imagem
    if (!victory_image) {
        fprintf(stderr, "ERRO: Não foi possível carregar vitoria.png\n");
        return false;
    }

    ALLEGRO_BITMAP *background_victory = al_load_bitmap("backgroundVitoria.png"); // Reutilizando o background
    if (!background_victory) {
        fprintf(stderr, "ERRO: Não foi possível carregar backgroundVitoria.png\n");
        al_destroy_bitmap(victory_image);
        return false;
    }

    const char *opcoes_victory[] = {
        "Jogar Novamente",
        "Sair do Jogo"
    };
    int num_opcoes_victory = sizeof(opcoes_victory) / sizeof(opcoes_victory[0]);
    int opcao_selecionada_v = 0;

    bool sair_victory_screen = false;
    bool restart_game = false;

    while (!sair_victory_screen) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(fila, &ev);

        if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            switch (ev.keyboard.keycode) {
                case ALLEGRO_KEY_UP:
                    opcao_selecionada_v--;
                    if (opcao_selecionada_v < 0) opcao_selecionada_v = num_opcoes_victory - 1;
                    break;
                case ALLEGRO_KEY_DOWN:
                    opcao_selecionada_v++;
                    if (opcao_selecionada_v >= num_opcoes_victory) opcao_selecionada_v = 0;
                    break;
                case ALLEGRO_KEY_ENTER:
                    if (opcao_selecionada_v == 0) { // Jogar Novamente
                        restart_game = true;
                    } else { // Sair do Jogo
                        restart_game = false;
                    }
                    sair_victory_screen = true;
                    break;
                case ALLEGRO_KEY_ESCAPE:
                    restart_game = false;
                    sair_victory_screen = true;
                    break;
            }
        } else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            restart_game = false;
            sair_victory_screen = true;
        }

        al_draw_scaled_bitmap(background_victory, 0, 0,
                              al_get_bitmap_width(background_victory), al_get_bitmap_height(background_victory),
                              0, 0, LARGURA, ALTURA, 0);

        al_draw_scaled_bitmap(victory_image, 0, 0,
                              al_get_bitmap_width(victory_image), al_get_bitmap_height(victory_image),
                              (LARGURA - al_get_bitmap_width(victory_image)) / 2.0,
                              (ALTURA / 2.0 - al_get_bitmap_height(victory_image) / 2.0) - 50,
                              al_get_bitmap_width(victory_image), al_get_bitmap_height(victory_image), 0);
        
        al_draw_text(fonte, al_map_rgb(128, 0, 128), LARGURA / 2, ALTURA / 2 + 50, ALLEGRO_ALIGN_CENTER, "VOCÊ CHEGOU AO PARAÍSO DE MAOMÉ!");


        int start_y = ALTURA / 2 + 130;
        for (int i = 0; i < num_opcoes_victory; i++) {
            ALLEGRO_COLOR cor_texto = (i == opcao_selecionada_v) ? al_map_rgb(128, 0, 128) : al_map_rgb(255, 255, 255);
            al_draw_text(fonte, cor_texto, LARGURA / 2, start_y + i * al_get_font_line_height(fonte) * 1.5, ALLEGRO_ALIGN_CENTER, opcoes_victory[i]);
        }

        al_flip_display();
    }

    al_destroy_bitmap(victory_image);
    al_destroy_bitmap(background_victory);
    return restart_game;
}


// --- Função da Tela de Game Over ---
// Retorna true se o jogador escolher tentar novamente, false se escolher sair
bool game_over_screen(ALLEGRO_DISPLAY *janela, ALLEGRO_EVENT_QUEUE *fila, ALLEGRO_FONT *fonte) {
    ALLEGRO_BITMAP *game_over_image = al_load_bitmap("gameover.png");
    if (!game_over_image) {
        fprintf(stderr, "ERRO: Não foi possível carregar gameover.png\n");
        return false;
    }

    ALLEGRO_BITMAP *background_game_over = al_load_bitmap("backgroundGameOver.png");
    if (!background_game_over) {
        fprintf(stderr, "ERRO: Não foi possível carregar backgroundGameOver.png\n");
        al_destroy_bitmap(game_over_image);
        return false;
    }

    const char *opcoes_game_over[] = {
        "Tentar novamente",
        "Sair do Jogo"
    };
    int num_opcoes_game_over = sizeof(opcoes_game_over) / sizeof(opcoes_game_over[0]);
    int opcao_selecionada_go = 0;

    bool sair_game_over_screen = false;
    bool restart_game = false;

    while (!sair_game_over_screen) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(fila, &ev);

        if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            switch (ev.keyboard.keycode) {
                case ALLEGRO_KEY_UP:
                    opcao_selecionada_go--;
                    if (opcao_selecionada_go < 0) opcao_selecionada_go = num_opcoes_game_over - 1;
                    break;
                case ALLEGRO_KEY_DOWN:
                    opcao_selecionada_go++;
                    if (opcao_selecionada_go >= num_opcoes_game_over) opcao_selecionada_go = 0;
                    break;
                case ALLEGRO_KEY_ENTER:
                    if (opcao_selecionada_go == 0) { // Tentar novamente
                        restart_game = true;
                    } else { // Sair do Jogo
                        restart_game = false;
                    }
                    sair_game_over_screen = true;
                    break;
                case ALLEGRO_KEY_ESCAPE:
                    restart_game = false;
                    sair_game_over_screen = true;
                    break;
            }
        } else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            restart_game = false;
            sair_game_over_screen = true;
        }

        al_draw_scaled_bitmap(background_game_over, 0, 0,
                              al_get_bitmap_width(background_game_over), al_get_bitmap_height(background_game_over),
                              0, 0, LARGURA, ALTURA, 0);

        al_draw_scaled_bitmap(game_over_image, 0, 0,
                              al_get_bitmap_width(game_over_image), al_get_bitmap_height(game_over_image),
                              (LARGURA - al_get_bitmap_width(game_over_image)) / 2.0,
                              (ALTURA / 2.0 - al_get_bitmap_height(game_over_image) / 2.0) - 50,
                              al_get_bitmap_width(game_over_image), al_get_bitmap_height(game_over_image), 0);

        int start_y = ALTURA / 2 + 130;
        for (int i = 0; i < num_opcoes_game_over; i++) {
            ALLEGRO_COLOR cor_texto = (i == opcao_selecionada_go) ? al_map_rgb(0, 255, 100) : al_map_rgb(255, 255, 255);
            al_draw_text(fonte, cor_texto, LARGURA / 2, start_y + i * al_get_font_line_height(fonte) * 1.5, ALLEGRO_ALIGN_CENTER, opcoes_game_over[i]);
        }

        al_flip_display();
    }

    al_destroy_bitmap(game_over_image);
    al_destroy_bitmap(background_game_over);
    return restart_game;
}


// --- Função Principal do Jogo ---
// Retorna true se o jogo deve ser reiniciado, false se deve sair
bool jogo(ALLEGRO_DISPLAY *janela, ALLEGRO_EVENT_QUEUE *fila, ALLEGRO_FONT *fonte) {
    fprintf(stderr, "DEBUG: Início da função jogo.\n");
    srand(time(NULL)); // Semeia o gerador de números aleatórios

    // --- Variáveis de Jogo ---
    float deslocamento_x = 0.0;
    float velocidade_andar = 3.0;
    float velocidade_correr = 6.0;
    float escala_personagens = 3.0;
    float escala_traficante = 1.7;

    // --- Carregamento de Bitmaps (Sprites) ---
    ALLEGRO_BITMAP *background = al_load_bitmap("background.png");
    ALLEGRO_BITMAP *sprite_garrafa = al_load_bitmap("garrafa.png");
    // Sprites do Noia
    ALLEGRO_BITMAP *sprite_inimigo_parado = al_load_bitmap("noia1_parado.png");
    ALLEGRO_BITMAP *sprite_inimigo_andando = al_load_bitmap("noia1_andando.png");
    ALLEGRO_BITMAP *sprite_inimigo_morte = al_load_bitmap("noia1_morte.png");
    ALLEGRO_BITMAP *sprite_inimigo_ataque = al_load_bitmap("noia1_ataque.png");
    // NOVO: Sprites do Policial
    ALLEGRO_BITMAP *sprite_policial_parado = al_load_bitmap("policial_parado.png");
    ALLEGRO_BITMAP *sprite_policial_arremesso = al_load_bitmap("policial_arremesso.png");
    ALLEGRO_BITMAP *sprite_explosao = al_load_bitmap("explosao.png");
    ALLEGRO_BITMAP *sprite_policial_morte = al_load_bitmap("policial_morte.png");
    // Outros Sprites
    ALLEGRO_BITMAP *sprite_traficante_parada = al_load_bitmap("traficante_parada.png");
    ALLEGRO_BITMAP *sprite_sacos_lixo = al_load_bitmap("sacos_lixo.png");
    ALLEGRO_BITMAP *sprite_placa_radar = al_load_bitmap("placa_radar.png");
    ALLEGRO_BITMAP *sprite_2reais = al_load_bitmap("2reais.png");
    ALLEGRO_BITMAP *sprite_5reais = al_load_bitmap("5reais.png");
    ALLEGRO_BITMAP *sprite_10reais = al_load_bitmap("10reais.png");

    // --- NOVO: Carregamento de Bitmaps do Chefe ---
    ALLEGRO_BITMAP *sprite_gatopreto = al_load_bitmap("gatopreto.png");
    ALLEGRO_BITMAP *sprite_prostituta_parada = al_load_bitmap("prostituta_parada.png");
    ALLEGRO_BITMAP *sprite_prostituta_trans1 = al_load_bitmap("prostituta_trans1.png");
    ALLEGRO_BITMAP *sprite_demon_trans2 = al_load_bitmap("demon_trans2.png");      // Sprite para a segunda transformação
    ALLEGRO_BITMAP *sprite_demon_parado = al_load_bitmap("demon_parado.png");      // Sprite para a animação idle do demônio
    // --- NOVOS SPRITES DO CHEFE ---
    ALLEGRO_BITMAP *sprite_demon_ataque = al_load_bitmap("demon_ataque.png");
    ALLEGRO_BITMAP *sprite_demon_dano = al_load_bitmap("demon_dano.png");
    ALLEGRO_BITMAP *sprite_demon_morte = al_load_bitmap("demon_morte.png");
    ALLEGRO_BITMAP *sprite_demon_projetil = al_load_bitmap("demon_projetil.png");


    // --- Carregamento de Áudio ---
    ALLEGRO_SAMPLE *som_verdebalaraio = al_load_sample("verdebalaraio.ogg");
    ALLEGRO_SAMPLE *som_garrafa_quebrando = al_load_sample("garrafaquebrando.ogg"); // NOVO: Carrega o som da garrafa quebrando
    ALLEGRO_SAMPLE_INSTANCE *instancia_som_verdebalaraio = NULL;

    // --- Verificação de Carregamento de Ativos ---
    if (!background || !sprite_garrafa ||
        !sprite_inimigo_parado || !sprite_inimigo_andando || !sprite_inimigo_morte || !sprite_inimigo_ataque ||
        !sprite_traficante_parada || !som_verdebalaraio || !sprite_sacos_lixo ||
        !sprite_placa_radar || !sprite_2reais || !sprite_5reais || !sprite_10reais ||
        !sprite_policial_parado || !sprite_policial_arremesso || !sprite_explosao || !sprite_policial_morte ||
        !sprite_gatopreto || !sprite_prostituta_parada || !sprite_prostituta_trans1 ||
        !sprite_demon_trans2 || !sprite_demon_parado ||
        // --- NOVO: Checagem dos novos sprites de combate do chefe ---
        !sprite_demon_ataque || !sprite_demon_dano || !sprite_demon_morte || !sprite_demon_projetil ||
        !som_garrafa_quebrando // NOVO: Verifica se o som da garrafa foi carregado
        ) {
        fprintf(stderr, "ERRO CRÍTICO: Falha ao carregar um ou mais bitmaps/áudios! Verifique nomes/caminhos dos arquivos.\n");
        // Limpeza de todos os bitmaps carregados para evitar vazamento de memória
        al_destroy_bitmap(background); al_destroy_bitmap(sprite_garrafa);
        al_destroy_bitmap(sprite_inimigo_parado); al_destroy_bitmap(sprite_inimigo_andando);
        al_destroy_bitmap(sprite_inimigo_morte); al_destroy_bitmap(sprite_inimigo_ataque);
        al_destroy_bitmap(sprite_traficante_parada); al_destroy_sample(som_verdebalaraio); al_destroy_bitmap(sprite_sacos_lixo);
        al_destroy_bitmap(sprite_placa_radar); al_destroy_bitmap(sprite_2reais); al_destroy_bitmap(sprite_5reais);
        al_destroy_bitmap(sprite_10reais); al_destroy_bitmap(sprite_policial_parado); al_destroy_bitmap(sprite_policial_arremesso);
        al_destroy_bitmap(sprite_explosao); al_destroy_bitmap(sprite_policial_morte);
        // Limpeza dos bitmaps do chefe em caso de erro
        al_destroy_bitmap(sprite_gatopreto); al_destroy_bitmap(sprite_prostituta_parada); al_destroy_bitmap(sprite_prostituta_trans1);
        al_destroy_bitmap(sprite_demon_trans2); al_destroy_bitmap(sprite_demon_parado);
        // --- NOVO: Limpeza dos novos bitmaps de combate ---
        al_destroy_bitmap(sprite_demon_ataque); al_destroy_bitmap(sprite_demon_dano); al_destroy_bitmap(sprite_demon_morte); al_destroy_bitmap(sprite_demon_projetil);
        al_destroy_sample(som_garrafa_quebrando); // NOVO
        return false;
    }
    fprintf(stderr, "DEBUG: Todos os bitmaps e áudios carregados com sucesso.\n");

    // --- Cria e anexa instância de áudio ---
    instancia_som_verdebalaraio = al_create_sample_instance(som_verdebalaraio);
    if (!instancia_som_verdebalaraio) {
        fprintf(stderr, "ERRO CRÍTICO: Falha ao criar instancia_som_verdebalaraio!\n");
        al_destroy_bitmap(background); al_destroy_bitmap(sprite_garrafa);
        al_destroy_bitmap(sprite_inimigo_parado); al_destroy_bitmap(sprite_inimigo_andando);
        al_destroy_bitmap(sprite_inimigo_morte); al_destroy_bitmap(sprite_inimigo_ataque);
        al_destroy_bitmap(sprite_traficante_parada);
        al_destroy_sample(som_verdebalaraio);
        al_destroy_bitmap(sprite_sacos_lixo);
        al_destroy_bitmap(sprite_placa_radar);
        al_destroy_bitmap(sprite_2reais); al_destroy_bitmap(sprite_5reais); al_destroy_bitmap(sprite_10reais);
        al_destroy_bitmap(sprite_policial_parado); al_destroy_bitmap(sprite_policial_arremesso); al_destroy_bitmap(sprite_explosao);
        al_destroy_bitmap(sprite_policial_morte);
        // NOVO: Limpeza dos bitmaps do chefe em caso de erro
        al_destroy_bitmap(sprite_gatopreto); al_destroy_bitmap(sprite_prostituta_parada); al_destroy_bitmap(sprite_prostituta_trans1);
        al_destroy_bitmap(sprite_demon_trans2); al_destroy_bitmap(sprite_demon_parado);
        al_destroy_sample(som_garrafa_quebrando); // NOVO
        return false;
    }
    al_set_sample_instance_playmode(instancia_som_verdebalaraio, ALLEGRO_PLAYMODE_LOOP);
    al_attach_sample_instance_to_mixer(instancia_som_verdebalaraio, al_get_default_mixer());
    al_set_sample_instance_gain(instancia_som_verdebalaraio, 0.0); // Começa com volume zero
    al_play_sample_instance(instancia_som_verdebalaraio); // Começa a tocar (silenciosamente)

    // --- Dimensões do Background para Tiling ---
    int bg_width = al_get_bitmap_width(background);
    int bg_height = al_get_bitmap_height(background);

    // --- Definições de Frames e Alturas dos Sprites ---
    // Larguras de frame do Noia
    int frame_largura_inimigo_parado = al_get_bitmap_width(sprite_inimigo_parado) / 7; // Assuming 7 frames
    int frame_largura_inimigo_andando = al_get_bitmap_width(sprite_inimigo_andando) / 8; // Assuming 8 frames
    int frame_largura_inimigo_morte = al_get_bitmap_width(sprite_inimigo_morte) / 4; // Assuming 4 frames
    int frame_largura_inimigo_ataque = al_get_bitmap_width(sprite_inimigo_ataque) / 4; // Assuming 4 frames

    // NOVO: Larguras de frame do Policial e Explosão
    int frame_largura_policial_parado = al_get_bitmap_width(sprite_policial_parado) / 7; // Assuming 7 frames
    int frame_largura_policial_arremesso = al_get_bitmap_width(sprite_policial_arremesso) / 9; // Assuming 9 frames
    int frame_largura_policial_morte = al_get_bitmap_width(sprite_policial_morte) / 4; // Assuming 4 frames
    int frame_largura_explosao = al_get_bitmap_width(sprite_explosao); // Já está em uma linha
    int frame_altura_explosao = al_get_bitmap_height(sprite_explosao); // O sprite de explosão já tem todos os frames em uma linha

    int inimigo_largura_sprite_max = (frame_largura_inimigo_parado > frame_largura_inimigo_andando) ? frame_largura_inimigo_parado : frame_largura_inimigo_andando;
    if (frame_largura_inimigo_morte > inimigo_largura_sprite_max) {
        inimigo_largura_sprite_max = frame_largura_inimigo_morte;
    }
    if (frame_largura_inimigo_ataque > inimigo_largura_sprite_max) {
        inimigo_largura_sprite_max = frame_largura_inimigo_ataque;
    }
    if (frame_largura_policial_parado > inimigo_largura_sprite_max) { // NOVO
        inimigo_largura_sprite_max = frame_largura_policial_parado;
    }
    if (frame_largura_policial_arremesso > inimigo_largura_sprite_max) { // NOVO
        inimigo_largura_sprite_max = frame_largura_policial_arremesso;
    }
    // Altura padrão para personagens (assumindo que inimigos têm a mesma altura base)
    int frame_altura_personagem_base = 128;
    int inimigo_altura_sprite = frame_altura_personagem_base;


    // --- Propriedades da Garrafa e Projétil do Chefe ---
    int garrafa_largura_original = al_get_bitmap_width(sprite_garrafa);
    int garrafa_altura_original = al_get_bitmap_height(sprite_garrafa);
    float escala_garrafa = 1.0;
    float garrafa_hitbox_scale = 0.8;
    float garrafa_hitbox_width = garrafa_largura_original * escala_garrafa * garrafa_hitbox_scale;
    float garrafa_hitbox_height = garrafa_altura_original * escala_garrafa * garrafa_hitbox_scale;
    // --- NOVO: Projétil do Chefe ---
    int boss_proj_largura_original = al_get_bitmap_width(sprite_demon_projetil);
    int boss_proj_altura_original = al_get_bitmap_height(sprite_demon_projetil);
    float escala_boss_proj = 2.0;

    // --- Propriedades de Obstáculos ---
    float escala_obstaculo_sacos = 0.1;
    float escala_placa_radar = 0.3;

    float personagem_y_base = (ALTURA - 300); // Posição Y base para o chão

    // --- Inicialização do Protagonista (AGORA EM UMA LINHA!) ---
    Protagonista protagonista;
    ALLEGRO_BITMAP *prot_sprites[9];
    prot_sprites[0] = al_load_bitmap("personagem.png");
    prot_sprites[1] = al_load_bitmap("personagem_andando.png");
    prot_sprites[2] = al_load_bitmap("personagem_correndo.png");
    prot_sprites[3] = al_load_bitmap("personagem_pulando.png");
    prot_sprites[4] = al_load_bitmap("personagem_agachado.png");
    prot_sprites[5] = al_load_bitmap("personagem_especial.png");
    prot_sprites[6] = al_load_bitmap("personagem_ataque1.png");
    prot_sprites[7] = al_load_bitmap("personagem_arremessando.png");
    prot_sprites[8] = al_load_bitmap("personagem_morte.png");

    for (int i = 0; i < 9; i++) {
        if (!prot_sprites[i]) {
            fprintf(stderr, "ERRO: Falha ao carregar sprite do protagonista em prot_sprites[%d].\n", i);
            // Liberar os que já foram carregados e retornar erro
            for (int j = 0; j < i; j++) al_destroy_bitmap(prot_sprites[j]);
            return false; // Ou lidar com o erro de outra forma
        }
    }
    protagonista_inicializar(&protagonista, prot_sprites);
    protagonista.y_base = personagem_y_base; // Define a base de Y para o protagonista

    // --- Inicialização do NPC Traficante ---
    NPC traficante;
    traficante.largura_sprite = al_get_bitmap_width(sprite_traficante_parada) / NPC_TRAFICANTE_FRAME_COUNT;
    traficante.altura_sprite = al_get_bitmap_height(sprite_traficante_parada);
    traficante.x = 4000;
    traficante.y = personagem_y_base - (traficante.altura_sprite * escala_traficante - protagonista.frame_h * escala_personagens);
    traficante.ativa = true;
    traficante.frame_atual = 0;
    traficante.acc_animacao = 0;

    int player_money = 2; // Dinheiro inicial

    // --- Inicialização de Garrafas, Granadas e Projéteis do Chefe ---
    Garrafa garrafas[MAX_GARRAFAS];
    Granada granadas[MAX_GRANADAS];
    BossProjectile boss_projectiles[MAX_BOSS_PROJECTILES]; // --- NOVO ---
    float garrafa_velocidade_angular = 0.2;

    for (int i = 0; i < MAX_GARRAFAS; i++) {
        garrafas[i].ativa = false;
    }
    for (int i = 0; i < MAX_GRANADAS; i++) {
        granadas[i].ativa = false;
    }
    // --- NOVO ---
    for (int i = 0; i < MAX_BOSS_PROJECTILES; i++) {
        boss_projectiles[i].ativa = false;
    }


    // --- Inicialização de Inimigos ---
    Inimigo inimigos[MAX_INIMIGOS];
    // (Lógica de spawn e inicialização de inimigos - sem alterações)
    float tempo_para_spawn_inimigo = 2.0;
    float min_intervalo_spawn_inimigo = 2.0;
    float max_intervalo_spawn_inimigo = 5.0;
    float inimigo_velocidade_parado_base = -0.5;
    float inimigo_velocidade_andando_base = -2.5;
    float inimigo_velocidade_ataque_base = 0.0;
    float noia_distancia_deteccao = 500.0;
    float noia_distancia_ataque = 180.0;
    float policial_distancia_deteccao = 800.0;
    float policial_distancia_ataque = 700.0;
    for (int i = 0; i < MAX_INIMIGOS; i++) {
        inimigos[i].ativa = false;
    }

    // --- Inicialização de Obstáculos ---
    Obstaculo obstaculos[MAX_OBSTACULOS];
    // (Lógica de spawn e inicialização de obstáculos - sem alterações)
    struct {
        ALLEGRO_BITMAP *sprite;
        float scale;
        bool crouch_pass;
    } obstacle_types[] = {
        {sprite_sacos_lixo, escala_obstaculo_sacos, false},
        {sprite_placa_radar, escala_placa_radar, true}
    };
    int num_obstacle_types = sizeof(obstacle_types) / sizeof(obstacle_types[0]);
    float proximo_ponto_spawn_obstaculo = LARGURA;
    float min_distancia_entre_obstaculos = 500.0;
    float variacao_distancia_obstaculos = 600.0;
    for (int i = 0; i < MAX_OBSTACULOS; i++) {
        obstaculos[i].ativa = true;
        int type_index = rand() % num_obstacle_types;
        obstaculos[i].sprite_bitmap = obstacle_types[type_index].sprite;
        obstaculos[i].only_crouch_pass = obstacle_types[type_index].crouch_pass;
        float current_scale = obstacle_types[type_index].scale;
        int original_width = al_get_bitmap_width(obstaculos[i].sprite_bitmap);
        int original_height = al_get_bitmap_height(obstaculos[i].sprite_bitmap);
        obstaculos[i].width = original_width * current_scale;
        obstaculos[i].height = original_height * current_scale;
        obstaculos[i].x = proximo_ponto_spawn_obstaculo + (float)rand() / RAND_MAX * variacao_distancia_obstaculos;
        obstaculos[i].y = personagem_y_base + (protagonista.frame_h * escala_personagens) - obstaculos[i].height;
        proximo_ponto_spawn_obstaculo = obstaculos[i].x + obstaculos[i].width + min_distancia_entre_obstaculos;
    }

    // --- Inicialização de Notas de Dinheiro ---
    MoneyNote money_notes[MAX_MONEY_NOTES];
    // (Lógica de spawn e inicialização de dinheiro - sem alterações)
    ALLEGRO_BITMAP *money_sprites[] = {sprite_2reais, sprite_5reais, sprite_10reais};
    int money_values[] = {2, 5, 10};
    int num_money_types = sizeof(money_sprites) / sizeof(money_sprites[0]);
    float time_to_spawn_money = MONEY_NOTE_SPAWN_INTERVAL;
    for (int i = 0; i < MAX_MONEY_NOTES; i++) {
        money_notes[i].ativa = false;
    }

    // --- ALTERADO: Inicialização da Entrada do Chefe e do Chefe ---
    BossEntrance entrance;
    entrance.sprite_bitmap = sprite_gatopreto;
    entrance.x = BOSS_SPAWN_X;
    float entrance_original_w = al_get_bitmap_width(entrance.sprite_bitmap);
    float entrance_original_h = al_get_bitmap_height(entrance.sprite_bitmap);
    float entrance_scale = (float)ALTURA / entrance_original_h;
    entrance.width = entrance_original_w * entrance_scale;
    entrance.height = ALTURA;
    entrance.y = 0;

    Boss boss;
    boss.ativa = false;
    boss.estado = BOSS_INVISIVEL;
    boss.escala = 2.8;
    // --- NOVO: Inicialização de combate ---
    boss.health = MAX_BOSS_HEALTH;
    boss.attack_cooldown = BOSS_ATTACK_COOLDOWN;
    boss.invulnerability_timer = 0.0f;

    // Sprites
    boss.sprite_parada = sprite_prostituta_parada;
    boss.sprite_trans1 = sprite_prostituta_trans1;
    boss.sprite_trans2 = sprite_demon_trans2;
    boss.sprite_demonio_idle = sprite_demon_parado;
    // --- NOVOS SPRITES ---
    boss.sprite_demonio_ataque = sprite_demon_ataque;
    boss.sprite_demonio_dano = sprite_demon_dano;
    boss.sprite_demonio_morte = sprite_demon_morte;

    // Dimensões dos frames do chefe
    boss.frame_largura_parada = al_get_bitmap_width(boss.sprite_parada) / BOSS_PROSTITUTA_PARADA_FRAME_COUNT;
    boss.frame_altura_parada = al_get_bitmap_height(boss.sprite_parada);
    boss.frame_largura_trans1 = al_get_bitmap_width(boss.sprite_trans1) / BOSS_PROSTITUTA_TRANS1_FRAME_COUNT;
    boss.frame_altura_trans1 = al_get_bitmap_height(boss.sprite_trans1);
    boss.frame_largura_trans2 = al_get_bitmap_width(boss.sprite_trans2) / BOSS_DEMON_TRANS2_FRAME_COUNT;
    boss.frame_altura_trans2 = al_get_bitmap_height(boss.sprite_trans2);
    boss.frame_largura_demonio_idle = al_get_bitmap_width(boss.sprite_demonio_idle) / BOSS_DEMON_IDLE_FRAME_COUNT;
    boss.frame_altura_demonio_idle = al_get_bitmap_height(boss.sprite_demonio_idle);
    // --- NOVAS DIMENSÕES ---
    boss.frame_largura_demonio_ataque = al_get_bitmap_width(boss.sprite_demonio_ataque) / BOSS_DEMON_ATTACK_FRAME_COUNT;
    boss.frame_altura_demonio_ataque = al_get_bitmap_height(boss.sprite_demonio_ataque);
    boss.frame_largura_demonio_dano = al_get_bitmap_width(boss.sprite_demonio_dano) / BOSS_DEMON_DANO_FRAME_COUNT;
    boss.frame_altura_demonio_dano = al_get_bitmap_height(boss.sprite_demonio_dano);
    boss.frame_largura_demonio_morte = al_get_bitmap_width(boss.sprite_demonio_morte) / BOSS_DEMON_MORTE_FRAME_COUNT;
    boss.frame_altura_demonio_morte = al_get_bitmap_height(boss.sprite_demonio_morte);


    // --- Configuração do Timer e Fila de Eventos ---
    ALLEGRO_TIMER *timer = al_create_timer(1.0 / 60.0);
    al_start_timer(timer);
    al_register_event_source(fila, al_get_display_event_source(janela));
    al_register_event_source(fila, al_get_keyboard_event_source());
    al_register_event_source(fila, al_get_timer_event_source(timer));

    bool jogando = true;
    bool should_restart = false;
    float deslocamento_x_anterior = deslocamento_x;

    // --- Loop Principal do Jogo ---
    while (jogando) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(fila, &ev);

        // --- Tratamento de Eventos (Janela, Teclado, Pausa) ---
        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            jogando = false;
            should_restart = false;
        } else if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                al_stop_timer(timer);
                al_draw_filled_rectangle(0, 0, LARGURA, ALTURA, al_map_rgba(0, 0, 0, 150));
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 2, (ALTURA / 2) - al_get_font_line_height(fonte), ALLEGRO_ALIGN_CENTER, "PAUSADO");
                al_draw_text(fonte, al_map_rgb(200, 200, 200), LARGURA / 2, (ALTURA / 2) + al_get_font_line_height(fonte), ALLEGRO_ALIGN_CENTER, "Pressione ESC para continuar");
                al_flip_display();

                bool despausar = false;
                while (!despausar) {
                    ALLEGRO_EVENT evento_pausa;
                    al_wait_for_event(fila, &evento_pausa);
                    if (evento_pausa.type == ALLEGRO_EVENT_KEY_DOWN) {
                        if (evento_pausa.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                            despausar = true;
                        }
                    } else if (evento_pausa.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
                        jogando = false;
                        should_restart = false;
                        despausar = true;
                    }
                }
                if (jogando) {
                    al_start_timer(timer);
                }
            } else {
                // Processa input do protagonista
                // O tratamento de "Garrafas" aqui precisa ser passado como array, como definido em protagonista.h
                protagonista_processar_evento(&protagonista, &ev, garrafas, deslocamento_x);

                // Processa input do jogo (interações com outros objetos que não são o protagonista)
                if (ev.keyboard.keycode == ALLEGRO_KEY_B) {
                    float prot_world_x_center = protagonista.x + (protagonista.frame_w_parado * escala_personagens) / 2.0 + deslocamento_x;

                    // Lógica de interação com o Traficante
                    float npc_world_x_center = traficante.x + (traficante.largura_sprite * escala_traficante) / 2.0;
                    float distance_to_npc = fabs(prot_world_x_center - npc_world_x_center);
                    if (traficante.ativa && distance_to_npc < NPC_INTERACTION_DISTANCE) {
                        if (player_money >= NPC_HEAL_COST) {
                            if (protagonista.vida >= MAX_PROTAGONIST_HEALTH + NPC_HEAL_AMOUNT) {
                                fprintf(stderr, "DEBUG: Vida já está no máximo (120 HP)! Não é possível comprar mais pó.\n");
                            } else {
                                player_money -= NPC_HEAL_COST;
                                protagonista.vida += NPC_HEAL_AMOUNT;
                                if (protagonista.vida > MAX_PROTAGONIST_HEALTH + NPC_HEAL_AMOUNT) {
                                    protagonista.vida = MAX_PROTAGONIST_HEALTH + NPC_HEAL_AMOUNT;
                                }
                                fprintf(stderr, "DEBUG: Vida recuperada! HP: %d, Dinheiro: R$%d\n", protagonista.vida, player_money);
                            }
                        } else {
                            fprintf(stderr, "DEBUG: Sem dinheiro suficiente para comprar! Dinheiro atual: R$%d, Custo: R$%d\n", player_money, NPC_HEAL_COST);
                        }
                    }

                    // Lógica de Interação com o Chefe
                    if (boss.ativa && boss.estado == BOSS_PARADA) {
                        float boss_world_x_center = boss.x + (boss.frame_largura_parada * boss.escala) / 2.0;
                        float distance_to_boss = fabs(prot_world_x_center - boss_world_x_center);

                        if (distance_to_boss < BOSS_INTERACTION_DISTANCE) {
                            fprintf(stderr, "DEBUG: Jogador interagiu com o chefe! Iniciando transformação.\n");
                            boss.estado = BOSS_TRANSFORMANDO;
                            boss.frame_atual = 0;
                            boss.acc_animacao = 0;
                        }
                    }
                }
            }
        } else if (ev.type == ALLEGRO_EVENT_TIMER) {
            // Atualiza o protagonista
            protagonista_atualizar(&protagonista, &protagonista.estado, &deslocamento_x, velocidade_andar, velocidade_correr);

            // --- Atualização do NPC Traficante ---
            if (traficante.ativa) {
                traficante.acc_animacao += 1.0 / 60.0;
                if (traficante.acc_animacao >= NPC_TRAFICANTE_TPF) {
                    traficante.acc_animacao -= NPC_TRAFICANTE_TPF;
                    traficante.frame_atual = (traficante.frame_atual + 1) % NPC_TRAFICANTE_FRAME_COUNT;
                }
            }

            // --- Atualiza Volume do Áudio do NPC (Fade) ---
            if (traficante.ativa && instancia_som_verdebalaraio) {
                float prot_world_x_center = protagonista.x + (protagonista.frame_w_parado * escala_personagens) / 2.0 + deslocamento_x;
                float npc_world_x_center = traficante.x + (traficante.largura_sprite * escala_traficante) / 2.0;
                float distance = fabs(prot_world_x_center - npc_world_x_center);
                float volume;

                if (distance <= AUDIO_MIN_DISTANCE) {
                    volume = 1.0;
                } else if (distance >= AUDIO_MAX_DISTANCE) {
                    volume = 0.0;
                } else {
                    volume = 1.0 - ((distance - AUDIO_MIN_DISTANCE) / (AUDIO_MAX_DISTANCE - AUDIO_MIN_DISTANCE));
                    if (volume < 0.0) volume = 0.0;
                    if (volume > 1.0) volume = 1.0;
                }
                al_set_sample_instance_gain(instancia_som_verdebalaraio, volume);
            }

            // --- Atualização de Garrafas ---
            for (int i = 0; i < MAX_GARRAFAS; i++) {
                if (garrafas[i].ativa) {
                    garrafas[i].x += garrafas[i].vel_x;
                    garrafas[i].angulo += garrafa_velocidade_angular;
                    if (garrafas[i].angulo > ALLEGRO_PI * 2) {
                        garrafas[i].angulo -= ALLEGRO_PI * 2;
                    }
                    if (garrafas[i].x - deslocamento_x > LARGURA + (garrafa_largura_original * escala_garrafa)) {
                        garrafas[i].ativa = false;
                    }
                }
            }

            // --- NOVO: Atualização dos Projéteis do Chefe ---
            for (int i = 0; i < MAX_BOSS_PROJECTILES; i++) {
                if (boss_projectiles[i].ativa) {
                    boss_projectiles[i].x += boss_projectiles[i].vel_x;
                    boss_projectiles[i].angulo += 0.2f; // Gira o projétil

                    // Desativa se sair da tela
                    if (boss_projectiles[i].x - deslocamento_x < -100 || boss_projectiles[i].x - deslocamento_x > LARGURA + 100) {
                        boss_projectiles[i].ativa = false;
                    }
                }
            }

            // --- Atualização das Granadas ---
            for (int i = 0; i < MAX_GRANADAS; i++) {
                if (granadas[i].ativa) {
                    if (!granadas[i].explodindo) {
                        granadas[i].x += granadas[i].vel_x;
                        granadas[i].y += granadas[i].vel_y;
                        granadas[i].vel_y += 0.5 * 0.5; // Granada mais leve que o personagem (gravidade * 0.5)

                        // Checa se atingiu o chão
                        if (granadas[i].y >= protagonista.y_base + (protagonista.frame_h * escala_personagens) - (frame_altura_explosao * escala_personagens / 2.0)) {
                            granadas[i].y = protagonista.y_base + (protagonista.frame_h * escala_personagens) - (frame_altura_explosao * escala_personagens / 2.0);
                            granadas[i].explodindo = true;
                            granadas[i].frame_explosao = 0;
                            granadas[i].acc_animacao = 0;
                            granadas[i].dano_aplicado = false; // Permite aplicar dano
                        }
                    } else { // Está explodindo
                        granadas[i].acc_animacao += 1.0 / 60.0;
                        if (granadas[i].acc_animacao >= (1.0 / 15.0)) { // tpf_explosao
                            granadas[i].acc_animacao -= (1.0 / 15.0);
                            if (granadas[i].frame_explosao < 9 - 1) { // frame_total_explosao
                                granadas[i].frame_explosao++;
                            } else {
                                granadas[i].ativa = false; // Animação da explosão terminou
                            }
                        }
                    }
                }
            }

            // --- Lógica de Spawn de Inimigos (ATUALIZADA) ---
            tempo_para_spawn_inimigo -= 1.0 / 60.0;
            // Modificado: Não spawna inimigos se o chefe estiver ativo OU já tiver sido permanentemente derrotado
            if (tempo_para_spawn_inimigo <= 0 && !boss.ativa && !boss_defeated_permanently) {
                for (int i = 0; i < MAX_INIMIGOS; i++) {
                    if (!inimigos[i].ativa) {
                        // Decide o tipo de inimigo a ser spawnado
                        if (rand() % 2 == 0) {
                            inimigos[i].type = NOIA;
                        } else {
                            inimigos[i].type = POLICIAL;
                        }

                        // Inicialização comum
                        inimigos[i].x = deslocamento_x + LARGURA + (float)(rand() % 200 + 50);
                        inimigos[i].y = personagem_y_base;
                        inimigos[i].ativa = true;
                        inimigos[i].frame_atual = 0;
                        inimigos[i].acc_animacao = 0;
                        inimigos[i].estado = INIMIGO_PARADO;
                        inimigos[i].animacao_morte_finalizada = false;
                        inimigos[i].inimigo_pode_dar_dano = true;
                        inimigos[i].attack_cooldown = 0;

                        // Inicialização específica por tipo
                        if (inimigos[i].type == NOIA) {
                            inimigos[i].vel_x = inimigo_velocidade_parado_base;
                            inimigos[i].hitbox_offset_x = 57.0 * escala_personagens;
                            inimigos[i].hitbox_offset_y = 20 * escala_personagens;
                            inimigos[i].hitbox_width = (frame_largura_inimigo_parado - 100) * escala_personagens;
                            inimigos[i].hitbox_height = (frame_altura_personagem_base - 20) * escala_personagens;
                        } else { // POLICIAL
                            inimigos[i].vel_x = inimigo_velocidade_parado_base;
                            inimigos[i].hitbox_offset_x = 57.0 * escala_personagens;
                            inimigos[i].hitbox_offset_y = 20 * escala_personagens;
                            inimigos[i].hitbox_width = (frame_largura_policial_parado - 100) * escala_personagens;
                            inimigos[i].hitbox_height = (frame_altura_personagem_base - 20) * escala_personagens;
                        }

                        tempo_para_spawn_inimigo = min_intervalo_spawn_inimigo +
                                                   ((float)rand() / RAND_MAX) * (max_intervalo_spawn_inimigo - min_intervalo_spawn_inimigo);
                        fprintf(stderr, "DEBUG: Inimigo %d (Tipo: %d) SPAWNADO.\n", i, inimigos[i].type);
                        break;
                    }
                }
            }

            // --- Lógica de Spawn e Atualização de Dinheiro ---
            time_to_spawn_money -= 1.0 / 60.0;
            // Modificado: Não spawna dinheiro se o chefe estiver ativo OU já tiver sido permanentemente derrotado
            if (time_to_spawn_money <= 0 && !boss.ativa && !boss_defeated_permanently) {
                for (int i = 0; i < MAX_MONEY_NOTES; i++) {
                    if (!money_notes[i].ativa) {
                        int type_index = rand() % num_money_types;
                        money_notes[i].sprite_bitmap = money_sprites[type_index];
                        money_notes[i].value = money_values[type_index];
                        money_notes[i].hitbox_width = al_get_bitmap_width(money_notes[i].sprite_bitmap) * MONEY_NOTE_SCALE;
                        money_notes[i].hitbox_height = al_get_bitmap_height(money_notes[i].sprite_bitmap) * MONEY_NOTE_SCALE;

                        money_notes[i].x = deslocamento_x + LARGURA + (float)(rand() % 400 + 100);
                        money_notes[i].y = personagem_y_base + (protagonista.frame_h * escala_personagens) - money_notes[i].hitbox_height;
                        money_notes[i].ativa = true;
                        money_notes[i].spawn_time = al_get_time();
                        fprintf(stderr, "DEBUG: Nota de R$%d SPAWNADA em (%.2f, %.2f).\n", money_notes[i].value, money_notes[i].x, money_notes[i].y);
                        break;
                    }
                }
            }

            for (int i = 0; i < MAX_MONEY_NOTES; i++) {
                if (money_notes[i].ativa) {
                    if (money_notes[i].x - deslocamento_x > LARGURA + money_notes[i].hitbox_width || // Nota saiu da tela para a direita
                        (al_get_time() - money_notes[i].spawn_time > MONEY_NOTE_LIFETIME)) { // Tempo de vida expirou
                        money_notes[i].ativa = false;
                        fprintf(stderr, "DEBUG: Nota de dinheiro desativada (fora da tela ou tempo de vida expirou).\n");
                    }
                }
            }

            // --- Lógica de Reciclagem de Objetos ---
            float max_x_existente = deslocamento_x + LARGURA;

            for(int i = 0; i < MAX_OBSTACULOS; i++) {
                if (obstaculos[i].ativa && obstaculos[i].x > max_x_existente) {
                    max_x_existente = obstaculos[i].x;
                }
            }
            if (traficante.ativa && traficante.x > max_x_existente) {
                max_x_existente = traficante.x;
            }
            for (int i = 0; i < MAX_INIMIGOS; i++) {
                if (inimigos[i].ativa && inimigos[i].x > max_x_existente) {
                    max_x_existente = inimigos[i].x;
                }
            }
            for (int i = 0; i < MAX_MONEY_NOTES; i++) {
                if (money_notes[i].ativa && money_notes[i].x > max_x_existente) {
                    max_x_existente = money_notes[i].x;
                }
            }

            for (int i = 0; i < MAX_OBSTACULOS; i++) {
                if (obstaculos[i].ativa && ((obstaculos[i].x + obstaculos[i].width) < deslocamento_x - LARGURA / 2) && !boss.ativa) {
                    fprintf(stderr, "DEBUG: Reciclando obstaculo %d.\n", i);
                    int type_index = rand() % num_obstacle_types;
                    obstaculos[i].sprite_bitmap = obstacle_types[type_index].sprite;
                    obstaculos[i].only_crouch_pass = obstacle_types[type_index].crouch_pass;
                    float current_scale = obstacle_types[type_index].scale;
                    obstaculos[i].width = al_get_bitmap_width(obstaculos[i].sprite_bitmap) * current_scale;
                    obstaculos[i].height = al_get_bitmap_height(obstaculos[i].sprite_bitmap) * current_scale;
                    obstaculos[i].x = max_x_existente + min_distancia_entre_obstaculos + (float)rand() / RAND_MAX * variacao_distancia_obstaculos;
                    obstaculos[i].y = personagem_y_base + (protagonista.frame_h * escala_personagens) - obstaculos[i].height;
                    obstaculos[i].ativa = true;
                }
            }

            if (traficante.ativa && (traficante.x + traficante.largura_sprite * escala_traficante) < deslocamento_x - LARGURA / 2) {
                fprintf(stderr, "DEBUG: Reciclando traficante.\n");
                float distancia_base = 5000.0;
                float variacao_distancia = 1500.0;
                traficante.x = deslocamento_x + distancia_base + (float)rand() / RAND_MAX * variacao_distancia;
                fprintf(stderr, "DEBUG: Traficante reposicionado para x = %.2f\n", traficante.x);
            }

            for (int i = 0; i < MAX_INIMIGOS; i++) {
                if (inimigos[i].ativa && (inimigos[i].x + inimigo_largura_sprite_max * escala_personagens) < deslocamento_x - LARGURA / 2) {
                    fprintf(stderr, "DEBUG: Inimigo %d DESATIVADO (saiu da tela à esquerda).\n", i);
                    inimigos[i].ativa = false;
                    inimigos[i].estado = INIMIGO_PARADO;
                    inimigos[i].vel_x = inimigo_velocidade_parado_base;
                    inimigos[i].animacao_morte_finalizada = false;
                    inimigos[i].frame_atual = 0;
                    inimigos[i].acc_animacao = 0;
                    inimigos[i].inimigo_pode_dar_dano = true;
                }
            }

            for (int i = 0; i < MAX_MONEY_NOTES; i++) {
                if (money_notes[i].ativa && (money_notes[i].x + money_notes[i].hitbox_width) < deslocamento_x - LARGURA / 2) {
                    money_notes[i].ativa = false;
                    fprintf(stderr, "DEBUG: Nota de dinheiro desativada (fora da tela à esquerda).\n");
                }
            }

            // --- Lógica de Ativação e Atualização do Chefe (Modificado) ---
            // Verifica se o jogador alcançou a área do chefe E se o chefe ainda não foi permanentemente derrotado
            if (!boss.ativa && deslocamento_x >= BOSS_SPAWN_X - LARGURA && !boss_defeated_permanently) {
                boss.ativa = true;
                fprintf(stderr, "DEBUG: Chefe ativado. Removendo inimigos e obstáculos.\n");
                for (int i = 0; i < MAX_INIMIGOS; i++) {
                    inimigos[i].ativa = false;
                }
                for (int i = 0; i < MAX_OBSTACULOS; i++) {
                    obstaculos[i].ativa = false;
                }
                for (int i = 0; i < MAX_MONEY_NOTES; i++) {
                    money_notes[i].ativa = false;
                }
                boss.estado = BOSS_PARADA;
                boss.frame_atual = 0;
                boss.acc_animacao = 0;
                // Posiciona o chefe um pouco à frente da entrada
                boss.x = entrance.x + 150;
                boss.y = personagem_y_base + (protagonista.frame_h * escala_personagens) - (boss.frame_altura_parada * boss.escala);
                fprintf(stderr, "DEBUG: Chefe ativado na posição X: %.2f\n", boss.x);
            }

            // Se o chefe estiver ativo, atualiza sua lógica
            if (boss.ativa) {
                 // --- NOVO: Atualiza cooldowns do chefe ---
                if(boss.attack_cooldown > 0) {
                    boss.attack_cooldown -= 1.0 / 60.0;
                }
                if(boss.invulnerability_timer > 0) {
                    boss.invulnerability_timer -= 1.0 / 60.0;
                }

                switch(boss.estado) {
                    case BOSS_PARADA:
                        // Animação da prostituta parada
                        boss.acc_animacao += 1.0 / 60.0;
                        if (boss.acc_animacao >= (1.0 / 5.0)) { // tpf_boss_parada
                            boss.acc_animacao -= (1.0 / 5.0);
                            boss.frame_atual = (boss.frame_atual + 1) % BOSS_PROSTITUTA_PARADA_FRAME_COUNT;
                        }
                        // A LÓGICA DE INTERAÇÃO COM O CHEFE FOI MOVIDA PARA O EVENTO DE TECLADO 'B'
                        break;

                    case BOSS_TRANSFORMANDO:
                        // Animação da transformação (parte 1)
                        boss.acc_animacao += 1.0 / 60.0;
                        if (boss.acc_animacao >= (1.0 / 12.0)) { // tpf_boss_trans1
                            boss.acc_animacao -= (1.0 / 12.0);
                            if (boss.frame_atual < BOSS_PROSTITUTA_TRANS1_FRAME_COUNT - 1) {
                                boss.frame_atual++;
                            } else {
                                // A animação terminou. Transiciona para a próxima parte da transformação.
                                fprintf(stderr, "DEBUG: Fim da transformação parte 1. Iniciando parte 2.\n");
                                boss.estado = BOSS_TRANSFORMANDO_2; // Transiciona para a segunda parte
                                boss.frame_atual = 0; // Reinicia o frame para a próxima animação
                                boss.acc_animacao = 0;
                            }
                        }
                        break;

                    // --- NOVO: Estado de transformação parte 2 ---
                    case BOSS_TRANSFORMANDO_2:
                        boss.acc_animacao += 1.0 / 60.0;
                        if (boss.acc_animacao >= (1.0 / 12.0)) { // tpf_boss_trans2
                            boss.acc_animacao -= (1.0 / 12.0);
                            if (boss.frame_atual < BOSS_DEMON_TRANS2_FRAME_COUNT - 1) {
                                boss.frame_atual++;
                            } else {
                                // A animação da segunda transformação terminou. Transiciona para o idle do demônio.
                                fprintf(stderr, "DEBUG: Fim da transformação parte 2. Assumindo forma de demônio (IDLE).\n");
                                boss.estado = BOSS_DEMONIO_IDLE;
                                boss.frame_atual = 0; // Reinicia o frame para a animação idle do demônio
                                boss.acc_animacao = 0;
                                // Ajusta a posição Y para a nova altura do sprite do demônio, se necessário
                                boss.y = personagem_y_base + (protagonista.frame_h * escala_personagens) - (boss.frame_altura_demonio_idle * boss.escala);
                            }
                        }
                        break;

                    // --- NOVO: Desenha o chefe demônio em idle ---
                    case BOSS_DEMONIO_IDLE:
                        // Animação de idle para o chefe demônio
                        boss.acc_animacao += 1.0 / 60.0;
                        if (boss.acc_animacao >= (1.0 / 8.0)) { // tpf_boss_demonio_idle
                            boss.acc_animacao -= (1.0 / 8.0);
                            boss.frame_atual = (boss.frame_atual + 1) % BOSS_DEMON_IDLE_FRAME_COUNT;
                        }
                        // --- NOVO: Lógica para iniciar ataque ---
                        if(boss.attack_cooldown <= 0) {
                            boss.estado = BOSS_DEMONIO_ATACANDO;
                            boss.frame_atual = 0;
                            boss.acc_animacao = 0;
                            fprintf(stderr, "DEBUG: Chefe iniciando ataque!\n");
                        }
                        break;
                    // --- NOVO: LÓGICA DE COMBATE DO CHEFE ---
                    case BOSS_DEMONIO_ATACANDO:
                        boss.acc_animacao += 1.0 / 60.0;
                        if (boss.acc_animacao >= (1.0 / 10.0)) { // tpf_boss_ataque
                            boss.acc_animacao -= (1.0 / 10.0);
                            if (boss.frame_atual < BOSS_DEMON_ATTACK_FRAME_COUNT - 1) {
                                boss.frame_atual++;
                                // Lança o projétil em um frame específico
                                if (boss.frame_atual == 4) {
                                    for(int i = 0; i < MAX_BOSS_PROJECTILES; i++) {
                                        if(!boss_projectiles[i].ativa) {
                                            boss_projectiles[i].ativa = true;
                                            boss_projectiles[i].x = boss.x + 50; // Posição no mundo, ajustada para sair da frente
                                            boss_projectiles[i].y = boss.y + (boss.frame_altura_demonio_ataque * boss.escala / 2);
                                            boss_projectiles[i].vel_x = -12.0f; // Velocidade para a esquerda
                                            boss_projectiles[i].angulo = 0;
                                            fprintf(stderr, "DEBUG: Chefe lançou projétil %d.\n", i);
                                            break;
                                        }
                                    }
                                }
                            } else {
                                // Fim da animação de ataque
                                boss.estado = BOSS_DEMONIO_IDLE;
                                boss.frame_atual = 0;
                                boss.acc_animacao = 0;
                                boss.attack_cooldown = BOSS_ATTACK_COOLDOWN; // Reinicia o cooldown
                            }
                        }
                        break;
                    case BOSS_DEMONIO_DANO:
                        boss.acc_animacao += 1.0 / 60.0;
                        if (boss.acc_animacao >= (1.0 / 12.0)) { // tpf_boss_dano
                             boss.acc_animacao -= (1.0 / 12.0);
                             if(boss.frame_atual < BOSS_DEMON_DANO_FRAME_COUNT - 1) {
                                 boss.frame_atual++;
                             } else {
                                 // Fim da animação de dano
                                 boss.estado = BOSS_DEMONIO_IDLE;
                                 boss.frame_atual = 0;
                                 boss.acc_animacao = 0;
                             }
                        }
                        break;
                    case BOSS_DEMONIO_MORRENDO:
                        boss.acc_animacao += 1.0 / 60.0;
                        if (boss.acc_animacao >= (1.0 / 8.0)) { // tpf_boss_morte
                             boss.acc_animacao -= (1.0 / 8.0);
                             if(boss.frame_atual < BOSS_DEMON_MORTE_FRAME_COUNT - 1) {
                                 boss.frame_atual++;
                             } else {
                                 // Fim da animação de morte
                                 boss.ativa = false; // Desativa o chefe
                                 boss_defeated_permanently = true; // Define que o chefe foi derrotado permanentemente
                                 jogando = false; // Sai do loop principal do jogo para ir para a tela de vitória
                                 should_restart = true; // Prepara para a tela de vitória
                                 fprintf(stderr, "DEBUG: CHEFE DERROTADO PERMANENTEMENTE! Indo para tela de Vitoria.\n");
                             }
                        }
                        break;
                    default:
                        break;
                }
            }

            // --- Lógica de Colisões ---
            float prot_hb_x = protagonista.x + protagonista.prot_hitbox_offset_x;
            float prot_hb_y = protagonista.y + protagonista.prot_hitbox_offset_y;
            float prot_hb_w = protagonista.prot_hitbox_width;
            float current_prot_hitbox_height = protagonista.agachando ? protagonista.prot_crouch_hitbox_height : protagonista.prot_hitbox_height;
            if(protagonista.agachando) prot_hb_y = protagonista.y + (protagonista.prot_hitbox_height - protagonista.prot_crouch_hitbox_height) + protagonista.prot_hitbox_offset_y;

            // Colisão Inimigos vs Protagonista
            float prot_attack_hb_x = protagonista.x + protagonista.prot_attack_hitbox_offset_x;
            float prot_attack_hb_y = protagonista.y + protagonista.prot_attack_hitbox_offset_y;

            for (int i = 0; i < MAX_INIMIGOS; i++) {
                if (inimigos[i].ativa) {
                    // --- CORREÇÃO: Variáveis movidas para o topo do bloco para compatibilidade ---
                    float inimigo_x_antes_movimento;
                    float inimigo_screen_x;
                    float inimigo_screen_hb_x;
                    float inimigo_hb_y;
                    float inimigo_hb_w;
                    float inimigo_hb_h;
                    float inimigo_world_hb_x;
                    float prot_world_x_center_hb;
                    float inimigo_world_x_center_hb;
                    float dist_to_protagonist_center_x;
                    float distancia_ataque_atual;
                    float distancia_deteccao_atual;
                    float tpf_current_inimigo = 0;
                    int frame_total_current_inimigo = 0;
                    // ------------------------------------------------------------------------

                    // Atualiza cooldown de ataque
                    if (inimigos[i].attack_cooldown > 0) {
                        inimigos[i].attack_cooldown -= 1.0 / 60.0;
                    }

                    inimigo_x_antes_movimento = inimigos[i].x;
                    inimigos[i].x += inimigos[i].vel_x;

                    inimigo_screen_x = inimigos[i].x - deslocamento_x;
                    inimigo_screen_hb_x = inimigo_screen_x + inimigos[i].hitbox_offset_x;
                    inimigo_hb_y = inimigos[i].y + inimigos[i].hitbox_offset_y;
                    inimigo_hb_w = inimigos[i].hitbox_width;
                    inimigo_hb_h = inimigos[i].hitbox_height;

                    // --- Colisão: Ataque do Protagonista vs Inimigo ---
                    if (protagonista.atacando && inimigos[i].estado != INIMIGO_MORRENDO) {
                        if (check_collision(prot_attack_hb_x, prot_attack_hb_y, protagonista.prot_attack_hitbox_width, protagonista.prot_attack_hitbox_height,
                                            inimigo_screen_hb_x, inimigo_hb_y, inimigo_hb_w, inimigo_hb_h)) {
                            fprintf(stderr, "COLISÃO! Ataque Especial atingiu Inimigo %d! Iniciando animação de morte.\n", i);
                            inimigos[i].estado = INIMIGO_MORRENDO;
                            inimigos[i].vel_x = 0;
                            inimigos[i].frame_atual = 0;
                            inimigos[i].acc_animacao = 0;
                            inimigos[i].animacao_morte_finalizada = false;
                        }
                    }

                    // --- Colisão com Protagonista: Lógica de Bloqueio e Ataque ---
                    if (inimigos[i].estado != INIMIGO_MORRENDO && protagonista.estado == PROT_NORMAL) {
                        if (check_collision(inimigo_screen_hb_x, inimigo_hb_y, inimigo_hb_w, inimigo_hb_h,
                                            prot_hb_x, prot_hb_y, prot_hb_w, current_prot_hitbox_height)) {

                            deslocamento_x = deslocamento_x_anterior; // Reverte o movimento do personagem para evitar sobreposição
                            inimigos[i].x = inimigo_x_antes_movimento; // Reverte o movimento do inimigo
                            // Não precisamos recalcular delta_deslocamento_x ou deslocamento_x_anterior aqui,
                            // pois o protagonista_atualizar() já lida com isso.
                            // Apenas resetamos o deslocamento_x para evitar que o personagem "atravesse" o inimigo.


                            if (inimigos[i].inimigo_pode_dar_dano && !protagonista.invulneravel) {
                                if (inimigos[i].type == NOIA && inimigos[i].estado == INIMIGO_ATACANDO) {
                                    protagonista.vida -= PROTAGONIST_DAMAGE_PER_HIT;
                                    inimigos[i].inimigo_pode_dar_dano = false;
                                    fprintf(stderr, "PROTAGONISTA SOFREU DANO DO NOIA! Vida restante: %d\n", protagonista.vida);

                                    protagonista.invulneravel = true;
                                    protagonista.timer_invulnerabilidade = PROTAGONIST_INVULNERABILITY_DURATION;
                                    protagonista.timer_piscar = 0.0;
                                    protagonista.visivel = false;
                                }
                                // Policial causa dano via granada, não contato direto
                            }

                            if (protagonista.vida <= 0) {
                                protagonista.estado = PROT_MORRENDO;
                                protagonista.frame_atual = 0; // Reinicia o frame para a animação de morte do protagonista
                                protagonista.acc_animacao = 0;
                                protagonista.animacao_morte_completa = false;
                                fprintf(stderr, "PROTAGONISTA MORREU! Iniciando animação de morte do protagonista.\n");
                            }
                        }
                    }

                    inimigo_world_hb_x = inimigos[i].x + inimigos[i].hitbox_offset_x;
                    if (inimigos[i].estado != INIMIGO_MORRENDO) {
                        for (int k = 0; k < MAX_INIMIGOS; k++) {
                            if (i == k) continue;
                            if (inimigos[k].ativa && inimigos[k].estado != INIMIGO_MORRENDO) {
                                float other_inimigo_world_hb_x = inimigos[k].x + inimigos[k].hitbox_offset_x;
                                float other_inimigo_hb_y = inimigos[k].y + inimigos[k].hitbox_offset_y;
                                float other_inimigo_hb_w = inimigos[k].hitbox_width;
                                float other_inimigo_hb_h = inimigos[k].hitbox_height;

                                if (check_collision(inimigo_world_hb_x, inimigo_hb_y, inimigo_hb_w, inimigo_hb_h,
                                                    other_inimigo_world_hb_x, other_inimigo_hb_y, other_inimigo_hb_w, other_inimigo_hb_h)) {
                                    float overlap_amount = 0;
                                    if (inimigo_world_hb_x < other_inimigo_world_hb_x) {
                                        overlap_amount = (inimigo_world_hb_x + inimigo_hb_w) - other_inimigo_world_hb_x;
                                        inimigos[i].x -= overlap_amount;
                                    } else {
                                        overlap_amount = (other_inimigo_world_hb_x + other_inimigo_hb_w) - inimigo_world_hb_x;
                                        inimigos[i].x += overlap_amount;
                                    }
                                    inimigo_world_hb_x = inimigos[i].x + inimigos[i].hitbox_offset_x;
                                }
                            }
                        }
                    }

                    // --- LÓGICA DE IA E ESTADOS ---
                    if (inimigos[i].estado != INIMIGO_MORRENDO) {
                        prot_world_x_center_hb = (protagonista.x + protagonista.prot_hitbox_offset_x + protagonista.prot_hitbox_width / 2.0) + deslocamento_x;
                        inimigo_world_x_center_hb = inimigos[i].x + inimigos[i].hitbox_offset_x + inimigos[i].hitbox_width / 2.0;
                        dist_to_protagonist_center_x = fabs(inimigo_world_x_center_hb - prot_world_x_center_hb);

                        distancia_ataque_atual = (inimigos[i].type == NOIA) ? noia_distancia_ataque : policial_distancia_ataque;
                        distancia_deteccao_atual = (inimigos[i].type == NOIA) ? noia_distancia_deteccao : policial_distancia_deteccao;

                        // Transições de estado
                        if (dist_to_protagonist_center_x <= distancia_ataque_atual && inimigos[i].attack_cooldown <= 0) {
                            if (inimigos[i].estado != INIMIGO_ATACANDO) {
                                inimigos[i].estado = INIMIGO_ATACANDO;
                                inimigos[i].vel_x = inimigo_velocidade_ataque_base;
                                inimigos[i].frame_atual = 0;
                                inimigos[i].acc_animacao = 0;
                                inimigos[i].inimigo_pode_dar_dano = true; // Permite dano para o novo ataque
                            }
                        } else if (dist_to_protagonist_center_x < distancia_deteccao_atual && inimigos[i].estado != INIMIGO_ATACANDO) {
                            if (inimigos[i].estado != INIMIGO_ANDANDO) {
                                inimigos[i].estado = INIMIGO_ANDANDO;
                                inimigos[i].vel_x = inimigo_velocidade_andando_base;
                            }
                        } else if (inimigos[i].estado != INIMIGO_ATACANDO) {
                            if (inimigos[i].estado != INIMIGO_PARADO) {
                                inimigos[i].estado = INIMIGO_PARADO;
                                inimigos[i].vel_x = inimigo_velocidade_parado_base;
                            }
                        }
                    }

                    // --- ATUALIZAÇÃO DE ANIMAÇÃO ---
                    if (inimigos[i].type == NOIA) {
                        switch(inimigos[i].estado) {
                            case INIMIGO_PARADO: tpf_current_inimigo = (1.0 / 8.0); frame_total_current_inimigo = 7; break; // tpf_inimigo_parado, frame_total_inimigo_parado
                            case INIMIGO_ANDANDO: tpf_current_inimigo = (1.0 / 10.0); frame_total_current_inimigo = 8; break; // tpf_inimigo_andando, frame_total_inimigo_andando
                            case INIMIGO_ATACANDO: tpf_current_inimigo = (1.0 / 8.0); frame_total_current_inimigo = 4; break; // tpf_inimigo_ataque, frame_total_inimigo_ataque
                            case INIMIGO_MORRENDO: tpf_current_inimigo = (1.0 / 8.0); frame_total_current_inimigo = 4; break; // tpf_inimigo_morte, frame_total_inimigo_morte
                        }
                    } else { // POLICIAL
                         switch(inimigos[i].estado) {
                            case INIMIGO_PARADO:
                            case INIMIGO_ANDANDO: // Usa a mesma animação parada para andar
                                tpf_current_inimigo = (1.0 / 8.0); // tpf_policial_parado
                                frame_total_current_inimigo = 7; // frame_total_policial_parado
                                break;
                            case INIMIGO_ATACANDO:
                                tpf_current_inimigo = (1.0 / 12.0); // tpf_policial_arremesso
                                frame_total_current_inimigo = 9; // frame_total_policial_arremesso
                                break;
                            case INIMIGO_MORRENDO:
                                tpf_current_inimigo = (1.0 / 8.0); // Usa o tempo por frame do policial (tpf_policial_morte)
                                frame_total_current_inimigo = 4; // Usa o total de frames do policial (frame_total_policial_morte)
                                break;
                        }
                    }
                    inimigos[i].acc_animacao += 1.0 / 60.0;
                    if (inimigos[i].acc_animacao >= tpf_current_inimigo) {
                        inimigos[i].acc_animacao -= tpf_current_inimigo;
                        if (inimigos[i].frame_atual < frame_total_current_inimigo - 1) {
                            inimigos[i].frame_atual++;
                            // NOVO: Lançar granada em um frame específico da animação de ataque do policial
                            if (inimigos[i].type == POLICIAL && inimigos[i].estado == INIMIGO_ATACANDO && inimigos[i].frame_atual == 5) {
                                for(int g = 0; g < MAX_GRANADAS; g++) {
                                    if(!granadas[g].ativa) {
                                        granadas[g].ativa = true;
                                        granadas[g].explodindo = false;
                                        granadas[g].x = inimigos[i].x + (inimigos[i].hitbox_width / 2.0); // Posição no mundo
                                        granadas[g].y = inimigos[i].y + (inimigos[i].hitbox_height / 2.0);
                                        granadas[g].vel_x = -10.0; // Velocidade para a esquerda
                                        granadas[g].vel_y = -8.0; // Lança para cima
                                        break;
                                    }
                                }
                            }
                        } else { // Fim da animação
                            if (inimigos[i].estado == INIMIGO_ATACANDO) {
                                inimigos[i].estado = INIMIGO_PARADO; // Volta a ficar parado
                                inimigos[i].vel_x = inimigo_velocidade_parado_base;
                                inimigos[i].attack_cooldown = 2.0 + (rand() % 2); // Cooldown de 2 a 3 segundos
                            } else if (inimigos[i].estado == INIMIGO_MORRENDO) {
                                inimigos[i].animacao_morte_finalizada = true;
                            } else {
                                inimigos[i].frame_atual = 0; // Reinicia animações de loop
                            }
                        }
                    }
                }

                // Detecção de colisão (Garrafa vs Inimigo)
                if (inimigos[i].estado != INIMIGO_MORRENDO) {
                    for (int j = 0; j < MAX_GARRAFAS; j++) {
                        if (garrafas[j].ativa) {
                            float garrafa_hb_x = garrafas[j].x - garrafa_hitbox_width / 2.0;
                            float garrafa_hb_y = garrafas[j].y - garrafa_hitbox_height / 2.0;

                            if (check_collision(garrafa_hb_x, garrafa_hb_y, garrafa_hitbox_width, garrafa_hitbox_height,
                                                inimigos[i].x + inimigos[i].hitbox_offset_x, inimigo_hb_y, inimigo_hb_w, inimigo_hb_h)) {
                                fprintf(stderr, "COLISÃO! Garrafa %d atingiu Inimigo %d! Iniciando animação de morte.\n", j, i);
                                
                                // NOVO: Toca o som da garrafa quebrando
                                al_play_sample(som_garrafa_quebrando, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                                
                                garrafas[j].ativa = false;

                                inimigos[i].estado = INIMIGO_MORRENDO;
                                inimigos[i].vel_x = 0;
                                inimigos[i].frame_atual = 0;
                                inimigos[i].acc_animacao = 0;
                                inimigos[i].animacao_morte_finalizada = false;
                                break;
                            }
                        }
                    }
                }
                if (inimigos[i].animacao_morte_finalizada) {
                    inimigos[i].ativa = false;
                }
            }
            // NOVO: Colisão Explosão da Granada vs Protagonista
            for (int i = 0; i < MAX_GRANADAS; i++) {
                if (granadas[i].ativa && granadas[i].explodindo && !granadas[i].dano_aplicado && protagonista.estado == PROT_NORMAL && !protagonista.invulneravel) {
                    float explosao_hb_w = frame_largura_explosao * escala_personagens;
                    float explosao_hb_h = frame_altura_explosao * escala_personagens;
                    float explosao_hb_x = (granadas[i].x - deslocamento_x) - explosao_hb_w / 2.0;
                    float explosao_hb_y = granadas[i].y - explosao_hb_h / 2.0;

                    if (check_collision(prot_hb_x, prot_hb_y, prot_hb_w, current_prot_hitbox_height,
                                        explosao_hb_x, explosao_hb_y, explosao_hb_w, explosao_hb_h)) {
                        protagonista.vida -= GRENADE_DAMAGE;
                        granadas[i].dano_aplicado = true;
                        fprintf(stderr, "PROTAGONISTA ATINGIDO PELA GRANADA! Vida: %d\n", protagonista.vida);

                        protagonista.invulneravel = true;
                        protagonista.timer_invulnerabilidade = PROTAGONIST_INVULNERABILITY_DURATION;
                        protagonista.timer_piscar = 0.0;
                        protagonista.visivel = false;

                        if (protagonista.vida <= 0) {
                            protagonista.estado = PROT_MORRENDO;
                            protagonista.frame_atual = 0; // Reinicia o frame para a animação de morte do protagonista
                            protagonista.acc_animacao = 0;
                            protagonista.animacao_morte_completa = false;
                        }
                    }
                }
            }

            // --- NOVO: Colisão Jogador vs Chefe ---
            if (boss.ativa && boss.estado >= BOSS_DEMONIO_IDLE && boss.estado != BOSS_DEMONIO_MORRENDO && boss.invulnerability_timer <= 0) {
                float boss_hb_w = boss.frame_largura_demonio_idle * boss.escala * 0.5; // Hitbox um pouco menor que o sprite
                float boss_hb_h = boss.frame_altura_demonio_idle * boss.escala * 0.8;
                float boss_screen_x = boss.x - deslocamento_x;
                float boss_hb_x = boss_screen_x + (boss.frame_largura_demonio_idle * boss.escala * 0.25);
                float boss_hb_y = boss.y + (boss.frame_altura_demonio_idle * boss.escala * 0.1);

                // Colisão: Garrafa do jogador -> Chefe
                for (int j = 0; j < MAX_GARRAFAS; j++) {
                    if (garrafas[j].ativa) {
                        float garrafa_hb_x = garrafas[j].x - deslocamento_x - garrafa_hitbox_width / 2.0;
                        float garrafa_hb_y = garrafas[j].y - garrafa_hitbox_height / 2.0;
                        if(check_collision(garrafa_hb_x, garrafa_hb_y, garrafa_hitbox_width, garrafa_hitbox_height, boss_hb_x, boss_hb_y, boss_hb_w, boss_hb_h)) {
                            garrafas[j].ativa = false;
                            boss.health -= PLAYER_ATTACK_DAMAGE_TO_BOSS;
                            boss.estado = BOSS_DEMONIO_DANO;
                            boss.frame_atual = 0;
                            boss.acc_animacao = 0;
                            boss.invulnerability_timer = 0.5f; // Meio segundo de invulnerabilidade
                            fprintf(stderr, "DEBUG: Chefe atingido por garrafa! Vida: %d\n", boss.health);
                        }
                    }
                }

                // Colisão: Ataque especial do jogador -> Chefe
                if (protagonista.atacando) {
                    float prot_attack_hb_x_world = protagonista.x + protagonista.prot_attack_hitbox_offset_x;
                    float prot_attack_hb_y_world = protagonista.y + protagonista.prot_attack_hitbox_offset_y;
                    if(check_collision(prot_attack_hb_x_world, prot_attack_hb_y_world, protagonista.prot_attack_hitbox_width, protagonista.prot_attack_hitbox_height, boss_hb_x, boss_hb_y, boss_hb_w, boss_hb_h)) {
                        boss.health -= PLAYER_ATTACK_DAMAGE_TO_BOSS;
                        boss.estado = BOSS_DEMONIO_DANO;
                        boss.frame_atual = 0;
                        boss.acc_animacao = 0;
                        boss.invulnerability_timer = 0.5f;
                        fprintf(stderr, "DEBUG: Chefe atingido por ataque especial! Vida: %d\n", boss.health);
                    }
                }

                // Checa se o chefe morreu
                if(boss.health <= 0) {
                    boss.estado = BOSS_DEMONIO_MORRENDO;
                    boss.frame_atual = 0;
                    boss.acc_animacao = 0;
                }
            }

            // --- NOVO: Colisão Projétil do Chefe vs Jogador ---
            for (int i = 0; i < MAX_BOSS_PROJECTILES; i++) {
                if (boss_projectiles[i].ativa && protagonista.estado == PROT_NORMAL && !protagonista.invulneravel) {
                    float proj_hb_w = boss_proj_largura_original * escala_boss_proj;
                    float proj_hb_h = boss_proj_altura_original * escala_boss_proj;
                    float proj_screen_x = boss_projectiles[i].x - deslocamento_x;

                    if((check_collision(prot_hb_x, prot_hb_y, prot_hb_w, current_prot_hitbox_height, proj_screen_x, boss_projectiles[i].y, proj_hb_w, proj_hb_h)) && !protagonista.agachando) {
                        boss_projectiles[i].ativa = false;
                        protagonista.vida -= BOSS_PROJECTILE_DAMAGE;
                        fprintf(stderr, "JOGADOR ATINGIDO PELO CHEFE! Vida: %d\n", protagonista.vida);

                        protagonista.invulneravel = true;
                        protagonista.timer_invulnerabilidade = PROTAGONIST_INVULNERABILITY_DURATION;
                        protagonista.timer_piscar = 0.0;
                        protagonista.visivel = false;

                        if(protagonista.vida <= 0) {
                            protagonista.estado = PROT_MORRENDO;
                        }
                    }
                }
            }


            // --- Lógica de Colisão Protagonista-Obstáculo e Coleta de Dinheiro ---
            if (protagonista.estado == PROT_NORMAL) {
                bool on_obstacle = false;

                float prot_current_hb_x = protagonista.x + protagonista.prot_hitbox_offset_x;
                float prot_current_hb_y = prot_hb_y;
                float prot_current_hb_w = protagonista.prot_hitbox_width;
                float prot_current_hb_h = current_prot_hitbox_height;

                for (int i = 0; i < MAX_OBSTACULOS; i++) {
                    if (obstaculos[i].ativa) {
                        float obstacle_screen_x = obstaculos[i].x - deslocamento_x;
                        float obstacle_top = obstaculos[i].y;
                        float obstacle_width = obstaculos[i].width;
                        float obstacle_height = obstaculos[i].height;

                        if (check_collision(prot_current_hb_x, prot_current_hb_y, prot_current_hb_w, prot_current_hb_h,
                                            obstacle_screen_x, obstacle_top, obstacle_width, obstacle_height)) {

                            bool should_block_horizontally = !protagonista.pulando && !(obstaculos[i].only_crouch_pass && protagonista.agachando);

                            if (should_block_horizontally) {
                                deslocamento_x = deslocamento_x_anterior; // Reverte o movimento do personagem
                                break;
                            }
                        }
                    }
                }

                for (int i = 0; i < MAX_OBSTACULOS; i++) {
                    if (obstaculos[i].ativa) {
                        float obstacle_screen_x = obstaculos[i].x - deslocamento_x;
                        float obstacle_top = obstaculos[i].y;

                        if (protagonista.pulando && protagonista.vel_y > 0 &&
                            (protagonista.y_chao_atual + protagonista.prot_hitbox_offset_y + prot_current_hb_h) <= obstacle_top &&
                            (protagonista.y + protagonista.prot_hitbox_offset_y + prot_current_hb_h) > obstacle_top &&
                            (prot_current_hb_x + prot_current_hb_w > obstacle_screen_x) &&
                            (prot_current_hb_x < obstacle_screen_x + obstaculos[i].width)) {

                            if (!obstaculos[i].only_crouch_pass) {
                                protagonista.y = obstacle_top - protagonista.prot_hitbox_offset_y - prot_current_hb_h;
                                protagonista.pulando = false;
                                protagonista.vel_y = 0;
                                protagonista.vel_x_pulo = 0;
                                protagonista.y_chao_atual = protagonista.y;
                                on_obstacle = true;
                                break;
                            }
                        }

                        if (!protagonista.pulando && !on_obstacle &&
                            (protagonista.y + protagonista.prot_hitbox_offset_y + prot_current_hb_h > obstacle_top - 5) &&
                            (protagonista.y + protagonista.prot_hitbox_offset_y + prot_current_hb_h < obstacle_top + 5) &&
                            (prot_current_hb_x + prot_current_hb_w > obstacle_screen_x) &&
                            (prot_current_hb_x < obstacle_screen_x + obstaculos[i].width)) {

                            if (!obstaculos[i].only_crouch_pass) {
                                on_obstacle = true;
                                protagonista.y_chao_atual = protagonista.y;
                            }
                        }
                    }
                }

                if (!on_obstacle && !protagonista.pulando && protagonista.y < protagonista.y_base) {
                    protagonista.y_chao_atual = protagonista.y_base;
                }
            }

            if (protagonista.estado == PROT_NORMAL) {
                for (int i = 0; i < MAX_MONEY_NOTES; i++) {
                    if (money_notes[i].ativa) {
                        float money_note_hb_x = money_notes[i].x - deslocamento_x;
                        float money_note_hb_y = money_notes[i].y;
                        float money_note_hb_w = money_notes[i].hitbox_width;
                        float money_note_hb_h = money_notes[i].hitbox_height;

                        if (check_collision(prot_hb_x, prot_hb_y, prot_hb_w, current_prot_hitbox_height,
                                            money_note_hb_x, money_note_hb_y, money_note_hb_w, money_note_hb_h)) {
                            player_money += money_notes[i].value;
                            money_notes[i].ativa = false;
                            fprintf(stderr, "DEBUG: Coletou R$%d! Total: R$%d\n", money_notes[i].value, player_money);
                        }
                    }
                }
            }

            // --- SEÇÃO DE DESENHO ---
            al_clear_to_color(al_map_rgb(0, 0, 0));

            // --- Desenho do Background, Obstáculos, NPC ---
            int offset_x_bg = (int)fmod(deslocamento_x, bg_width);
            if (offset_x_bg < 0) offset_x_bg += bg_width;

            al_draw_scaled_bitmap(background,
                                  offset_x_bg, 0,
                                  bg_width - offset_x_bg, bg_height,
                                  0, 0,
                                  (float)(bg_width - offset_x_bg), (float)ALTURA, 0);

            if ((bg_width - offset_x_bg) < LARGURA) {
                al_draw_scaled_bitmap(background,
                                      0, 0,
                                      LARGURA - (bg_width - offset_x_bg), bg_height,
                                      (float)(bg_width - offset_x_bg), 0,
                                      (float)(LARGURA - (bg_width - offset_x_bg)), (float)ALTURA, 0);
            }

            // --- Desenha Obstáculos ---
            for (int i = 0; i < MAX_OBSTACULOS; i++) {
                if (obstaculos[i].ativa) {
                    float draw_x = obstaculos[i].x - deslocamento_x;
                    al_draw_scaled_bitmap(obstaculos[i].sprite_bitmap,
                                          0, 0,
                                          al_get_bitmap_width(obstaculos[i].sprite_bitmap), al_get_bitmap_height(obstaculos[i].sprite_bitmap),
                                          draw_x, obstaculos[i].y,
                                          obstaculos[i].width, obstaculos[i].height,
                                          0);
                }
            }

            // --- Desenha NPC ---
            if (traficante.ativa) {
                float draw_x = traficante.x - deslocamento_x;
                al_draw_scaled_bitmap(sprite_traficante_parada,
                                      traficante.frame_atual * traficante.largura_sprite, 0,
                                      traficante.largura_sprite, traficante.altura_sprite,
                                      draw_x, traficante.y,
                                      traficante.largura_sprite * escala_traficante, traficante.altura_sprite * escala_traficante,
                                      ALLEGRO_FLIP_HORIZONTAL);
                
                float text_x = draw_x + (traficante.largura_sprite * escala_traficante) / 2.0;
                float text_y = traficante.y + 70;

                al_draw_text(fonte, al_map_rgb(255, 255, 255), text_x, text_y, ALLEGRO_ALIGN_CENTER, "Aperte B para");
                al_draw_text(fonte, al_map_rgb(255, 255, 255), text_x, text_y + al_get_font_line_height(fonte), ALLEGRO_ALIGN_CENTER, "comprar pó");
                al_draw_textf(fonte, al_map_rgb(255, 255, 255), text_x, text_y + al_get_font_line_height(fonte) * 2, ALLEGRO_ALIGN_CENTER, "(+20 HP)");
                al_draw_textf(fonte, al_map_rgb(255, 255, 255), text_x, text_y + al_get_font_line_height(fonte) * 3, ALLEGRO_ALIGN_CENTER, "R$%d", NPC_HEAL_COST);
            }

            // --- NOVO: Desenha a Entrada do Chefe e o Chefe ---
            if (boss.ativa || (boss_defeated_permanently && deslocamento_x < BOSS_SPAWN_X + entrance.width)) { // Desenha a entrada mesmo se o chefe foi derrotado, se estiver na tela
                // Desenha a entrada primeiro (para ficar atrás do chefe)
                al_draw_scaled_bitmap(entrance.sprite_bitmap,
                                      0, 0,
                                      entrance_original_w, entrance_original_h,
                                      entrance.x - deslocamento_x, entrance.y,
                                      entrance.width, entrance.height,
                                      0);
            }

            if (boss.ativa) {
                // Desenha o chefe com base no seu estado atual
                float draw_x = boss.x - deslocamento_x;
                float draw_y = boss.y;
                ALLEGRO_BITMAP* current_boss_sprite = NULL;
                int current_frame_w = 0, current_frame_h = 0;

                switch(boss.estado) {
                    case BOSS_PARADA:
                        current_boss_sprite = boss.sprite_parada;
                        current_frame_w = boss.frame_largura_parada; current_frame_h = boss.frame_altura_parada;
                        break;

                    case BOSS_TRANSFORMANDO:
                        current_boss_sprite = boss.sprite_trans1;
                        current_frame_w = boss.frame_largura_trans1; current_frame_h = boss.frame_altura_trans1;
                        break;
                    
                    // --- NOVO: Desenha a segunda animação de transformação ---
                    case BOSS_TRANSFORMANDO_2:
                        current_boss_sprite = boss.sprite_trans2;
                        current_frame_w = boss.frame_largura_trans2; current_frame_h = boss.frame_altura_trans2;
                        break;

                    // --- NOVO: Desenha o chefe demônio em idle ---
                    case BOSS_DEMONIO_IDLE:
                        current_boss_sprite = boss.sprite_demonio_idle;
                        current_frame_w = boss.frame_largura_demonio_idle; current_frame_h = boss.frame_altura_demonio_idle;
                        break;
                    // --- NOVO: Desenha os novos estados ---
                    case BOSS_DEMONIO_ATACANDO:
                        current_boss_sprite = boss.sprite_demonio_ataque;
                        current_frame_w = boss.frame_largura_demonio_ataque; current_frame_h = boss.frame_altura_demonio_ataque;
                        break;
                    case BOSS_DEMONIO_DANO:
                        current_boss_sprite = boss.sprite_demonio_dano;
                        current_frame_w = boss.frame_largura_demonio_dano; current_frame_h = boss.frame_altura_demonio_dano;
                        break;
                    case BOSS_DEMONIO_MORRENDO:
                        current_boss_sprite = boss.sprite_demonio_morte;
                        current_frame_w = boss.frame_largura_demonio_morte; current_frame_h = boss.frame_altura_demonio_morte;
                        break;
                    default:
                        break;
                }
                if(current_boss_sprite) {
                    al_draw_scaled_bitmap(current_boss_sprite,
                                          boss.frame_atual * current_frame_w, 0,
                                          current_frame_w, current_frame_h,
                                          draw_x, draw_y,
                                          current_frame_w * boss.escala, current_frame_h * boss.escala,
                                          ALLEGRO_FLIP_HORIZONTAL);
                }
            }


            // --- Desenha o Protagonista (AGORA EM UMA LINHA!) ---
            protagonista_desenhar(&protagonista);


            // --- Desenha Garrafas ---
            for (int i = 0; i < MAX_GARRAFAS; i++) {
                if (garrafas[i].ativa) {
                    al_draw_scaled_rotated_bitmap(sprite_garrafa,
                                                  garrafa_largura_original / 2.0, garrafa_altura_original / 2.0,
                                                  garrafas[i].x - deslocamento_x, garrafas[i].y,
                                                  escala_garrafa, escala_garrafa,
                                                  garrafas[i].angulo,
                                                  0);
                }
            }

             // --- NOVO: Desenha Projéteis do Chefe ---
             for (int i = 0; i < MAX_BOSS_PROJECTILES; i++) {
                 if (boss_projectiles[i].ativa) {
                     al_draw_scaled_rotated_bitmap(sprite_demon_projetil,
                                                   boss_proj_largura_original / 2.0, boss_proj_altura_original / 2.0,
                                                   boss_projectiles[i].x - deslocamento_x, boss_projectiles[i].y,
                                                   escala_boss_proj, escala_boss_proj,
                                                   boss_projectiles[i].angulo, 0);
                 }
             }

            // --- Desenha Inimigos (ATUALIZADO) ---
            for (int i = 0; i < MAX_INIMIGOS; i++) {
                if (inimigos[i].ativa) {
                    ALLEGRO_BITMAP *current_enemy_sprite = NULL;
                    int current_frame_largura_inimigo = 0;

                    if (inimigos[i].type == NOIA) {
                        switch(inimigos[i].estado) {
                            case INIMIGO_PARADO: current_enemy_sprite = sprite_inimigo_parado; current_frame_largura_inimigo = frame_largura_inimigo_parado; break;
                            case INIMIGO_ANDANDO: current_enemy_sprite = sprite_inimigo_andando; current_frame_largura_inimigo = frame_largura_inimigo_andando; break;
                            case INIMIGO_ATACANDO: current_enemy_sprite = sprite_inimigo_ataque; current_frame_largura_inimigo = frame_largura_inimigo_ataque; break;
                            case INIMIGO_MORRENDO: current_enemy_sprite = sprite_inimigo_morte; current_frame_largura_inimigo = frame_largura_inimigo_morte; break;
                        }
                    } else { // POLICIAL
                        switch(inimigos[i].estado) {
                            case INIMIGO_PARADO:
                            case INIMIGO_ANDANDO:
                                current_enemy_sprite = sprite_policial_parado;
                                current_frame_largura_inimigo = frame_largura_policial_parado;
                                break;
                            case INIMIGO_ATACANDO:
                                current_enemy_sprite = sprite_policial_arremesso;
                                current_frame_largura_inimigo = frame_largura_policial_arremesso;
                                break;
                            case INIMIGO_MORRENDO:
                                current_enemy_sprite = sprite_policial_morte; // Usa o sprite de morte do policial
                                current_frame_largura_inimigo = frame_largura_policial_morte; // Usa a largura do frame do policial
                                break;
                        }
                    }

                    if (current_enemy_sprite) {
                        al_draw_scaled_bitmap(current_enemy_sprite,
                                              inimigos[i].frame_atual * current_frame_largura_inimigo, 0,
                                              current_frame_largura_inimigo, inimigo_altura_sprite,
                                              inimigos[i].x - deslocamento_x, inimigos[i].y,
                                              current_frame_largura_inimigo * escala_personagens, inimigo_altura_sprite * escala_personagens,
                                              ALLEGRO_FLIP_HORIZONTAL);
                    }
                }
            }

            // --- NOVO: Desenha Granadas e Explosões ---
            for (int i = 0; i < MAX_GRANADAS; i++) {
                if (granadas[i].ativa) {
                    if (granadas[i].explodindo) {
                        float draw_x = (granadas[i].x - deslocamento_x) - (frame_largura_explosao * escala_personagens / 2.0);
                        float draw_y = granadas[i].y - (frame_altura_explosao * escala_personagens / 2.0);
                        al_draw_scaled_bitmap(sprite_explosao,
                                              granadas[i].frame_explosao * frame_largura_explosao, 0,
                                              frame_largura_explosao, frame_altura_explosao,
                                              draw_x, draw_y,
                                              frame_largura_explosao * escala_personagens, frame_altura_explosao * escala_personagens,
                                              0);
                    } else {
                        // Desenha a granada em si como um círculo
                        al_draw_filled_circle(granadas[i].x - deslocamento_x, granadas[i].y, 8, al_map_rgb(50, 80, 50));
                        al_draw_circle(granadas[i].x - deslocamento_x, granadas[i].y, 8, al_map_rgb(20, 30, 20), 2);
                    }
                }
            }

            // --- Desenha Notas de Dinheiro, HUD (Vida, Dinheiro) ---
            for (int i = 0; i < MAX_MONEY_NOTES; i++) {
                if (money_notes[i].ativa) {
                    al_draw_scaled_bitmap(money_notes[i].sprite_bitmap,
                                          0, 0,
                                          al_get_bitmap_width(money_notes[i].sprite_bitmap), al_get_bitmap_height(money_notes[i].sprite_bitmap),
                                          money_notes[i].x - deslocamento_x, money_notes[i].y,
                                          money_notes[i].hitbox_width, money_notes[i].hitbox_height,
                                          0);
                }
            }

            float health_bar_width = (float)protagonista.vida / (MAX_PROTAGONIST_HEALTH + NPC_HEAL_AMOUNT) * 200;
            if (health_bar_width < 0) health_bar_width = 0;
            ALLEGRO_COLOR health_color = al_map_rgb(0, 255, 0);
            if (protagonista.vida > MAX_PROTAGONIST_HEALTH) {
                health_color = al_map_rgb(0, 255, 255);
            }
            al_draw_filled_rectangle(10, 10, 10 + health_bar_width, 30, health_color);
            al_draw_rectangle(10, 10, 10 + 200, 30, al_map_rgb(255, 255, 255), 2);
            al_draw_textf(fonte, al_map_rgb(255, 255, 255), 220, 15, 0, "HP: %d", protagonista.vida);

            al_draw_textf(fonte, al_map_rgb(255, 255, 0), LARGURA - 10, 10, ALLEGRO_ALIGN_RIGHT, "R$ %d", player_money);

            // --- NOVO: Desenha a Barra de Vida do Chefe ---
            if (boss.ativa && boss.estado >= BOSS_DEMONIO_IDLE) {
                float boss_health_bar_w = 400;
                float boss_health_bar_h = 25;
                float boss_health_bar_x = LARGURA / 2 - boss_health_bar_w / 2;
                float boss_health_bar_y = 20;

                float current_health_w = ((float)boss.health / MAX_BOSS_HEALTH) * boss_health_bar_w;
                if(current_health_w < 0) current_health_w = 0;

                // Barra de fundo
                al_draw_filled_rectangle(boss_health_bar_x, boss_health_bar_y, boss_health_bar_x + boss_health_bar_w, boss_health_bar_y + boss_health_bar_h, al_map_rgb(50, 0, 0));
                // Barra de vida atual
                al_draw_filled_rectangle(boss_health_bar_x, boss_health_bar_y, boss_health_bar_x + current_health_w, boss_health_bar_y + boss_health_bar_h, al_map_rgb(200, 0, 50));
                // Borda
                al_draw_rectangle(boss_health_bar_x, boss_health_bar_y, boss_health_bar_x + boss_health_bar_w, boss_health_bar_y + boss_health_bar_h, al_map_rgb(255, 255, 255), 2);
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 2, boss_health_bar_y + boss_health_bar_h + 5, ALLEGRO_ALIGN_CENTER, "Lilith, a Profana");
            }


            al_flip_display();
        }
    }

    // --- Limpeza de Recursos (Liberação de Memória) ---
    if (instancia_som_verdebalaraio) {
        al_stop_sample_instance(instancia_som_verdebalaraio);
        al_destroy_sample_instance(instancia_som_verdebalaraio);
    }
    if (som_verdebalaraio) al_destroy_sample(som_verdebalaraio);
    if (som_garrafa_quebrando) al_destroy_sample(som_garrafa_quebrando); // NOVO: Libera o som da garrafa

    al_destroy_bitmap(background);
    // Destruir os bitmaps do protagonista que foram carregados em prot_sprites
    for(int i=0; i<9; i++) {
        al_destroy_bitmap(prot_sprites[i]);
    }
    al_destroy_bitmap(sprite_garrafa);
    al_destroy_bitmap(sprite_inimigo_parado);
    al_destroy_bitmap(sprite_inimigo_andando);
    al_destroy_bitmap(sprite_inimigo_morte);
    al_destroy_bitmap(sprite_inimigo_ataque);
    al_destroy_bitmap(sprite_traficante_parada);
    al_destroy_bitmap(sprite_sacos_lixo);
    al_destroy_bitmap(sprite_placa_radar);
    al_destroy_bitmap(sprite_2reais);
    al_destroy_bitmap(sprite_5reais);
    al_destroy_bitmap(sprite_10reais);
    al_destroy_bitmap(sprite_policial_parado);
    al_destroy_bitmap(sprite_policial_arremesso);
    al_destroy_bitmap(sprite_explosao);
    al_destroy_bitmap(sprite_policial_morte);
    // NOVO: Limpa os recursos do chefe
    al_destroy_bitmap(sprite_gatopreto);
    al_destroy_bitmap(sprite_prostituta_parada);
    al_destroy_bitmap(sprite_prostituta_trans1);
    al_destroy_bitmap(sprite_demon_trans2);  // Libera o novo sprite de transformação
    al_destroy_bitmap(sprite_demon_parado); // Libera o sprite de demônio idle
    // NOVO: Limpa os recursos dos novos sprites de combate do chefe
    al_destroy_bitmap(sprite_demon_ataque);
    al_destroy_bitmap(sprite_demon_dano);
    al_destroy_bitmap(sprite_demon_morte);
    al_destroy_bitmap(sprite_demon_projetil);

    al_destroy_timer(timer);

    if (protagonista.estado == PROT_MORRENDO && protagonista.animacao_morte_completa) {
        should_restart = game_over_screen(janela, fila, fonte);
    } else if (boss_defeated_permanently) { // Se o chefe foi derrotado, mostra a tela de vitória
        should_restart = victory_screen(janela, fila, fonte);
    }

    return should_restart;
}




int main() {
    if (!al_init()) {
        fprintf(stderr, "Falha ao inicializar Allegro.\n");
        return -1;
    }
    if (!al_install_keyboard()) {
        fprintf(stderr, "Falha ao inicializar teclado.\n");
        return -1;
    }

    al_init_image_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_video_addon();
    al_init_primitives_addon();

    if (!al_install_audio()) {
        fprintf(stderr, "Falha ao inicializar o sistema de áudio.\n");
        return -1;
    }

    if (!al_init_acodec_addon()) {
        fprintf(stderr, "Falha ao inicializar codecs de áudio.\n");
        return -1;
    }

    if (!al_reserve_samples(1)) {
        fprintf(stderr, "Falha ao reservar canais de áudio.\n");
        return -1;
    }

    ALLEGRO_DISPLAY *janela = al_create_display(LARGURA, ALTURA);
    if (!janela) {
        fprintf(stderr, "Falha ao criar janela.\n");
        return -1;
    }

    ALLEGRO_EVENT_QUEUE *fila = al_create_event_queue();
    if (!fila) {
        fprintf(stderr, "Falha ao criar fila de eventos.\n");
        al_destroy_display(janela);
        return -1;
    }

    ALLEGRO_FONT *fonte = al_load_ttf_font("arial.ttf", 36, 0);
    if (!fonte) {
        al_show_native_message_box(janela, "Erro", "Fonte", "Não foi possível carregar a fonte arial.ttf", NULL, 0);
        al_destroy_event_queue(fila);
        al_destroy_display(janela);
        return -1;
    }

    al_register_event_source(fila, al_get_keyboard_event_source());
    al_register_event_source(fila, al_get_display_event_source(janela));

// === TELA DE LOAD ===
ALLEGRO_BITMAP *load_img = al_load_bitmap("loadscreen.png");
if (!load_img) {
    al_show_native_message_box(janela, "Erro", "Imagem", "Não foi possível carregar loadscreen.png", NULL, 0);
    al_destroy_font(fonte);
    al_destroy_event_queue(fila);
    al_destroy_display(janela);
    return -1;
}

// Desenha a imagem de load
al_draw_scaled_bitmap(load_img, 0, 0,
                     al_get_bitmap_width(load_img),
                     al_get_bitmap_height(load_img),
                     0, 0,
                     LARGURA, ALTURA,
                     0);

// Desenha a mensagem em branco, centralizada
const char *msg = "Aperte qualquer tecla para iniciar";
al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA/2, ALTURA - 100, ALLEGRO_ALIGN_CENTER, msg);

al_flip_display();

esperar_tecla(fila);

al_destroy_bitmap(load_img);


    // === MENU COM VÍDEO DE FUNDO ===
    ALLEGRO_VIDEO *video = al_open_video("fundo_menu.ogv");
    if (!video) {
        al_show_native_message_box(janela, "Erro", "Vídeo", "Erro ao carregar vídeo!", NULL, 0);
        al_destroy_font(fonte);
        al_destroy_event_queue(fila);
        al_destroy_display(janela);
        return -1;
    }

    al_register_event_source(fila, al_get_video_event_source(video));

    al_start_video(video, al_get_default_mixer());

    const char *opcoes[NUM_OPCOES] = {
        "Iniciar jogo",
        "Configuracoes",
        "Sair"
    };
    int opcao_selecionada = 0;

    bool sair_menu = false;
    while (!sair_menu) {
        ALLEGRO_EVENT ev;
        while (al_get_next_event(fila, &ev)) {
            if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
                switch (ev.keyboard.keycode) {
                    case ALLEGRO_KEY_UP:
                        opcao_selecionada--;
                        if (opcao_selecionada < 0) opcao_selecionada = NUM_OPCOES - 1;
                        break;
                    case ALLEGRO_KEY_DOWN:
                        opcao_selecionada++;
                        if (opcao_selecionada >= NUM_OPCOES) opcao_selecionada = 0;
                        break;
                    case ALLEGRO_KEY_ENTER:
                        if (opcao_selecionada == 2) { // Sair
                            sair_menu = true;
                        } else if (opcao_selecionada == 0) { // Iniciar jogo
                            al_close_video(video); // Fecha o vídeo antes de iniciar o jogo
                            bool restart_game = true;
                            while (restart_game) { // Loop para reiniciar o jogo
                                restart_game = jogo(janela, fila, fonte);
                                if (!restart_game) { // Se não for para reiniciar, sai do loop
                                    sair_menu = true; // Sai do menu também
                                    break;
                                }
                            }
                            if (sair_menu) break; // Se saiu do menu, quebre o loop externo também
                            al_start_video(video, al_get_default_mixer()); // Reinicia vídeo do menu se o jogo não foi encerrado
                        } else {
                            printf("Opção selecionada: %s\n", opcoes[opcao_selecionada]);
                        }
                        break;
                    case ALLEGRO_KEY_ESCAPE:
                        sair_menu = true;
                        break;
                }
            } else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
                sair_menu = true;
            } else if (ev.type == ALLEGRO_EVENT_VIDEO_FINISHED) {
                // Reinicia o vídeo para dar loop
                al_seek_video(video, 0.0);
                al_start_video(video, al_get_default_mixer());
            }
        }

        ALLEGRO_BITMAP *frame = al_get_video_frame(video);
        if (frame) {
            desenhar_video_proporcional(frame);
        }

        // Desenha menu por cima
        int base_y = ALTURA / 2 - (NUM_OPCOES * 40) / 2;
        for (int i = 0; i < NUM_OPCOES; i++) {
            if (i == opcao_selecionada) {
                al_draw_text(fonte, al_map_rgb(255, 165, 0), LARGURA / 2, base_y + i * 50, ALLEGRO_ALIGN_CENTER, opcoes[i]);
            } else {
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 2, base_y + i * 50, ALLEGRO_ALIGN_CENTER, opcoes[i]);
            }
        }

        al_flip_display();
        al_rest(0.01);
    }

    al_close_video(video);

    al_destroy_font(fonte);
    al_destroy_event_queue(fila);
    al_destroy_display(janela);

    return 0;
}