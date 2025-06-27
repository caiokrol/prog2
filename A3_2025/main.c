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

// --- Novas Definições para o Chefe ---
#define BOSS_SPAWN_X 1500.0f
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
    BOSS_INVISIVEL,     // O chefe ainda não apareceu
    BOSS_PARADA,        // A forma de prostituta, parada
    BOSS_TRANSFORMANDO, // A primeira animação de transformação
    BOSS_TRANSFORMANDO_2, // A segunda animação de transformação
    BOSS_DEMONIO_IDLE,  // O chefe transformado, em modo de espera
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

#define NUM_OPCOES 4 // NUM OPCOES MENU

const int VIDEO_LARGURA = 1920;
const int VIDEO_ALTURA = 1080;

// --- Estrutura para propriedades das garrafas ---
typedef struct {
    float x, y;
    float vel_x;
    float angulo;
    bool ativa;
} Garrafa;

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
typedef enum {
    PROT_NORMAL,  // Estado normal (parado, andando, pulando, agachando, atacando, arremessando)
    PROT_MORRENDO  // Estado de morte do protagonista
} ProtagonistState;

// Estrutura para propriedades do inimigo (ATUALIZADA)
typedef struct {
    float x, y;    // Posição do inimigo no MUNDO (não na TELA)
    float vel_x;    // Velocidade horizontal RELATIVA AO MUNDO
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
#define NPC_TRAFICANTE_SCALE 1.7 // Escala de desenho do traficante, usada para hitbox e posicionamento

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

// --- Estrutura para armazenar todos os recursos do jogo ---
typedef struct {
    ALLEGRO_BITMAP *background;
    ALLEGRO_BITMAP *sprite_parado;
    ALLEGRO_BITMAP *sprite_andando;
    ALLEGRO_BITMAP *sprite_correndo;
    ALLEGRO_BITMAP *sprite_pulando;
    ALLEGRO_BITMAP *sprite_agachado;
    ALLEGRO_BITMAP *sprite_especial;
    ALLEGRO_BITMAP *sprite_ataque1;
    ALLEGRO_BITMAP *sprite_arremesso;
    ALLEGRO_BITMAP *sprite_garrafa;
    ALLEGRO_BITMAP *sprite_personagem_morte;
    ALLEGRO_BITMAP *sprite_inimigo_parado;
    ALLEGRO_BITMAP *sprite_inimigo_andando;
    ALLEGRO_BITMAP *sprite_inimigo_morte;
    ALLEGRO_BITMAP *sprite_inimigo_ataque;
    ALLEGRO_BITMAP *sprite_policial_parado;
    ALLEGRO_BITMAP *sprite_policial_arremesso;
    ALLEGRO_BITMAP *sprite_explosao;
    ALLEGRO_BITMAP *sprite_policial_morte;
    ALLEGRO_BITMAP *sprite_traficante_parada;
    ALLEGRO_BITMAP *sprite_sacos_lixo;
    ALLEGRO_BITMAP *sprite_placa_radar;
    ALLEGRO_BITMAP *sprite_2reais;
    ALLEGRO_BITMAP *sprite_5reais;
    ALLEGRO_BITMAP *sprite_10reais;
    ALLEGRO_BITMAP *sprite_gatopreto;
    ALLEGRO_BITMAP *sprite_prostituta_parada;
    ALLEGRO_BITMAP *sprite_prostituta_trans1;
    ALLEGRO_BITMAP *sprite_demon_trans2;
    ALLEGRO_BITMAP *sprite_demon_parado;
    ALLEGRO_BITMAP *sprite_demon_ataque;
    ALLEGRO_BITMAP *sprite_demon_dano;
    ALLEGRO_BITMAP *sprite_demon_morte;
    ALLEGRO_BITMAP *sprite_demon_projetil;
    ALLEGRO_SAMPLE *som_verdebalaraio;
    ALLEGRO_SAMPLE_INSTANCE *instancia_som_verdebalaraio;
} GameResources;

// --- Estrutura para as dimensões dos sprites do personagem ---
typedef struct {
    int frame_altura;
    int frame_total_parado;
    int frame_total_andando;
    int frame_total_correndo;
    int frame_total_pulando;
    int frame_total_agachado;
    int frame_total_especial;
    int frame_total_ataque1;
    int frame_total_arremesso;
    int frame_total_personagem_morte;
    int frame_total_inimigo_parado;
    int frame_total_inimigo_andando;
    int frame_total_inimigo_morte;
    int frame_total_inimigo_ataque;
    int frame_total_policial_parado;
    int frame_total_policial_arremesso;
    int frame_total_explosao;
    int frame_total_policial_morte;

    int frame_largura_parado;
    int frame_largura_andando;
    int frame_largura_correndo;
    int frame_largura_pulando;
    int frame_largura_agachado;
    int frame_largura_especial;
    int frame_largura_ataque1;
    int frame_largura_arremesso;
    int frame_largura_personagem_morte;
    int frame_largura_inimigo_parado;
    int frame_largura_inimigo_andando;
    int frame_largura_inimigo_morte;
    int frame_largura_inimigo_ataque;
    int frame_largura_policial_parado;
    int frame_largura_policial_arremesso;
    int frame_largura_policial_morte;
    int frame_largura_explosao;
    int frame_altura_explosao;

    int inimigo_largura_sprite_max;
    int inimigo_altura_sprite;
} SpriteDimensions;

// --- Estrutura para as propriedades de combate do protagonista ---
typedef struct {
    int health;
    bool invulnerable;
    float invulnerability_timer;
    float blink_timer;
    bool visible;
    ProtagonistState estado;
    bool animacao_morte_finalizada;
    int personagem_frame_morte;
    float personagem_acc_morte;
    float tpf_personagem_morte;
} ProtagonistCombatState;

// --- Estrutura para as propriedades de movimento do protagonista ---
typedef struct {
    float x, y;
    float vel_y; // vel_x_pulo será usado para movimento horizontal durante o pulo
    float gravidade;
    float velocidade_andar;
    float velocidade_correr;
    float velocidade;
    float escala_personagens;
    float personagem_y_base;
    float current_ground_y;
    bool pulando, agachando, especial_ativo, especial_finalizado, atacando, arremessando;
    bool crouch_animation_finished;

    int frame_parado, frame_andando, frame_correndo, frame_pulando, frame_agachado, frame_especial, frame_ataque1, frame_arremesso;
    float acc_parado, acc_andando, acc_correndo, acc_pulando, acc_agachado, acc_especial, acc_ataque1, acc_arremesso;
    float tpf_parado, tpf_andando, tpf_correndo, tpf_pulando, tpf_agachado, tpf_especial, tpf_ataque1, tpf_arremesso;

    float prot_hitbox_offset_x;
    float prot_hitbox_offset_y;
    float prot_hitbox_width;
    float prot_hitbox_height;
    float prot_crouch_hitbox_height;
    float prot_attack_hitbox_offset_x;
    float prot_attack_hitbox_offset_y;
    float prot_attack_hitbox_width;
    float prot_attack_hitbox_height;

    float vel_x_pulo; // Velocidade horizontal aplicada durante o pulo
} ProtagonistMovementState;

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

// --- Funções "Modularizadas" ---

/**
 * @brief Inicializa todos os recursos (bitmaps, áudios) do jogo.
 * @param resources Ponteiro para a estrutura GameResources para armazenar os recursos.
 * @return true se todos os recursos foram carregados com sucesso, false caso contrário.
 */
bool inicializar_jogo_recursos(GameResources *resources) {
    resources->background = al_load_bitmap("background.png");
    resources->sprite_parado = al_load_bitmap("personagem.png");
    resources->sprite_andando = al_load_bitmap("personagem_andando.png");
    resources->sprite_correndo = al_load_bitmap("personagem_correndo.png");
    resources->sprite_pulando = al_load_bitmap("personagem_pulando.png");
    resources->sprite_agachado = al_load_bitmap("personagem_agachado.png");
    resources->sprite_especial = al_load_bitmap("personagem_especial.png");
    resources->sprite_ataque1 = al_load_bitmap("personagem_ataque1.png");
    resources->sprite_arremesso = al_load_bitmap("personagem_arremessando.png");
    resources->sprite_garrafa = al_load_bitmap("garrafa.png");
    resources->sprite_personagem_morte = al_load_bitmap("personagem_morte.png");
    resources->sprite_inimigo_parado = al_load_bitmap("noia1_parado.png");
    resources->sprite_inimigo_andando = al_load_bitmap("noia1_andando.png");
    resources->sprite_inimigo_morte = al_load_bitmap("noia1_morte.png");
    resources->sprite_inimigo_ataque = al_load_bitmap("noia1_ataque.png");
    resources->sprite_policial_parado = al_load_bitmap("policial_parado.png");
    resources->sprite_policial_arremesso = al_load_bitmap("policial_arremesso.png");
    resources->sprite_explosao = al_load_bitmap("explosao.png");
    resources->sprite_policial_morte = al_load_bitmap("policial_morte.png");
    resources->sprite_traficante_parada = al_load_bitmap("traficante_parada.png");
    resources->sprite_sacos_lixo = al_load_bitmap("sacos_lixo.png");
    resources->sprite_placa_radar = al_load_bitmap("placa_radar.png");
    resources->sprite_2reais = al_load_bitmap("2reais.png");
    resources->sprite_5reais = al_load_bitmap("5reais.png");
    resources->sprite_10reais = al_load_bitmap("10reais.png");
    resources->sprite_gatopreto = al_load_bitmap("gatopreto.png");
    resources->sprite_prostituta_parada = al_load_bitmap("prostituta_parada.png");
    resources->sprite_prostituta_trans1 = al_load_bitmap("prostituta_trans1.png");
    resources->sprite_demon_trans2 = al_load_bitmap("demon_trans2.png");
    resources->sprite_demon_parado = al_load_bitmap("demon_parado.png");
    resources->sprite_demon_ataque = al_load_bitmap("demon_ataque.png");
    resources->sprite_demon_dano = al_load_bitmap("demon_dano.png");
    resources->sprite_demon_morte = al_load_bitmap("demon_morte.png");
    resources->sprite_demon_projetil = al_load_bitmap("demon_projetil.png");
    resources->som_verdebalaraio = al_load_sample("verdebalaraio.ogg");

    if (!resources->background || !resources->sprite_parado || !resources->sprite_andando || !resources->sprite_correndo || !resources->sprite_pulando ||
        !resources->sprite_agachado || !resources->sprite_especial || !resources->sprite_ataque1 || !resources->sprite_arremesso || !resources->sprite_garrafa ||
        !resources->sprite_inimigo_parado || !resources->sprite_inimigo_andando || !resources->sprite_inimigo_morte || !resources->sprite_inimigo_ataque ||
        !resources->sprite_personagem_morte || !resources->sprite_traficante_parada || !resources->som_verdebalaraio || !resources->sprite_sacos_lixo ||
        !resources->sprite_placa_radar || !resources->sprite_2reais || !resources->sprite_5reais || !resources->sprite_10reais ||
        !resources->sprite_policial_parado || !resources->sprite_policial_arremesso || !resources->sprite_explosao || !resources->sprite_policial_morte ||
        !resources->sprite_gatopreto || !resources->sprite_prostituta_parada || !resources->sprite_prostituta_trans1 ||
        !resources->sprite_demon_trans2 || !resources->sprite_demon_parado || !resources->sprite_demon_ataque || !resources->sprite_demon_dano ||
        !resources->sprite_demon_morte || !resources->sprite_demon_projetil) {
        fprintf(stderr, "ERRO CRÍTICO: Falha ao carregar um ou mais bitmaps/áudios! Verifique nomes/caminhos dos arquivos.\n");
        return false;
    }

    resources->instancia_som_verdebalaraio = al_create_sample_instance(resources->som_verdebalaraio);
    if (!resources->instancia_som_verdebalaraio) {
        fprintf(stderr, "ERRO CRÍTICO: Falha ao criar instancia_som_verdebalaraio!\n");
        return false;
    }
    al_set_sample_instance_playmode(resources->instancia_som_verdebalaraio, ALLEGRO_PLAYMODE_LOOP);
    al_attach_sample_instance_to_mixer(resources->instancia_som_verdebalaraio, al_get_default_mixer());
    al_set_sample_instance_gain(resources->instancia_som_verdebalaraio, 0.0);
    al_play_sample_instance(resources->instancia_som_verdebalaraio);
    return true;
}

/**
 * @brief Libera todos os recursos (bitmaps, áudios) do jogo.
 * @param resources Ponteiro para a estrutura GameResources contendo os recursos a serem liberados.
 */
void liberar_jogo_recursos(GameResources *resources) {
    if (resources->instancia_som_verdebalaraio) {
        al_stop_sample_instance(resources->instancia_som_verdebalaraio);
        al_destroy_sample_instance(resources->instancia_som_verdebalaraio);
    }
    if (resources->som_verdebalaraio) al_destroy_sample(resources->som_verdebalaraio);

    al_destroy_bitmap(resources->background);
    al_destroy_bitmap(resources->sprite_parado);
    al_destroy_bitmap(resources->sprite_andando);
    al_destroy_bitmap(resources->sprite_correndo);
    al_destroy_bitmap(resources->sprite_pulando);
    al_destroy_bitmap(resources->sprite_agachado);
    al_destroy_bitmap(resources->sprite_especial);
    al_destroy_bitmap(resources->sprite_ataque1);
    al_destroy_bitmap(resources->sprite_arremesso);
    al_destroy_bitmap(resources->sprite_garrafa);
    al_destroy_bitmap(resources->sprite_inimigo_parado);
    al_destroy_bitmap(resources->sprite_inimigo_andando);
    al_destroy_bitmap(resources->sprite_inimigo_morte);
    al_destroy_bitmap(resources->sprite_inimigo_ataque);
    al_destroy_bitmap(resources->sprite_personagem_morte);
    al_destroy_bitmap(resources->sprite_traficante_parada);
    al_destroy_bitmap(resources->sprite_sacos_lixo);
    al_destroy_bitmap(resources->sprite_placa_radar);
    al_destroy_bitmap(resources->sprite_2reais);
    al_destroy_bitmap(resources->sprite_5reais);
    al_destroy_bitmap(resources->sprite_10reais);
    al_destroy_bitmap(resources->sprite_policial_parado);
    al_destroy_bitmap(resources->sprite_policial_arremesso);
    al_destroy_bitmap(resources->sprite_explosao);
    al_destroy_bitmap(resources->sprite_policial_morte);
    al_destroy_bitmap(resources->sprite_gatopreto);
    al_destroy_bitmap(resources->sprite_prostituta_parada);
    al_destroy_bitmap(resources->sprite_prostituta_trans1);
    al_destroy_bitmap(resources->sprite_demon_trans2);
    al_destroy_bitmap(resources->sprite_demon_parado);
    al_destroy_bitmap(resources->sprite_demon_ataque);
    al_destroy_bitmap(resources->sprite_demon_dano);
    al_destroy_bitmap(resources->sprite_demon_morte);
    al_destroy_bitmap(resources->sprite_demon_projetil);
}

/**
 * @brief Inicializa as dimensões e propriedades dos sprites.
 * @param dims Ponteiro para a estrutura SpriteDimensions.
 * @param resources Ponteiro para a estrutura GameResources (para obter larguras e alturas).
 */
void inicializar_dimensoes_sprites(SpriteDimensions *dims, const GameResources *resources) {
    dims->frame_altura = 128; // Altura padrão

    // Larguras de frame do personagem
    dims->frame_total_parado = 6; dims->frame_largura_parado = 768 / dims->frame_total_parado;
    dims->frame_total_andando = 8; dims->frame_largura_andando = 1024 / dims->frame_total_andando;
    dims->frame_total_correndo = 8; dims->frame_largura_correndo = 1024 / dims->frame_total_correndo;
    dims->frame_total_pulando = 16; dims->frame_largura_pulando = 2048 / dims->frame_total_pulando;
    dims->frame_total_agachado = 8; dims->frame_largura_agachado = 1024 / dims->frame_total_agachado;
    dims->frame_total_especial = 13; dims->frame_largura_especial = 1664 / dims->frame_total_especial;
    dims->frame_total_ataque1 = 5; dims->frame_largura_ataque1 = 640 / dims->frame_total_ataque1;
    dims->frame_total_arremesso = 4; dims->frame_largura_arremesso = 512 / dims->frame_total_arremesso;
    dims->frame_total_personagem_morte = 4;
    dims->frame_largura_personagem_morte = al_get_bitmap_width(resources->sprite_personagem_morte) / dims->frame_total_personagem_morte;

    // Larguras de frame do Noia
    dims->frame_total_inimigo_parado = 7; dims->frame_largura_inimigo_parado = al_get_bitmap_width(resources->sprite_inimigo_parado) / dims->frame_total_inimigo_parado;
    dims->frame_total_inimigo_andando = 8; dims->frame_largura_inimigo_andando = al_get_bitmap_width(resources->sprite_inimigo_andando) / dims->frame_total_inimigo_andando;
    dims->frame_total_inimigo_morte = 4; dims->frame_largura_inimigo_morte = al_get_bitmap_width(resources->sprite_inimigo_morte) / dims->frame_total_inimigo_morte;
    dims->frame_total_inimigo_ataque = 4; dims->frame_largura_inimigo_ataque = al_get_bitmap_width(resources->sprite_inimigo_ataque) / dims->frame_total_inimigo_ataque;

    // Larguras de frame do Policial e Explosão
    dims->frame_total_policial_parado = 7; dims->frame_largura_policial_parado = al_get_bitmap_width(resources->sprite_policial_parado) / dims->frame_total_policial_parado;
    dims->frame_total_policial_arremesso = 9; dims->frame_largura_policial_arremesso = al_get_bitmap_width(resources->sprite_policial_arremesso) / dims->frame_total_policial_arremesso;
    dims->frame_total_policial_morte = 4; dims->frame_largura_policial_morte = al_get_bitmap_width(resources->sprite_policial_morte) / dims->frame_total_policial_morte;
    
    // A largura de um frame de explosão é a largura total do bitmap dividido pelo número total de frames.
    dims->frame_total_explosao = 9; // Certifique-se que este valor está correto para o seu sprite!
    dims->frame_largura_explosao = al_get_bitmap_width(resources->sprite_explosao) / dims->frame_total_explosao;
    // A altura de um frame de explosão é a altura total do bitmap (assumindo frames em uma única linha horizontal)
    dims->frame_altura_explosao = al_get_bitmap_height(resources->sprite_explosao);

    dims->inimigo_largura_sprite_max = (dims->frame_largura_inimigo_parado > dims->frame_largura_inimigo_andando) ? dims->frame_largura_inimigo_parado : dims->frame_largura_inimigo_andando;
    if (dims->frame_largura_inimigo_morte > dims->inimigo_largura_sprite_max) {
        dims->inimigo_largura_sprite_max = dims->frame_largura_inimigo_morte;
    }
    if (dims->frame_largura_inimigo_ataque > dims->inimigo_largura_sprite_max) {
        dims->inimigo_largura_sprite_max = dims->frame_largura_inimigo_ataque;
    }
    if (dims->frame_largura_policial_parado > dims->inimigo_largura_sprite_max) {
        dims->inimigo_largura_sprite_max = dims->frame_largura_policial_parado;
    }
    if (dims->frame_largura_policial_arremesso > dims->inimigo_largura_sprite_max) {
        dims->inimigo_largura_sprite_max = dims->frame_largura_policial_arremesso;
    }
    dims->inimigo_altura_sprite = dims->frame_altura;
}

/**
 * @brief Inicializa o estado de combate do protagonista.
 * @param combat_state Ponteiro para a estrutura ProtagonistCombatState.
 * @param dims Ponteiro para SpriteDimensions (para tpf_personagem_morte).
 */
void inicializar_protagonista_combate(ProtagonistCombatState *combat_state, const SpriteDimensions *dims) {
    combat_state->estado = PROT_NORMAL;
    combat_state->health = MAX_PROTAGONIST_HEALTH;
    combat_state->invulnerable = false;
    combat_state->invulnerability_timer = 0.0;
    combat_state->blink_timer = 0.0;
    combat_state->visible = true;
    combat_state->personagem_frame_morte = 0;
    combat_state->personagem_acc_morte = 0;
    combat_state->tpf_personagem_morte = 1.0 / 8.0;
    combat_state->animacao_morte_finalizada = false;
}

/**
 * @brief Inicializa o estado de movimento e animação do protagonista.
 * @param movement_state Ponteiro para a estrutura ProtagonistMovementState.
 * @param dims Ponteiro para SpriteDimensions.
 */
void inicializar_protagonista_movimento_animacao(ProtagonistMovementState *movement_state, const SpriteDimensions *dims) {
    movement_state->velocidade_andar = 3.0;
    movement_state->velocidade_correr = 6.0;
    movement_state->velocidade = movement_state->velocidade_andar;
    movement_state->escala_personagens = 3.0;
    movement_state->gravidade = 0.5;

    movement_state->x = LARGURA / 2.0 - (dims->frame_largura_correndo * movement_state->escala_personagens) / 2.0;
    movement_state->personagem_y_base = (ALTURA - 300) - (dims->frame_altura * movement_state->escala_personagens) / 2.0;
    movement_state->y = movement_state->personagem_y_base;
    movement_state->current_ground_y = movement_state->personagem_y_base;

    movement_state->pulando = false;
    movement_state->agachando = false;
    movement_state->especial_ativo = false;
    movement_state->especial_finalizado = false;
    movement_state->atacando = false;
    movement_state->arremessando = false;
    movement_state->crouch_animation_finished = false;
    movement_state->vel_y = 0.0;
    movement_state->vel_x_pulo = 0.0;

    // Animações
    movement_state->frame_parado = 0; movement_state->acc_parado = 0; movement_state->tpf_parado = 1.0 / 5.0;
    movement_state->frame_andando = 0; movement_state->acc_andando = 0; movement_state->tpf_andando = 1.0 / 10.0;
    movement_state->frame_correndo = 0; movement_state->acc_correndo = 0; movement_state->tpf_correndo = 1.0 / 10.0;
    movement_state->frame_pulando = 0; movement_state->acc_pulando = 0; movement_state->tpf_pulando = 1.0 / 12;
    movement_state->frame_agachado = 0; movement_state->acc_agachado = 0; movement_state->tpf_agachado = 1.0 / 8.0;
    movement_state->frame_especial = 0; movement_state->acc_especial = 0; movement_state->tpf_especial = 1.0 / 15.0;
    movement_state->frame_ataque1 = 0; movement_state->acc_ataque1 = 0; movement_state->tpf_ataque1 = 1.0 / 10.0;
    movement_state->frame_arremesso = 0; movement_state->acc_arremesso = 0; movement_state->tpf_arremesso = 1.0 / 8.0;

    // Hitbox
    movement_state->prot_hitbox_offset_x = 40.0 * movement_state->escala_personagens;
    movement_state->prot_hitbox_offset_y = 5.0 * movement_state->escala_personagens;
    movement_state->prot_hitbox_width = (dims->frame_largura_parado - 80) * movement_state->escala_personagens;
    movement_state->prot_hitbox_height = (dims->frame_altura - 10) * movement_state->escala_personagens;
    movement_state->prot_crouch_hitbox_height = (dims->frame_altura - 50) * movement_state->escala_personagens;
    movement_state->prot_attack_hitbox_offset_x = 60.0 * movement_state->escala_personagens;
    movement_state->prot_attack_hitbox_offset_y = 50.0 * movement_state->escala_personagens;
    movement_state->prot_attack_hitbox_width = 50 * movement_state->escala_personagens;
    movement_state->prot_attack_hitbox_height = 80.0 * movement_state->escala_personagens;
}

/**
 * @brief Inicializa o estado de todos os objetos do jogo (inimigos, obstáculos, dinheiro, projéteis).
 * @param garrafas Array de Garrafa.
 * @param granadas Array de Granada.
 * @param boss_projectiles Array de BossProjectile.
 * @param inimigos Array de Inimigo.
 * @param obstaculos Array de Obstaculo.
 * @param money_notes Array de MoneyNote.
 * @param traficante Ponteiro para NPC.
 * @param resources Ponteiro para GameResources.
 * @param dims Ponteiro para SpriteDimensions.
 * @param movement_state Ponteiro para ProtagonistMovementState.
 * @param out_proximo_ponto_spawn_obstaculo Ponteiro para float, armazenará o próximo ponto de spawn de obstáculo.
 * @param out_time_to_spawn_money Ponteiro para float, armazenará o tempo para spawn de dinheiro.
 * @param out_tempo_para_spawn_inimigo Ponteiro para float, armazenará o tempo para spawn de inimigos.
 */
void inicializar_objetos_jogo(Garrafa garrafas[], Granada granadas[], BossProjectile boss_projectiles[], Inimigo inimigos[], Obstaculo obstaculos[], MoneyNote money_notes[], NPC *traficante, const GameResources *resources, const SpriteDimensions *dims, const ProtagonistMovementState *movement_state, float *out_proximo_ponto_spawn_obstaculo, float *out_time_to_spawn_money, float *out_tempo_para_spawn_inimigo) {
    // Inicialização de Garrafas, Granadas e Projéteis do Chefe
    for (int i = 0; i < MAX_GARRAFAS; i++) {
        garrafas[i].ativa = false;
    }
    for (int i = 0; i < MAX_GRANADAS; i++) {
        granadas[i].ativa = false;
    }
    for (int i = 0; i < MAX_BOSS_PROJECTILES; i++) {
        boss_projectiles[i].ativa = false;
    }

    // Inicialização de Inimigos
    for (int i = 0; i < MAX_INIMIGOS; i++) {
        inimigos[i].ativa = false;
    }
    *out_tempo_para_spawn_inimigo = 2.0; // Tempo inicial para o primeiro spawn de inimigo

    // Inicialização de Obstáculos
    struct {
        ALLEGRO_BITMAP *sprite;
        float scale;
        bool crouch_pass;
    } obstacle_types[] = {
        {resources->sprite_sacos_lixo, 0.1f, false},
        {resources->sprite_placa_radar, 0.3f, true}
    };
    int num_obstacle_types = sizeof(obstacle_types) / sizeof(obstacle_types[0]);
    *out_proximo_ponto_spawn_obstaculo = LARGURA; // O primeiro obstáculo nasce na borda direita da tela
    float min_distancia_entre_obstaculos = 500.0f;
    float variacao_distancia_obstaculos = 600.0f;
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
        obstaculos[i].x = *out_proximo_ponto_spawn_obstaculo + (float)rand() / RAND_MAX * variacao_distancia_obstaculos;
        obstaculos[i].y = movement_state->personagem_y_base + (dims->frame_altura * movement_state->escala_personagens) - obstaculos[i].height;
        *out_proximo_ponto_spawn_obstaculo = obstaculos[i].x + obstaculos[i].width + min_distancia_entre_obstaculos;
    }

    // Inicialização de Notas de Dinheiro
    for (int i = 0; i < MAX_MONEY_NOTES; i++) {
        money_notes[i].ativa = false;
    }
    *out_time_to_spawn_money = MONEY_NOTE_SPAWN_INTERVAL;

    // Inicialização do NPC Traficante
    traficante->largura_sprite = al_get_bitmap_width(resources->sprite_traficante_parada) / NPC_TRAFICANTE_FRAME_COUNT;
    traficante->altura_sprite = al_get_bitmap_height(resources->sprite_traficante_parada);
    // Ajuste da posição Y do traficante para que seus pés fiquem no mesmo "chão" do personagem principal
    traficante->y = movement_state->personagem_y_base + (dims->frame_altura * movement_state->escala_personagens) - (traficante->altura_sprite * NPC_TRAFICANTE_SCALE);
    traficante->x = 4000; // Posição inicial no mundo
    traficante->ativa = true;
    traficante->frame_atual = 0;
    traficante->acc_animacao = 0;
}

/**
 * @brief Inicializa a entrada do chefe e o chefe em si.
 * @param entrance Ponteiro para BossEntrance.
 * @param boss Ponteiro para Boss.
 * @param resources Ponteiro para GameResources.
 * @param dims Ponteiro para SpriteDimensions.
 * @param movement_state Ponteiro para ProtagonistMovementState.
 */
void inicializar_chefe(BossEntrance *entrance, Boss *boss, const GameResources *resources, const SpriteDimensions *dims, const ProtagonistMovementState *movement_state) {
    entrance->sprite_bitmap = resources->sprite_gatopreto;
    entrance->x = BOSS_SPAWN_X;
    float entrance_original_w = al_get_bitmap_width(entrance->sprite_bitmap);
    float entrance_original_h = al_get_bitmap_height(entrance->sprite_bitmap);
    float entrance_scale = (float)ALTURA / entrance_original_h;
    entrance->width = entrance_original_w * entrance_scale;
    entrance->height = ALTURA;
    entrance->y = 0;

    boss->ativa = false;
    boss->estado = BOSS_INVISIVEL;
    boss->escala = 2.8; // Escala do sprite do chefe
    boss->health = MAX_BOSS_HEALTH;
    boss->attack_cooldown = BOSS_ATTACK_COOLDOWN;
    boss->invulnerability_timer = 0.0f;

    // Sprites do Chefe
    boss->sprite_parada = resources->sprite_prostituta_parada;
    boss->sprite_trans1 = resources->sprite_prostituta_trans1;
    boss->sprite_trans2 = resources->sprite_demon_trans2;
    boss->sprite_demonio_idle = resources->sprite_demon_parado;
    boss->sprite_demonio_ataque = resources->sprite_demon_ataque;
    boss->sprite_demonio_dano = resources->sprite_demon_dano;
    boss->sprite_demonio_morte = resources->sprite_demon_morte;

    // Dimensões dos frames do chefe
    boss->frame_largura_parada = al_get_bitmap_width(boss->sprite_parada) / BOSS_PROSTITUTA_PARADA_FRAME_COUNT;
    boss->frame_altura_parada = al_get_bitmap_height(boss->sprite_parada);
    boss->frame_largura_trans1 = al_get_bitmap_width(boss->sprite_trans1) / BOSS_PROSTITUTA_TRANS1_FRAME_COUNT;
    boss->frame_altura_trans1 = al_get_bitmap_height(boss->sprite_trans1);
    boss->frame_largura_trans2 = al_get_bitmap_width(boss->sprite_trans2) / BOSS_DEMON_TRANS2_FRAME_COUNT;
    boss->frame_altura_trans2 = al_get_bitmap_height(boss->sprite_trans2);
    boss->frame_largura_demonio_idle = al_get_bitmap_width(boss->sprite_demonio_idle) / BOSS_DEMON_IDLE_FRAME_COUNT;
    boss->frame_altura_demonio_idle = al_get_bitmap_height(boss->sprite_demonio_idle);
    boss->frame_largura_demonio_ataque = al_get_bitmap_width(boss->sprite_demonio_ataque) / BOSS_DEMON_ATTACK_FRAME_COUNT;
    boss->frame_altura_demonio_ataque = al_get_bitmap_height(boss->sprite_demonio_ataque);
    boss->frame_largura_demonio_dano = al_get_bitmap_width(boss->sprite_demonio_dano) / BOSS_DEMON_DANO_FRAME_COUNT;
    boss->frame_altura_demonio_dano = al_get_bitmap_height(boss->sprite_demonio_dano);
    boss->frame_largura_demonio_morte = al_get_bitmap_width(boss->sprite_demonio_morte) / BOSS_DEMON_MORTE_FRAME_COUNT;
    boss->frame_altura_demonio_morte = al_get_bitmap_height(boss->sprite_demonio_morte);
}


/**
 * @brief Processa as entradas do teclado para o protagonista.
 * @param ev Evento de teclado.
 * @param movement_state Ponteiro para ProtagonistMovementState.
 * @param combat_state Ponteiro para ProtagonistCombatState.
 * @param garrafas Array de Garrafa.
 * @param traficante Ponteiro para NPC.
 * @param player_money Ponteiro para o dinheiro do jogador.
 * @param boss Ponteiro para Boss.
 * @param dims Ponteiro para SpriteDimensions.
 * @param deslocamento_x Deslocamento do mundo.
 */
void handle_protagonist_input(ALLEGRO_EVENT ev, ProtagonistMovementState *movement_state, ProtagonistCombatState *combat_state, Garrafa garrafas[], NPC *traficante, int *player_money, Boss *boss, const SpriteDimensions *dims, float deslocamento_x) {
    if (combat_state->estado == PROT_NORMAL) {
        if (ev.keyboard.keycode == ALLEGRO_KEY_UP && !movement_state->pulando && !movement_state->agachando && !movement_state->atacando && !movement_state->arremessando) {
            movement_state->pulando = true; movement_state->vel_y = -10.0; movement_state->frame_pulando = 0; movement_state->acc_pulando = 0; movement_state->especial_ativo = false; movement_state->especial_finalizado = false;
            ALLEGRO_KEYBOARD_STATE estado_kb; al_get_keyboard_state(&estado_kb);
            if (al_key_down(&estado_kb, ALLEGRO_KEY_RIGHT)) movement_state->vel_x_pulo = movement_state->velocidade;
            else if (al_key_down(&estado_kb, ALLEGRO_KEY_LEFT)) movement_state->vel_x_pulo = -movement_state->velocidade;
            else movement_state->vel_x_pulo = 0;
            movement_state->current_ground_y = movement_state->personagem_y_base;
        }
        else if (ev.keyboard.keycode == ALLEGRO_KEY_DOWN && !movement_state->pulando && !movement_state->atacando && !movement_state->arremessando) {
            movement_state->agachando = true; movement_state->frame_agachado = 0; movement_state->acc_agachado = 0; movement_state->especial_ativo = false; movement_state->especial_finalizado = false;
            movement_state->crouch_animation_finished = false;
        }
        else if (ev.keyboard.keycode == ALLEGRO_KEY_Z && !movement_state->pulando && !movement_state->agachando && !movement_state->especial_ativo && !movement_state->atacando && !movement_state->arremessando) { movement_state->especial_ativo = true; movement_state->especial_finalizado = false; movement_state->frame_especial = 0; movement_state->acc_especial = 0; }
        else if (ev.keyboard.keycode == ALLEGRO_KEY_SPACE && movement_state->especial_ativo && movement_state->especial_finalizado && !movement_state->atacando && !movement_state->arremessando) { movement_state->atacando = true; movement_state->frame_ataque1 = 0; movement_state->acc_ataque1 = 0; }
        else if (ev.keyboard.keycode == ALLEGRO_KEY_Q && !movement_state->agachando && !movement_state->especial_ativo && !movement_state->atacando && !movement_state->arremessando) { movement_state->arremessando = true; movement_state->frame_arremesso = 0; movement_state->acc_arremesso = 0; }
        else if (ev.keyboard.keycode == ALLEGRO_KEY_B) {
            // Calcula a posição central do protagonista no mundo
            float prot_world_x_center = movement_state->x + (dims->frame_largura_parado * movement_state->escala_personagens) / 2.0 + deslocamento_x;

            // Lógica de interação com o Traficante
            // Calcula a posição central do NPC no mundo (usando a escala correta)
            float npc_world_x_center = traficante->x + (traficante->largura_sprite * NPC_TRAFICANTE_SCALE) / 2.0; 
            float distance_to_npc = fabs(prot_world_x_center - npc_world_x_center);
            if (traficante->ativa && distance_to_npc < NPC_INTERACTION_DISTANCE) {
                if (*player_money >= NPC_HEAL_COST) {
                    // Verifica se a vida já está no máximo (MAX_PROTAGONIST_HEALTH + NPC_HEAL_AMOUNT)
                    if (combat_state->health >= (MAX_PROTAGONIST_HEALTH + NPC_HEAL_AMOUNT)) { 
                        fprintf(stderr, "DEBUG: Vida já está no máximo (120 HP)! Não é possível comprar mais pó.\n");
                    } else {
                        *player_money -= NPC_HEAL_COST;
                        combat_state->health += NPC_HEAL_AMOUNT;
                        // Garante que a vida não ultrapasse o máximo permitido
                        if (combat_state->health > (MAX_PROTAGONIST_HEALTH + NPC_HEAL_AMOUNT)) {
                            combat_state->health = (MAX_PROTAGONIST_HEALTH + NPC_HEAL_AMOUNT);
                        }
                        fprintf(stderr, "DEBUG: Vida recuperada! HP: %d, Dinheiro: R$%d\n", combat_state->health, *player_money);
                    }
                } else {
                    fprintf(stderr, "DEBUG: Sem dinheiro suficiente para comprar! Dinheiro atual: R$%d, Custo: R$%d\n", *player_money, NPC_HEAL_COST);
                }
            }

            // Lógica de Interação com o Chefe
            if (boss->ativa && boss->estado == BOSS_PARADA) {
                float boss_world_x_center = boss->x + (boss->frame_largura_parada * boss->escala) / 2.0;
                float distance_to_boss = fabs(prot_world_x_center - boss_world_x_center);

                if (distance_to_boss < BOSS_INTERACTION_DISTANCE) {
                    fprintf(stderr, "DEBUG: Jogador interagiu com o chefe! Iniciando transformação.\n");
                    boss->estado = BOSS_TRANSFORMANDO;
                    boss->frame_atual = 0;
                    boss->acc_animacao = 0;
                }
            }
        }
    }
}

/**
 * @brief Atualiza o movimento e a animação do protagonista.
 * @param movement_state Ponteiro para ProtagonistMovementState.
 * @param combat_state Ponteiro para ProtagonistCombatState.
 * @param garrafas Array de Garrafa.
 * @param garrafa_largura_original Largura original da garrafa.
 * @param escala_garrafa Escala da garrafa.
 * @param dims Ponteiro para SpriteDimensions.
 * @param deslocamento_x Ponteiro para o deslocamento do mundo.
 * @param dt Delta tempo (geralmente 1.0 / 60.0).
 */
void atualizar_personagem_movimento_animacao(ProtagonistMovementState *movement_state, ProtagonistCombatState *combat_state, Garrafa garrafas[], int garrafa_largura_original, float escala_garrafa, const SpriteDimensions *dims, float *deslocamento_x, float dt) {
    ALLEGRO_KEYBOARD_STATE estado;
    al_get_keyboard_state(&estado);
    bool andando = false, correndo = false;

    if (combat_state->estado == PROT_NORMAL) {
        // Invulnerabilidade
        if (combat_state->invulnerable) {
            combat_state->invulnerability_timer -= dt;
            combat_state->blink_timer += dt;
            if (combat_state->blink_timer >= PROTAGONIST_BLINK_INTERVAL) {
                combat_state->blink_timer = 0.0;
                combat_state->visible = !combat_state->visible;
            }
            if (combat_state->invulnerability_timer <= 0) {
                combat_state->invulnerable = false;
                combat_state->visible = true;
            }
        }

        // Movimento horizontal
        if (!movement_state->atacando && (!movement_state->especial_ativo || movement_state->especial_finalizado) && !movement_state->arremessando) {
            movement_state->velocidade = (al_key_down(&estado, ALLEGRO_KEY_LSHIFT) || al_key_down(&estado, ALLEGRO_KEY_RSHIFT)) ? movement_state->velocidade_correr : movement_state->velocidade_andar;
            if (!movement_state->pulando) { // Movimento normal no chão
                if (al_key_down(&estado, ALLEGRO_KEY_RIGHT)) {
                    *deslocamento_x += movement_state->velocidade;
                    andando = true;
                    if (movement_state->velocidade == movement_state->velocidade_correr) correndo = true;
                }
                if (al_key_down(&estado, ALLEGRO_KEY_LEFT)) {
                    *deslocamento_x -= movement_state->velocidade;
                    andando = true;
                    if (movement_state->velocidade == movement_state->velocidade_correr) correndo = true;
                }
            } else {
                // Durante o pulo, a velocidade horizontal é determinada por vel_x_pulo
                // (já aplicada no deslocamento_x mais abaixo, após o cálculo do pulo)
            }
        }

        // Movimento vertical (pulo/gravidade)
        if (movement_state->pulando) {
            movement_state->y += movement_state->vel_y;
            movement_state->vel_y += movement_state->gravidade;
            *deslocamento_x += movement_state->vel_x_pulo; // Aplica o deslocamento horizontal do pulo
            if (movement_state->y >= movement_state->current_ground_y && movement_state->vel_y >= 0) {
                movement_state->y = movement_state->current_ground_y;
                movement_state->pulando = false;
                movement_state->vel_y = 0;
                movement_state->vel_x_pulo = 0; // Zera a velocidade horizontal de pulo
            }
            movement_state->acc_pulando += dt; if (movement_state->acc_pulando >= movement_state->tpf_pulando) { movement_state->acc_pulando -= movement_state->tpf_pulando; if (movement_state->frame_pulando < dims->frame_total_pulando - 1) movement_state->frame_pulando++; }
        } else if (movement_state->agachando) {
            movement_state->acc_agachado += dt;
            if (movement_state->acc_agachado >= movement_state->tpf_agachado) {
                movement_state->acc_agachado -= movement_state->tpf_agachado;
                if (movement_state->frame_agachado < dims->frame_total_agachado - 1) {
                    movement_state->frame_agachado++;
                } else {
                    movement_state->crouch_animation_finished = true;
                    movement_state->frame_agachado = dims->frame_total_agachado - 1; // Mantém no último frame
                }
            }
            // Sai do estado agachado se a tecla for solta E a animação de agachar terminou
            if (!al_key_down(&estado, ALLEGRO_KEY_DOWN) && movement_state->crouch_animation_finished) {
                movement_state->agachando = false;
                movement_state->crouch_animation_finished = false;
                movement_state->frame_agachado = 0;
                movement_state->acc_agachado = 0;
            }
            // Zera outros estados de animação quando agachado
            movement_state->frame_parado = 0; movement_state->acc_parado = 0;
            movement_state->frame_andando = 0; movement_state->acc_andando = 0;
            movement_state->frame_correndo = 0; movement_state->acc_correndo = 0;
            movement_state->especial_ativo = false; movement_state->especial_finalizado = false;
            movement_state->atacando = false; movement_state->frame_ataque1 = 0; movement_state->acc_ataque1 = 0;
            movement_state->arremessando = false; movement_state->frame_arremesso = 0; movement_state->acc_arremesso = 0;
        } else if (movement_state->atacando) {
            movement_state->acc_ataque1 += dt; if (movement_state->acc_ataque1 >= movement_state->tpf_ataque1) { movement_state->acc_ataque1 -= movement_state->tpf_ataque1; if (movement_state->frame_ataque1 < dims->frame_total_ataque1 - 1) { movement_state->frame_ataque1++; } else { movement_state->atacando = false; movement_state->frame_ataque1 = 0; } }
            // Zera outros estados de animação durante o ataque
            movement_state->frame_parado = 0; movement_state->acc_parado = 0;
            movement_state->frame_andando = 0; movement_state->acc_andando = 0;
            movement_state->frame_correndo = 0; movement_state->acc_correndo = 0;
            movement_state->frame_pulando = 0; movement_state->acc_pulando = 0;
            movement_state->frame_agachado = 0; movement_state->acc_agachado = 0;
            movement_state->especial_ativo = false; movement_state->especial_finalizado = false; // Zera especial
            movement_state->arremessando = false; movement_state->frame_arremesso = 0; movement_state->acc_arremesso = 0;
        } else if (movement_state->especial_ativo) {
            movement_state->acc_especial += dt; if (movement_state->acc_especial >= movement_state->tpf_especial) { movement_state->acc_especial -= movement_state->tpf_especial; if (movement_state->frame_especial < dims->frame_total_especial - 1) { movement_state->frame_especial++; } else { movement_state->especial_finalizado = true; } }
            // Zera outros estados de animação durante o especial
            movement_state->frame_parado = 0; movement_state->acc_parado = 0;
            movement_state->frame_andando = 0; movement_state->acc_andando = 0;
            movement_state->frame_correndo = 0; movement_state->acc_correndo = 0;
            movement_state->frame_pulando = 0; movement_state->acc_pulando = 0;
            movement_state->frame_agachado = 0; movement_state->acc_agachado = 0;
            movement_state->atacando = false; movement_state->frame_ataque1 = 0; movement_state->acc_ataque1 = 0;
            movement_state->arremessando = false; movement_state->frame_arremesso = 0; movement_state->acc_arremesso = 0;
        } else if (movement_state->arremessando) {
            movement_state->acc_arremesso += dt; if (movement_state->acc_arremesso >= movement_state->tpf_arremesso) { movement_state->acc_arremesso -= movement_state->tpf_arremesso; if (movement_state->frame_arremesso < dims->frame_total_arremesso - 1) { movement_state->frame_arremesso++; } else { movement_state->arremessando = false;
                // Lançar garrafa no final da animação de arremesso
                for (int i = 0; i < MAX_GARRAFAS; i++) {
                    if (!garrafas[i].ativa) {
                        garrafas[i].x = movement_state->x + (dims->frame_largura_arremesso * movement_state->escala_personagens) / 2.0 + *deslocamento_x;
                        garrafas[i].y = movement_state->y + (dims->frame_altura * movement_state->escala_personagens) / 2.0;
                        garrafas[i].vel_x = 15.0; // Velocidade da garrafa
                        garrafas[i].ativa = true;
                        garrafas[i].angulo = 0.0;
                        break;
                    }
                }
                movement_state->frame_arremesso = 0;
            } }
            // Zera outros estados de animação ao arremessar
            movement_state->frame_parado = 0; movement_state->acc_parado = 0;
            movement_state->frame_andando = 0; movement_state->acc_andando = 0;
            movement_state->frame_agachado = 0; movement_state->acc_agachado = 0;
            movement_state->especial_ativo = false; movement_state->especial_finalizado = false; // Zera especial
            movement_state->atacando = false; movement_state->frame_ataque1 = 0; movement_state->acc_ataque1 = 0;
        } else { // Estado parado ou andando no chão
            // Aplica gravidade se não estiver no chão base ou sobre um obstáculo
            if (movement_state->y < movement_state->current_ground_y) {
                movement_state->y += movement_state->gravidade * 8; // Acelera a queda para o chão
                if (movement_state->y > movement_state->current_ground_y) {
                    movement_state->y = movement_state->current_ground_y;
                    movement_state->vel_y = 0;
                }
            } else if (movement_state->y > movement_state->current_ground_y) {
                // Caso o personagem esteja abaixo do chão por algum motivo, ajusta para cima
                movement_state->y = movement_state->current_ground_y;
                movement_state->vel_y = 0;
            }

            // Animações de andar/correr/parado
            if (andando) {
                if (correndo) {
                    movement_state->acc_correndo += dt; if (movement_state->acc_correndo >= movement_state->tpf_correndo) { movement_state->acc_correndo -= movement_state->tpf_correndo; movement_state->frame_correndo = (movement_state->frame_correndo + 1) % dims->frame_total_correndo; }
                    movement_state->frame_andando = 0; movement_state->acc_andando = 0; movement_state->frame_parado = 0; movement_state->acc_parado = 0;
                }
                else {
                    movement_state->acc_andando += dt; if (movement_state->acc_andando >= movement_state->tpf_andando) { movement_state->acc_andando -= movement_state->tpf_andando; movement_state->frame_andando = (movement_state->frame_andando + 1) % dims->frame_total_andando; }
                    movement_state->frame_correndo = 0; movement_state->acc_correndo = 0; movement_state->frame_parado = 0; movement_state->acc_parado = 0;
                }
            } else { // Parado
                movement_state->acc_parado += dt; if (movement_state->acc_parado >= movement_state->tpf_parado) { movement_state->acc_parado -= movement_state->tpf_parado; movement_state->frame_parado = (movement_state->frame_parado + 1) % dims->frame_total_parado; }
                movement_state->frame_correndo = 0; movement_state->acc_correndo = 0; movement_state->frame_andando = 0; movement_state->acc_andando = 0;
            }
        }
    } else if (combat_state->estado == PROT_MORRENDO) {
        // Animação de morte do protagonista
        combat_state->personagem_acc_morte += dt;
        if (combat_state->personagem_acc_morte >= combat_state->tpf_personagem_morte) {
            combat_state->personagem_acc_morte -= combat_state->tpf_personagem_morte;
            if (combat_state->personagem_frame_morte < dims->frame_total_personagem_morte - 1) {
                combat_state->personagem_frame_morte++;
            } else {
                combat_state->animacao_morte_finalizada = true;
            }
        }
    }
}

/**
 * @brief Atualiza o estado do NPC Traficante e o volume de seu áudio.
 * @param traficante Ponteiro para NPC.
 * @param instancia_som_verdebalaraio Instância de áudio.
 * @param movement_state Ponteiro para ProtagonistMovementState.
 * @param dims Ponteiro para SpriteDimensions.
 * @param deslocamento_x Deslocamento do mundo.
 * @param dt Delta tempo.
 */
void atualizar_npc(NPC *traficante, ALLEGRO_SAMPLE_INSTANCE *instancia_som_verdebalaraio, const ProtagonistMovementState *movement_state, const SpriteDimensions *dims, float deslocamento_x, float dt) {
    if (traficante->ativa) {
        // Atualiza animação do NPC
        traficante->acc_animacao += dt;
        if (traficante->acc_animacao >= NPC_TRAFICANTE_TPF) {
            traficante->acc_animacao -= NPC_TRAFICANTE_TPF;
            traficante->frame_atual = (traficante->frame_atual + 1) % NPC_TRAFICANTE_FRAME_COUNT;
        }

        // Atualiza Volume do Áudio do NPC (Fade baseado na distância)
        float prot_world_x_center = movement_state->x + (dims->frame_largura_parado * movement_state->escala_personagens) / 2.0 + deslocamento_x;
        // Usa NPC_TRAFICANTE_SCALE para calcular o centro do NPC para o áudio
        float npc_world_x_center = traficante->x + (traficante->largura_sprite * NPC_TRAFICANTE_SCALE) / 2.0;
        float distance = fabs(prot_world_x_center - npc_world_x_center);
        float volume;

        if (distance <= AUDIO_MIN_DISTANCE) {
            volume = 1.0; // Volume máximo quando muito próximo
        } else if (distance >= AUDIO_MAX_DISTANCE) {
            volume = 0.0; // Mudo quando muito longe
        } else {
            // Interpolação linear do volume entre AUDIO_MIN_DISTANCE e AUDIO_MAX_DISTANCE
            volume = 1.0 - ((distance - AUDIO_MIN_DISTANCE) / (AUDIO_MAX_DISTANCE - AUDIO_MIN_DISTANCE));
            if (volume < 0.0) volume = 0.0; // Garante que o volume não seja negativo
            if (volume > 1.0) volume = 1.0; // Garante que o volume não ultrapasse 1.0
        }
        al_set_sample_instance_gain(instancia_som_verdebalaraio, volume);
    }
}

/**
 * @brief Atualiza a posição e estado de garrafas, granadas e projéteis do chefe.
 * @param garrafas Array de Garrafa.
 * @param granadas Array de Granada.
 * @param boss_projectiles Array de BossProjectile.
 * @param deslocamento_x Deslocamento do mundo.
 * @param dt Delta tempo.
 * @param garrafa_largura_original Largura original da garrafa.
 * @param escala_garrafa Escala da garrafa.
 * @param dims Ponteiro para SpriteDimensions (para frames de explosão).
 * @param movement_state Ponteiro para ProtagonistMovementState (para y_base da granada).
 */
void atualizar_projeteis(Garrafa garrafas[], Granada granadas[], BossProjectile boss_projectiles[], float deslocamento_x, float dt, int garrafa_largura_original, float escala_garrafa, const SpriteDimensions *dims, const ProtagonistMovementState *movement_state) {
    float garrafa_velocidade_angular = 0.2f;

    // Atualização de Garrafas
    for (int i = 0; i < MAX_GARRAFAS; i++) {
        if (garrafas[i].ativa) {
            garrafas[i].x += garrafas[i].vel_x; // Movimento horizontal
            garrafas[i].angulo += garrafa_velocidade_angular; // Rotação da garrafa
            if (garrafas[i].angulo > ALLEGRO_PI * 2) {
                garrafas[i].angulo -= ALLEGRO_PI * 2;
            }
            // Desativa se sair muito da tela à direita
            if (garrafas[i].x - deslocamento_x > LARGURA + (garrafa_largura_original * escala_garrafa)) {
                garrafas[i].ativa = false;
            }
        }
    }

    // Atualização dos Projéteis do Chefe
    for (int i = 0; i < MAX_BOSS_PROJECTILES; i++) {
        if (boss_projectiles[i].ativa) {
            boss_projectiles[i].x += boss_projectiles[i].vel_x; // Movimento horizontal
            boss_projectiles[i].angulo += 0.2f; // Gira o projétil

            // Desativa se sair da tela
            if (boss_projectiles[i].x - deslocamento_x < -100 || boss_projectiles[i].x - deslocamento_x > LARGURA + 100) {
                boss_projectiles[i].ativa = false;
            }
        }
    }

    // Atualização das Granadas
    for (int i = 0; i < MAX_GRANADAS; i++) {
        if (granadas[i].ativa) {
            if (!granadas[i].explodindo) {
                granadas[i].x += granadas[i].vel_x; // Movimento horizontal
                granadas[i].y += granadas[i].vel_y; // Movimento vertical
                granadas[i].vel_y += movement_state->gravidade * 0.5; // Aplica gravidade (metade da normal para ser mais leve)

                // Checa se atingiu o chão (personagem_y_base + altura_personagem - metade_altura_explosao)
                if (granadas[i].y >= movement_state->personagem_y_base + (dims->frame_altura * movement_state->escala_personagens) - (dims->frame_altura_explosao * movement_state->escala_personagens / 2.0)) {
                    granadas[i].y = movement_state->personagem_y_base + (dims->frame_altura * movement_state->escala_personagens) - (dims->frame_altura_explosao * movement_state->escala_personagens / 2.0); // Crava no chão
                    granadas[i].explodindo = true; // Inicia explosão
                    granadas[i].frame_explosao = 0;
                    granadas[i].acc_animacao = 0;
                    granadas[i].dano_aplicado = false; // Permite aplicar dano ao explodir
                }
            } else { // Está explodindo
                granadas[i].acc_animacao += dt;
                if (granadas[i].acc_animacao >= (1.0 / 15.0)) { // Usa tpf_explosao diretamente
                    granadas[i].acc_animacao -= (1.0 / 15.0);
                    if (granadas[i].frame_explosao < dims->frame_total_explosao - 1) {
                        granadas[i].frame_explosao++;
                    } else {
                        granadas[i].ativa = false; // Animação da explosão terminou, desativa granada
                    }
                }
            }
        }
    }
}

/**
 * @brief Gerencia o spawn e atualização dos inimigos.
 * @param inimigos Array de Inimigo.
 * @param deslocamento_x Deslocamento do mundo.
 * @param dt Delta tempo.
 * @param tempo_para_spawn_inimigo Ponteiro para o tempo para o próximo spawn.
 * @param min_intervalo_spawn_inimigo Intervalo mínimo de spawn.
 * @param max_intervalo_spawn_inimigo Intervalo máximo de spawn.
 * @param movement_state Ponteiro para ProtagonistMovementState.
 * @param dims Ponteiro para SpriteDimensions.
 * @param granadas Array de Granada (para spawnar granadas do policial).
 * @param boss Ponteiro para Boss.
 */
void gerenciar_inimigos(Inimigo inimigos[], float deslocamento_x, float dt, float *tempo_para_spawn_inimigo, float min_intervalo_spawn_inimigo, float max_intervalo_spawn_inimigo, const ProtagonistMovementState *movement_state, const SpriteDimensions *dims, Granada granadas[], const Boss *boss) {
    float inimigo_velocidade_parado_base = -0.5f;
    float inimigo_velocidade_andando_base = -2.5f;
    float inimigo_velocidade_ataque_base = 0.0f; // Inimigos ficam parados ao atacar
    float noia_distancia_deteccao = 500.0f;
    float noia_distancia_ataque = 100.0f;
    float policial_distancia_deteccao = 800.0f; // Policial detecta de mais longe
    float policial_distancia_ataque = 700.0f;   // Policial ataca de mais longe (arremesso)

    *tempo_para_spawn_inimigo -= dt;
    // Não spawna inimigos se o chefe estiver ativo OU já tiver sido permanentemente derrotado
    if (*tempo_para_spawn_inimigo <= 0 && !boss->ativa && !boss_defeated_permanently) {
        for (int i = 0; i < MAX_INIMIGOS; i++) {
            if (!inimigos[i].ativa) {
                // Decide o tipo de inimigo a ser spawnado (50% Noia, 50% Policial)
                if (rand() % 2 == 0) {
                    inimigos[i].type = NOIA;
                } else {
                    inimigos[i].type = POLICIAL;
                }

                // Inicialização comum para ambos os tipos
                inimigos[i].x = deslocamento_x + LARGURA + (float)(rand() % 200 + 50); // Spawna fora da tela à direita
                inimigos[i].y = movement_state->personagem_y_base; // Nível do chão
                inimigos[i].ativa = true;
                inimigos[i].frame_atual = 0;
                inimigos[i].acc_animacao = 0;
                inimigos[i].estado = INIMIGO_PARADO;
                inimigos[i].animacao_morte_finalizada = false;
                inimigos[i].inimigo_pode_dar_dano = true; // Permite que o inimigo cause dano novamente
                inimigos[i].attack_cooldown = 0; // Cooldown inicial zero para atacar imediatamente se detectar

                // Inicialização específica por tipo (tamanhos de hitbox)
                if (inimigos[i].type == NOIA) {
                    inimigos[i].vel_x = inimigo_velocidade_parado_base;
                    inimigos[i].hitbox_offset_x = 57.0f * movement_state->escala_personagens;
                    inimigos[i].hitbox_offset_y = 20 * movement_state->escala_personagens;
                    inimigos[i].hitbox_width = (dims->frame_largura_inimigo_parado - 100) * movement_state->escala_personagens;
                    inimigos[i].hitbox_height = (dims->frame_altura - 20) * movement_state->escala_personagens;
                } else { // POLICIAL
                    inimigos[i].vel_x = inimigo_velocidade_parado_base;
                    inimigos[i].hitbox_offset_x = 57.0f * movement_state->escala_personagens;
                    inimigos[i].hitbox_offset_y = 20 * movement_state->escala_personagens;
                    inimigos[i].hitbox_width = (dims->frame_largura_policial_parado - 100) * movement_state->escala_personagens;
                    inimigos[i].hitbox_height = (dims->frame_altura - 20) * movement_state->escala_personagens;
                }
                // Define o próximo tempo para spawn
                *tempo_para_spawn_inimigo = min_intervalo_spawn_inimigo + ((float)rand() / RAND_MAX) * (max_intervalo_spawn_inimigo - min_intervalo_spawn_inimigo);
                fprintf(stderr, "DEBUG: Inimigo %d (Tipo: %d) SPAWNADO.\n", i, inimigos[i].type);
                break; // Spawna apenas um inimigo por vez
            }
        }
    }

    // Atualiza o estado de cada inimigo ativo
    for (int i = 0; i < MAX_INIMIGOS; i++) {
        if (inimigos[i].ativa) {
            // Atualiza cooldown de ataque do inimigo
            if (inimigos[i].attack_cooldown > 0) { inimigos[i].attack_cooldown -= dt; }

            inimigos[i].x += inimigos[i].vel_x; // Aplica a velocidade horizontal

            // Calcula distância do inimigo ao protagonista
            float prot_world_x_center_hb = (movement_state->x + movement_state->prot_hitbox_offset_x + movement_state->prot_hitbox_width / 2.0) + deslocamento_x;
            float inimigo_world_x_center_hb = inimigos[i].x + inimigos[i].hitbox_offset_x + inimigos[i].hitbox_width / 2.0;
            float dist_to_protagonist_center_x = fabs(inimigo_world_x_center_hb - prot_world_x_center_hb);

            // Define as distâncias de detecção e ataque baseadas no tipo do inimigo
            float distancia_ataque_atual = (inimigos[i].type == NOIA) ? noia_distancia_ataque : policial_distancia_ataque;
            float distancia_deteccao_atual = (inimigos[i].type == NOIA) ? noia_distancia_deteccao : policial_distancia_deteccao;

            // LÓGICA DE IA E TRANSIÇÕES DE ESTADO (só se não estiver morrendo)
            if (inimigos[i].estado != INIMIGO_MORRENDO) {
                // Se o jogador estiver na distância de ataque E o cooldown zerou, inicia ataque
                if (dist_to_protagonist_center_x <= distancia_ataque_atual && inimigos[i].attack_cooldown <= 0) {
                    if (inimigos[i].estado != INIMIGO_ATACANDO) {
                        inimigos[i].estado = INIMIGO_ATACANDO;
                        inimigos[i].vel_x = inimigo_velocidade_ataque_base; // Parado ao atacar
                        inimigos[i].frame_atual = 0;
                        inimigos[i].acc_animacao = 0;
                        inimigos[i].inimigo_pode_dar_dano = true; // Permite dano para o novo ataque
                    }
                }
                // Se o jogador estiver na distância de detecção, mas não na de ataque, e não estiver atacando, anda
                else if (dist_to_protagonist_center_x < distancia_deteccao_atual && inimigos[i].estado != INIMIGO_ATACANDO) {
                    if (inimigos[i].estado != INIMIGO_ANDANDO) {
                        inimigos[i].estado = INIMIGO_ANDANDO;
                        inimigos[i].vel_x = inimigo_velocidade_andando_base; // Anda em direção ao jogador
                    }
                }
                // Se o jogador estiver fora da distância de detecção, e não estiver atacando, fica parado
                else if (inimigos[i].estado != INIMIGO_ATACANDO) {
                    if (inimigos[i].estado != INIMIGO_PARADO) {
                        inimigos[i].estado = INIMIGO_PARADO;
                        inimigos[i].vel_x = inimigo_velocidade_parado_base; // Move lentamente para a esquerda
                    }
                }
            }

            // ATUALIZAÇÃO DE ANIMAÇÃO DO INIMIGO
            float tpf_current_inimigo = 0;
            int frame_total_current_inimigo = 0;

            if (inimigos[i].type == NOIA) {
                switch(inimigos[i].estado) {
                    case INIMIGO_PARADO: tpf_current_inimigo = 1.0 / 8.0; frame_total_current_inimigo = dims->frame_total_inimigo_parado; break;
                    case INIMIGO_ANDANDO: tpf_current_inimigo = 1.0 / 10.0; frame_total_current_inimigo = dims->frame_total_inimigo_andando; break;
                    case INIMIGO_ATACANDO: tpf_current_inimigo = 1.0 / 8.0; frame_total_current_inimigo = dims->frame_total_inimigo_ataque; break;
                    case INIMIGO_MORRENDO: tpf_current_inimigo = 1.0 / 8.0; frame_total_current_inimigo = dims->frame_total_inimigo_morte; break;
                }
            } else { // POLICIAL
                 switch(inimigos[i].estado) {
                    case INIMIGO_PARADO:
                    case INIMIGO_ANDANDO: // Policial usa a mesma animação parada para andar
                        tpf_current_inimigo = 1.0 / 8.0;
                        frame_total_current_inimigo = dims->frame_total_policial_parado;
                        break;
                    case INIMIGO_ATACANDO:
                        tpf_current_inimigo = 1.0 / 12.0;
                        frame_total_current_inimigo = dims->frame_total_policial_arremesso;
                        break;
                    case INIMIGO_MORRENDO:
                        tpf_current_inimigo = 1.0 / 8.0; // Usa o tempo por frame do policial
                        frame_total_current_inimigo = dims->frame_total_policial_morte; // Usa o total de frames do policial
                        break;
                }
            }
            inimigos[i].acc_animacao += dt;
            if (inimigos[i].acc_animacao >= tpf_current_inimigo) {
                inimigos[i].acc_animacao -= tpf_current_inimigo;
                if (inimigos[i].frame_atual < frame_total_current_inimigo - 1) {
                    inimigos[i].frame_atual++;
                    // NOVO: Lançar granada em um frame específico da animação de ataque do policial
                    if (inimigos[i].type == POLICIAL && inimigos[i].estado == INIMIGO_ATACANDO && inimigos[i].frame_atual == 5) {
                        for(int g = 0; g < MAX_GRANADAS; g++) {
                            if(!granadas[g].ativa) {
                                granadas[g].ativa = true;
                                granadas[g].explodindo = false; // Não explodindo ainda
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
                        inimigos[i].attack_cooldown = 2.0 + (rand() % 2); // Cooldown de 2 a 3 segundos antes de atacar novamente
                    } else if (inimigos[i].estado == INIMIGO_MORRENDO) {
                        inimigos[i].animacao_morte_finalizada = true; // Marca que a animação de morte terminou
                    } else {
                        inimigos[i].frame_atual = 0; // Reinicia animações de loop (parado, andando)
                    }
                }
            }
        }
    }
}

/**
 * @brief Gerencia o spawn e o ciclo de vida das notas de dinheiro.
 * @param money_notes Array de MoneyNote.
 * @param resources Ponteiro para GameResources.
 * @param deslocamento_x Deslocamento do mundo.
 * @param dt Delta tempo.
 * @param time_to_spawn_money Ponteiro para o tempo para o próximo spawn de dinheiro.
 * @param boss Ponteiro para Boss.
 * @param dims Ponteiro para SpriteDimensions.
 * @param movement_state Ponteiro para ProtagonistMovementState.
 */
void gerenciar_dinheiro(MoneyNote money_notes[], const GameResources *resources, float deslocamento_x, float dt, float *time_to_spawn_money, const Boss *boss, const SpriteDimensions *dims, const ProtagonistMovementState *movement_state) {
    ALLEGRO_BITMAP *money_sprites[] = {resources->sprite_2reais, resources->sprite_5reais, resources->sprite_10reais};
    int money_values[] = {2, 5, 10};
    int num_money_types = sizeof(money_sprites) / sizeof(money_sprites[0]);

    *time_to_spawn_money -= dt;
    // Não spawna dinheiro se o chefe estiver ativo OU já tiver sido permanentemente derrotado
    if (*time_to_spawn_money <= 0 && !boss->ativa && !boss_defeated_permanently) {
        for (int i = 0; i < MAX_MONEY_NOTES; i++) {
            if (!money_notes[i].ativa) {
                int type_index = rand() % num_money_types;
                money_notes[i].sprite_bitmap = money_sprites[type_index];
                money_notes[i].value = money_values[type_index];
                // Calcula a hitbox da nota de dinheiro baseada na escala
                money_notes[i].hitbox_width = al_get_bitmap_width(money_notes[i].sprite_bitmap) * MONEY_NOTE_SCALE;
                money_notes[i].hitbox_height = al_get_bitmap_height(money_notes[i].sprite_bitmap) * MONEY_NOTE_SCALE;

                money_notes[i].x = deslocamento_x + LARGURA + (float)(rand() % 400 + 100); // Spawna fora da tela à direita
                // Posiciona a nota no chão (baseado no y_base do personagem e altura da nota)
                money_notes[i].y = movement_state->personagem_y_base + (dims->frame_altura * movement_state->escala_personagens) - money_notes[i].hitbox_height;
                money_notes[i].ativa = true;
                money_notes[i].spawn_time = al_get_time(); // Marca o tempo de criação
                fprintf(stderr, "DEBUG: Nota de R$%d SPAWNADA em (%.2f, %.2f).\n", money_notes[i].value, money_notes[i].x, money_notes[i].y);
                break; // Spawna apenas uma nota por vez
            }
        }
        *time_to_spawn_money = MONEY_NOTE_SPAWN_INTERVAL; // Reseta o timer para o próximo spawn
    }

    // Verifica o tempo de vida das notas de dinheiro e desativa se expirou ou saiu da tela
    for (int i = 0; i < MAX_MONEY_NOTES; i++) {
        if (money_notes[i].ativa) {
            if (money_notes[i].x - deslocamento_x < -money_notes[i].hitbox_width || // Saiu da tela à esquerda
                (al_get_time() - money_notes[i].spawn_time > MONEY_NOTE_LIFETIME)) { // Tempo de vida expirou
                money_notes[i].ativa = false;
                fprintf(stderr, "DEBUG: Nota de dinheiro desativada (fora da tela ou tempo de vida expirou).\n");
            }
        }
    }
}

/**
 * @brief Recicla obstáculos, inimigos, dinheiro e o NPC quando saem da tela.
 * @param deslocamento_x Deslocamento do mundo.
 * @param obstaculos Array de Obstaculo.
 * @param inimigos Array de Inimigo.
 * @param money_notes Array de MoneyNote.
 * @param traficante Ponteiro para NPC.
 * @param boss Ponteiro para Boss.
 * @param dims Ponteiro para SpriteDimensions.
 * @param movement_state Ponteiro para ProtagonistMovementState.
 * @param resources Ponteiro para GameResources.
 */
void reciclar_objetos_mundo(float deslocamento_x, Obstaculo obstaculos[], Inimigo inimigos[], MoneyNote money_notes[], NPC *traficante, const Boss *boss, const SpriteDimensions *dims, const ProtagonistMovementState *movement_state, const GameResources *resources) {
    float max_x_existente = deslocamento_x + LARGURA; // O ponto mais à direita na tela + um buffer

    // Encontra o objeto ativo mais à direita para posicionar novos objetos de forma que não haja lacunas
    for(int i = 0; i < MAX_OBSTACULOS; i++) {
        if (obstaculos[i].ativa && obstaculos[i].x > max_x_existente) {
            max_x_existente = obstaculos[i].x;
        }
    }
    if (traficante->ativa && traficante->x > max_x_existente) {
        max_x_existente = traficante->x;
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

    // Tipos de obstáculos para reciclagem
    struct {
        ALLEGRO_BITMAP *sprite;
        float scale;
        bool crouch_pass;
    } obstacle_types[] = {
        {resources->sprite_sacos_lixo, 0.1f, false},
        {resources->sprite_placa_radar, 0.3f, true}
    };
    int num_obstacle_types = sizeof(obstacle_types) / sizeof(obstacle_types[0]);
    float min_distancia_entre_obstaculos = 500.0f;
    float variacao_distancia_obstaculos = 600.0f;

    // Recicla obstáculos
    for (int i = 0; i < MAX_OBSTACULOS; i++) {
        // Se o obstáculo estiver ativo E saiu da tela à esquerda E o chefe NÃO estiver ativo (para de spawnar quando o chefe aparece)
        if (obstaculos[i].ativa && ((obstaculos[i].x + obstaculos[i].width) < deslocamento_x - LARGURA / 2) && !boss->ativa) {
            fprintf(stderr, "DEBUG: Reciclando obstaculo %d.\n", i);
            int type_index = rand() % num_obstacle_types;
            obstaculos[i].sprite_bitmap = obstacle_types[type_index].sprite;
            obstaculos[i].only_crouch_pass = obstacle_types[type_index].crouch_pass;
            float current_scale = obstacle_types[type_index].scale;
            obstaculos[i].width = al_get_bitmap_width(obstaculos[i].sprite_bitmap) * current_scale;
            obstaculos[i].height = al_get_bitmap_height(obstaculos[i].sprite_bitmap) * current_scale;
            // Posiciona o novo obstáculo após o objeto mais à direita + uma distância aleatória
            obstaculos[i].x = max_x_existente + min_distancia_entre_obstaculos + (float)rand() / RAND_MAX * variacao_distancia_obstaculos;
            // Posiciona Y para que a base do obstáculo esteja no chão
            obstaculos[i].y = movement_state->personagem_y_base + (dims->frame_altura * movement_state->escala_personagens) - obstaculos[i].height;
            obstaculos[i].ativa = true;
        }
    }

    // Recicla NPC Traficante
    if (traficante->ativa && (traficante->x + traficante->largura_sprite * NPC_TRAFICANTE_SCALE) < deslocamento_x - LARGURA / 2) {
        fprintf(stderr, "DEBUG: Reciclando traficante.\n");
        float distancia_base = 5000.0f;
        float variacao_distancia = 1500.0f;
        // Reposiciona o traficante muito à frente do jogador
        traficante->x = deslocamento_x + distancia_base + (float)rand() / RAND_MAX * variacao_distancia;
        fprintf(stderr, "DEBUG: Traficante reposicionado para x = %.2f\n", traficante->x);
    }

    // Recicla inimigos
    for (int i = 0; i < MAX_INIMIGOS; i++) {
        // Se o inimigo estiver ativo E saiu da tela à esquerda E a animação de morte não terminou
        if (inimigos[i].ativa && (inimigos[i].x + dims->inimigo_largura_sprite_max * movement_state->escala_personagens) < deslocamento_x - LARGURA / 2) {
            fprintf(stderr, "DEBUG: Inimigo %d DESATIVADO (saiu da tela à esquerda).\n", i);
            inimigos[i].ativa = false; // Desativa o inimigo
            // Reseta o estado para que possa ser reutilizado
            inimigos[i].estado = INIMIGO_PARADO;
            inimigos[i].vel_x = -0.5f; // Velocidade padrão para "parado"
            inimigos[i].animacao_morte_finalizada = false;
            inimigos[i].frame_atual = 0;
            inimigos[i].acc_animacao = 0;
            inimigos[i].inimigo_pode_dar_dano = true;
        }
    }

    // Recicla notas de dinheiro
    for (int i = 0; i < MAX_MONEY_NOTES; i++) {
        // Se a nota de dinheiro estiver ativa E saiu da tela à esquerda
        if (money_notes[i].ativa && (money_notes[i].x + money_notes[i].hitbox_width) < deslocamento_x - LARGURA / 2) {
            money_notes[i].ativa = false;
            fprintf(stderr, "DEBUG: Nota de dinheiro desativada (fora da tela à esquerda).\n");
        }
    }
}


/**
 * @brief Gerencia a ativação e o comportamento de combate do chefe.
 * @param boss Ponteiro para Boss.
 * @param entrance Ponteiro para BossEntrance.
 * @param inimigos Array de Inimigo (para desativar).
 * @param obstaculos Array de Obstaculo (para desativar).
 * @param money_notes Array de MoneyNote (para desativar).
 * @param boss_projectiles Array de BossProjectile (para lançar).
 * @param deslocamento_x Deslocamento do mundo.
 * @param dt Delta tempo.
 * @param movement_state Ponteiro para ProtagonistMovementState.
 * @param dims Ponteiro para SpriteDimensions.
 */
void gerenciar_chefe_e_combate(Boss *boss, const BossEntrance *entrance, Inimigo inimigos[], Obstaculo obstaculos[], MoneyNote money_notes[], BossProjectile boss_projectiles[], float deslocamento_x, float dt, const ProtagonistMovementState *movement_state, const SpriteDimensions *dims) {
    // Ativação do chefe: quando o jogador alcança a posição BOSS_SPAWN_X e o chefe ainda não foi permanentemente derrotado
    if (!boss->ativa && deslocamento_x >= BOSS_SPAWN_X - LARGURA && !boss_defeated_permanently) {
        boss->ativa = true;
        fprintf(stderr, "DEBUG: Chefe ativado. Removendo inimigos e obstáculos.\n");
        // Desativa todos os inimigos, obstáculos e notas de dinheiro existentes
        for (int i = 0; i < MAX_INIMIGOS; i++) { inimigos[i].ativa = false; }
        for (int i = 0; i < MAX_OBSTACULOS; i++) { obstaculos[i].ativa = false; }
        for (int i = 0; i < MAX_MONEY_NOTES; i++) { money_notes[i].ativa = false; }
        // Define o estado inicial do chefe como "parada" (forma de prostituta)
        boss->estado = BOSS_PARADA;
        boss->frame_atual = 0;
        boss->acc_animacao = 0;
        // Posiciona o chefe um pouco à frente da entrada (gato preto)
        boss->x = entrance->x + 150;
        // Ajusta a posição Y do chefe para que seus pés estejam no chão, considerando a escala
        boss->y = movement_state->personagem_y_base + (dims->frame_altura * movement_state->escala_personagens) - (boss->frame_altura_parada * boss->escala);
        fprintf(stderr, "DEBUG: Chefe ativado na posição X: %.2f\n", boss->x);
    }

    // Se o chefe estiver ativo, atualiza sua lógica
    if (boss->ativa) {
        // Atualiza os timers de cooldown e invulnerabilidade do chefe
        if(boss->attack_cooldown > 0) { boss->attack_cooldown -= dt; }
        if(boss->invulnerability_timer > 0) { boss->invulnerability_timer -= dt; }

        // Tempos por frame para as animações do chefe
        float tpf_boss_parada = 1.0 / 5.0;
        float tpf_boss_trans1 = 1.0 / 12.0;
        float tpf_boss_trans2 = 1.0 / 12.0;
        float tpf_boss_demonio_idle = 1.0 / 8.0;
        float tpf_boss_ataque = 1.0 / 10.0;
        float tpf_boss_dano = 1.0 / 12.0;
        float tpf_boss_morte = 1.0 / 8.0;

        // Máquina de estados do chefe
        switch(boss->estado) {
            case BOSS_PARADA:
                // Animação da prostituta parada
                boss->acc_animacao += dt;
                if (boss->acc_animacao >= tpf_boss_parada) {
                    boss->acc_animacao -= tpf_boss_parada;
                    boss->frame_atual = (boss->frame_atual + 1) % BOSS_PROSTITUTA_PARADA_FRAME_COUNT;
                }
                // A transição para BOSS_TRANSFORMANDO é feita pela interação do jogador (tecla 'B')
                break;

            case BOSS_TRANSFORMANDO:
                // Animação da primeira parte da transformação
                boss->acc_animacao += dt;
                if (boss->acc_animacao >= tpf_boss_trans1) {
                    boss->acc_animacao -= tpf_boss_trans1;
                    if (boss->frame_atual < BOSS_PROSTITUTA_TRANS1_FRAME_COUNT - 1) {
                        boss->frame_atual++;
                    } else {
                        // A animação da primeira parte terminou, transiciona para a segunda
                        fprintf(stderr, "DEBUG: Fim da transformação parte 1. Iniciando parte 2.\n");
                        boss->estado = BOSS_TRANSFORMANDO_2;
                        boss->frame_atual = 0;
                        boss->acc_animacao = 0;
                    }
                }
                break;

            case BOSS_TRANSFORMANDO_2:
                // Animação da segunda parte da transformação (demônio)
                boss->acc_animacao += dt;
                if (boss->acc_animacao >= tpf_boss_trans2) {
                    boss->acc_animacao -= tpf_boss_trans2;
                    if (boss->frame_atual < BOSS_DEMON_TRANS2_FRAME_COUNT - 1) {
                        boss->frame_atual++;
                    } else {
                        // A animação da segunda transformação terminou, transiciona para o estado IDLE do demônio
                        fprintf(stderr, "DEBUG: Fim da transformação parte 2. Assumindo forma de demônio (IDLE).\n");
                        boss->estado = BOSS_DEMONIO_IDLE;
                        boss->frame_atual = 0;
                        boss->acc_animacao = 0;
                        // Ajusta a posição Y para a nova altura do sprite do demônio, se necessário
                        boss->y = movement_state->personagem_y_base + (dims->frame_altura * movement_state->escala_personagens) - (boss->frame_altura_demonio_idle * boss->escala);
                    }
                }
                break;

            case BOSS_DEMONIO_IDLE:
                // Animação de idle para o chefe demônio
                boss->acc_animacao += dt;
                if (boss->acc_animacao >= tpf_boss_demonio_idle) {
                    boss->acc_animacao -= tpf_boss_demonio_idle;
                    boss->frame_atual = (boss->frame_atual + 1) % BOSS_DEMON_IDLE_FRAME_COUNT;
                }
                // Lógica para iniciar ataque: se o cooldown zerou
                if(boss->attack_cooldown <= 0) {
                    boss->estado = BOSS_DEMONIO_ATACANDO;
                    boss->frame_atual = 0;
                    boss->acc_animacao = 0;
                    fprintf(stderr, "DEBUG: Chefe iniciando ataque!\n");
                }
                break;

            case BOSS_DEMONIO_ATACANDO:
                // Animação de ataque do chefe demônio
                boss->acc_animacao += dt;
                if (boss->acc_animacao >= tpf_boss_ataque) {
                    boss->acc_animacao -= tpf_boss_ataque;
                    if (boss->frame_atual < BOSS_DEMON_ATTACK_FRAME_COUNT - 1) {
                        boss->frame_atual++;
                        // Lança o projétil em um frame específico da animação de ataque (frame 4)
                        if (boss->frame_atual == 4) {
                            for(int i = 0; i < MAX_BOSS_PROJECTILES; i++) {
                                if(!boss_projectiles[i].ativa) {
                                    boss_projectiles[i].ativa = true;
                                    boss_projectiles[i].x = boss->x + 50; // Posição no mundo, ajustada para sair da frente do chefe
                                    boss_projectiles[i].y = boss->y + (boss->frame_altura_demonio_ataque * boss->escala / 2);
                                    boss_projectiles[i].vel_x = -12.0f; // Velocidade para a esquerda
                                    boss_projectiles[i].angulo = 0;
                                    fprintf(stderr, "DEBUG: Chefe lançou projétil %d.\n", i);
                                    break;
                                }
                            }
                        }
                    } else {
                        // Fim da animação de ataque, volta para IDLE e reinicia o cooldown
                        boss->estado = BOSS_DEMONIO_IDLE;
                        boss->frame_atual = 0;
                        boss->acc_animacao = 0;
                        boss->attack_cooldown = BOSS_ATTACK_COOLDOWN;
                    }
                }
                break;

            case BOSS_DEMONIO_DANO:
                // Animação do chefe tomando dano
                boss->acc_animacao += dt;
                if (boss->acc_animacao >= tpf_boss_dano) {
                    boss->acc_animacao -= tpf_boss_dano;
                    if(boss->frame_atual < BOSS_DEMON_DANO_FRAME_COUNT - 1) {
                        boss->frame_atual++;
                    } else {
                        // Fim da animação de dano, volta para IDLE
                        boss->estado = BOSS_DEMONIO_IDLE;
                        boss->frame_atual = 0;
                        boss->acc_animacao = 0;
                    }
                }
                break;

            case BOSS_DEMONIO_MORRENDO:
                // Animação de morte do chefe
                boss->acc_animacao += dt;
                if (boss->acc_animacao >= tpf_boss_morte) {
                    boss->acc_animacao -= tpf_boss_morte;
                    if(boss->frame_atual < BOSS_DEMON_MORTE_FRAME_COUNT - 1) {
                        boss->frame_atual++;
                    } else {
                        // Fim da animação de morte, desativa o chefe e marca como permanentemente derrotado
                        boss->ativa = false;
                        boss_defeated_permanently = true;
                        // O jogo irá para a tela de vitória após isso
                    }
                }
                break;
            default:
                break;
        }
    }
}


/**
 * @brief Processa todas as colisões entre entidades do jogo.
 * @param deslocamento_x Ponteiro para o deslocamento do mundo (pode ser ajustado).
 * @param old_deslocamento_x_prot Deslocamento X do protagonista no frame anterior.
 * @param movement_state Ponteiro para ProtagonistMovementState.
 * @param combat_state Ponteiro para ProtagonistCombatState.
 * @param inimigos Array de Inimigo.
 * @param garrafas Array de Garrafa.
 * @param granadas Array de Granada.
 * @param obstaculos Array de Obstaculo.
 * @param money_notes Array de MoneyNote.
 * @param player_money Ponteiro para o dinheiro do jogador.
 * @param boss Ponteiro para Boss.
 * @param boss_projectiles Array de BossProjectile.
 * @param garrafa_hitbox_width Largura da hitbox da garrafa.
 * @param garrafa_hitbox_height Altura da hitbox da garrafa.
 * @param boss_proj_largura_original Largura original do projétil do chefe.
 * @param boss_proj_altura_original Altura original do projétil do chefe.
 * @param escala_boss_proj Escala do projétil do chefe.
 * @param dims Ponteiro para SpriteDimensions.
 */
void processar_colisoes(float *deslocamento_x, float old_deslocamento_x_prot, ProtagonistMovementState *movement_state, ProtagonistCombatState *combat_state, Inimigo inimigos[], Garrafa garrafas[], Granada granadas[], Obstaculo obstaculos[], MoneyNote money_notes[], int *player_money, Boss *boss, BossProjectile boss_projectiles[], float garrafa_hitbox_width, float garrafa_hitbox_height, int boss_proj_largura_original, int boss_proj_altura_original, float escala_boss_proj, const SpriteDimensions *dims) {
    // Calcula a hitbox atual do protagonista
    float prot_hb_x = movement_state->x + movement_state->prot_hitbox_offset_x;
    float prot_hb_y = movement_state->y + movement_state->prot_hitbox_offset_y;
    float prot_hb_w = movement_state->prot_hitbox_width;
    float current_prot_hitbox_height = movement_state->agachando ? movement_state->prot_crouch_hitbox_height : movement_state->prot_hitbox_height;
    // Se agachado, ajusta a posição Y da hitbox para refletir a altura menor
    if(movement_state->agachando) prot_hb_y = movement_state->y + (movement_state->prot_hitbox_height - movement_state->prot_crouch_hitbox_height) + movement_state->prot_hitbox_offset_y;

    // Hitbox de ataque do protagonista (para ataques corpo a corpo)
    float prot_attack_hb_x = movement_state->x + movement_state->prot_attack_hitbox_offset_x;
    float prot_attack_hb_y = movement_state->y + movement_state->prot_attack_hitbox_offset_y;

    // Colisão Inimigos vs Protagonista e Ataque do Protagonista
    for (int i = 0; i < MAX_INIMIGOS; i++) {
        if (inimigos[i].ativa) {
            float inimigo_x_antes_movimento = inimigos[i].x; // Salva para reverter movimento em caso de colisão
            inimigos[i].x += inimigos[i].vel_x; // Aplica o movimento do inimigo para testar colisão

            float inimigo_screen_x = inimigos[i].x - *deslocamento_x; // Posição do inimigo na tela
            float inimigo_screen_hb_x = inimigo_screen_x + inimigos[i].hitbox_offset_x; // Posição X da hitbox do inimigo na tela
            float inimigo_hb_y = inimigos[i].y + inimigos[i].hitbox_offset_y; // Posição Y da hitbox do inimigo
            float inimigo_hb_w = inimigos[i].hitbox_width;
            float inimigo_hb_h = inimigos[i].hitbox_height;

            // Colisão: Ataque do Protagonista (especial/corpo-a-corpo) vs Inimigo
            if (movement_state->atacando && inimigos[i].estado != INIMIGO_MORRENDO) {
                if (check_collision(prot_attack_hb_x, prot_attack_hb_y, movement_state->prot_attack_hitbox_width, movement_state->prot_attack_hitbox_height,
                                    inimigo_screen_hb_x, inimigo_hb_y, inimigo_hb_w, inimigo_hb_h)) {
                    fprintf(stderr, "COLISÃO! Ataque Especial atingiu Inimigo %d! Iniciando animação de morte.\n", i);
                    inimigos[i].estado = INIMIGO_MORRENDO; // Manda o inimigo para o estado de morte
                    inimigos[i].vel_x = 0; // Para o inimigo
                    inimigos[i].frame_atual = 0;
                    inimigos[i].acc_animacao = 0;
                    inimigos[i].animacao_morte_finalizada = false;
                }
            }

            // Colisão com Protagonista: Lógica de Bloqueio e Dano (contato direto com Noia)
            if (inimigos[i].estado != INIMIGO_MORRENDO && combat_state->estado == PROT_NORMAL) {
                if (check_collision(inimigo_screen_hb_x, inimigo_hb_y, inimigo_hb_w, inimigo_hb_h,
                                    prot_hb_x, prot_hb_y, prot_hb_w, current_prot_hitbox_height)) {
                    // Se houver colisão, reverte os movimentos para evitar sobreposição
                    *deslocamento_x = old_deslocamento_x_prot; // Reverte o movimento do protagonista
                    inimigos[i].x = inimigo_x_antes_movimento; // Reverte o movimento do inimigo

                    // Se o inimigo pode dar dano (não está em cooldown) e o protagonista não está invulnerável
                    if (inimigos[i].inimigo_pode_dar_dano && !combat_state->invulnerable) {
                        // Noia causa dano por contato direto em seu estado de ataque
                        if (inimigos[i].type == NOIA && inimigos[i].estado == INIMIGO_ATACANDO) {
                            combat_state->health -= PROTAGONIST_DAMAGE_PER_HIT;
                            inimigos[i].inimigo_pode_dar_dano = false; // Previna dano contínuo do mesmo ataque
                            fprintf(stderr, "PROTAGONISTA SOFREU DANO DO NOIA! Vida restante: %d\n", combat_state->health);

                            // Ativa invulnerabilidade e efeito de piscar
                            combat_state->invulnerable = true;
                            combat_state->invulnerability_timer = PROTAGONIST_INVULNERABILITY_DURATION;
                            combat_state->blink_timer = 0.0;
                            combat_state->visible = false; // Esconde o personagem para o efeito de piscar
                        }
                        // Policial causa dano via granada, não contato direto, então não verifica dano de contato aqui
                    }
                    // Verifica se o protagonista morreu
                    if (combat_state->health <= 0) {
                        combat_state->estado = PROT_MORRENDO;
                        combat_state->personagem_frame_morte = 0;
                        combat_state->personagem_acc_morte = 0;
                        combat_state->animacao_morte_finalizada = false;
                        fprintf(stderr, "PROTAGONISTA MORREU! Iniciando animação de morte do protagonista.\n");
                    }
                }
            }
            // Colisão entre inimigos (para evitar que se sobreponham)
            float inimigo_world_hb_x = inimigos[i].x + inimigos[i].hitbox_offset_x;
            if (inimigos[i].estado != INIMIGO_MORRENDO) {
                for (int k = 0; k < MAX_INIMIGOS; k++) {
                    if (i == k) continue; // Não verifica colisão consigo mesmo
                    if (inimigos[k].ativa && inimigos[k].estado != INIMIGO_MORRENDO) {
                        float other_inimigo_world_hb_x = inimigos[k].x + inimigos[k].hitbox_offset_x;
                        float other_inimigo_hb_y = inimigos[k].y + inimigos[k].hitbox_offset_y;
                        float other_inimigo_hb_w = inimigos[k].hitbox_width;
                        float other_inimigo_hb_h = inimigos[k].hitbox_height;

                        if (check_collision(inimigo_world_hb_x, inimigo_hb_y, inimigo_hb_w, inimigo_hb_h,
                                            other_inimigo_world_hb_x, other_inimigo_hb_y, other_inimigo_hb_w, other_inimigo_hb_h)) {
                            // Resolve a sobreposição empurrando um dos inimigos
                            float overlap_amount = 0;
                            if (inimigo_world_hb_x < other_inimigo_world_hb_x) {
                                overlap_amount = (inimigo_world_hb_x + inimigo_hb_w) - other_inimigo_world_hb_x;
                                inimigos[i].x -= overlap_amount; // Empurra o inimigo 'i' para a esquerda
                            } else {
                                overlap_amount = (other_inimigo_world_hb_x + other_inimigo_hb_w) - inimigo_world_hb_x;
                                inimigos[i].x += overlap_amount; // Empurra o inimigo 'i' para a direita
                            }
                            inimigo_world_hb_x = inimigos[i].x + inimigos[i].hitbox_offset_x; // Atualiza a posição X da hitbox após o ajuste
                        }
                    }
                }
            }

            // Colisão: Garrafa do Protagonista vs Inimigo
            if (inimigos[i].estado != INIMIGO_MORRENDO) {
                for (int j = 0; j < MAX_GARRAFAS; j++) {
                    if (garrafas[j].ativa) {
                        float garrafa_hb_x = garrafas[j].x - garrafa_hitbox_width / 2.0;
                        float garrafa_hb_y = garrafas[j].y - garrafa_hitbox_height / 2.0;

                        if (check_collision(garrafa_hb_x, garrafa_hb_y, garrafa_hitbox_width, garrafa_hitbox_height,
                                            inimigos[i].x + inimigos[i].hitbox_offset_x, inimigo_hb_y, inimigo_hb_w, inimigo_hb_h)) {
                            fprintf(stderr, "COLISÃO! Garrafa %d atingiu Inimigo %d! Iniciando animação de morte.\n", j, i);
                            garrafas[j].ativa = false; // Desativa a garrafa

                            inimigos[i].estado = INIMIGO_MORRENDO; // Manda o inimigo para o estado de morte
                            inimigos[i].vel_x = 0;
                            inimigos[i].frame_atual = 0;
                            inimigos[i].acc_animacao = 0;
                            inimigos[i].animacao_morte_finalizada = false;
                            break; // Apenas uma garrafa pode atingir um inimigo por vez
                        }
                    }
                }
            }
            // Se a animação de morte do inimigo terminou, desativa-o para reciclagem
            if (inimigos[i].animacao_morte_finalizada) {
                inimigos[i].ativa = false;
            }
        }
    }

    // Colisão: Explosão da Granada (do Policial) vs Protagonista
    for (int i = 0; i < MAX_GRANADAS; i++) {
        // Se a granada estiver ativa, explodindo, e ainda não aplicou dano, e o protagonista não está invulnerável
        if (granadas[i].ativa && granadas[i].explodindo && !granadas[i].dano_aplicado && combat_state->estado == PROT_NORMAL && !combat_state->invulnerable) {
            float explosao_hb_w = dims->frame_largura_explosao * movement_state->escala_personagens;
            float explosao_hb_h = dims->frame_altura_explosao * movement_state->escala_personagens;
            float explosao_hb_x = (granadas[i].x - *deslocamento_x) - explosao_hb_w / 2.0; // Centraliza a hitbox da explosão
            float explosao_hb_y = granadas[i].y - explosao_hb_h / 2.0;

            if (check_collision(prot_hb_x, prot_hb_y, prot_hb_w, current_prot_hitbox_height,
                                explosao_hb_x, explosao_hb_y, explosao_hb_w, explosao_hb_h)) {
                combat_state->health -= GRENADE_DAMAGE;
                granadas[i].dano_aplicado = true; // Marca que o dano foi aplicado
                fprintf(stderr, "PROTAGONISTA ATINGIDO PELA GRANADA! Vida: %d\n", combat_state->health);

                // Ativa invulnerabilidade e efeito de piscar
                combat_state->invulnerable = true;
                combat_state->invulnerability_timer = PROTAGONIST_INVULNERABILITY_DURATION;
                combat_state->blink_timer = 0.0;
                combat_state->visible = false;

                if (combat_state->health <= 0) {
                    combat_state->estado = PROT_MORRENDO;
                }
            }
        }
    }

    // Colisão Jogador vs Chefe (ataques do jogador)
    // Apenas checa se o chefe está ativo, não está morrendo, e não está invulnerável
    if (boss->ativa && boss->estado >= BOSS_DEMONIO_IDLE && boss->estado != BOSS_DEMONIO_MORRENDO && boss->invulnerability_timer <= 0) {
        // Calcula a hitbox do chefe (um pouco menor que o sprite para ser mais justo)
        float boss_hb_w = boss->frame_largura_demonio_idle * boss->escala * 0.5f;
        float boss_hb_h = boss->frame_altura_demonio_idle * boss->escala * 0.8f;
        float boss_screen_x = boss->x - *deslocamento_x; // Posição do chefe na tela
        // Ajusta a posição da hitbox para centralizar no sprite (offset de 25% do sprite largura total)
        float boss_hb_x = boss_screen_x + (boss->frame_largura_demonio_idle * boss->escala * 0.25f);
        float boss_hb_y = boss->y + (boss->frame_altura_demonio_idle * boss->escala * 0.1f);

        // Colisão: Garrafa do jogador -> Chefe
        for (int j = 0; j < MAX_GARRAFAS; j++) {
            if (garrafas[j].ativa) {
                float garrafa_hb_x = garrafas[j].x - *deslocamento_x - garrafa_hitbox_width / 2.0f;
                float garrafa_hb_y = garrafas[j].y - garrafa_hitbox_height / 2.0f;
                if(check_collision(garrafa_hb_x, garrafa_hb_y, garrafa_hitbox_width, garrafa_hitbox_height, boss_hb_x, boss_hb_y, boss_hb_w, boss_hb_h)) {
                    garrafas[j].ativa = false; // Desativa a garrafa
                    boss->health -= PLAYER_ATTACK_DAMAGE_TO_BOSS; // Reduz a vida do chefe
                    boss->estado = BOSS_DEMONIO_DANO; // Chefe entra em estado de dano
                    boss->frame_atual = 0;
                    boss->acc_animacao = 0;
                    boss->invulnerability_timer = 0.5f; // Dá um breve período de invulnerabilidade ao chefe
                    fprintf(stderr, "DEBUG: Chefe atingido por garrafa! Vida: %d\n", boss->health);
                }
            }
        }

        // Colisão: Ataque especial do jogador (corpo a corpo) -> Chefe
        if (movement_state->atacando) {
            // prot_attack_hb_x e prot_attack_hb_y já estão calculados no início da função
            if(check_collision(prot_attack_hb_x, prot_attack_hb_y, movement_state->prot_attack_hitbox_width, movement_state->prot_attack_hitbox_height, boss_hb_x, boss_hb_y, boss_hb_w, boss_hb_h)) {
                boss->health -= PLAYER_ATTACK_DAMAGE_TO_BOSS; // Reduz a vida do chefe
                boss->estado = BOSS_DEMONIO_DANO; // Chefe entra em estado de dano
                boss->frame_atual = 0;
                boss->acc_animacao = 0;
                boss->invulnerability_timer = 0.5f; // Dá um breve período de invulnerabilidade ao chefe
                fprintf(stderr, "DEBUG: Chefe atingido por ataque especial! Vida: %d\n", boss->health);
            }
        }

        // Verifica se o chefe morreu
        if(boss->health <= 0) {
            boss->estado = BOSS_DEMONIO_MORRENDO; // Manda o chefe para o estado de morte
            boss->frame_atual = 0;
            boss->acc_animacao = 0;
        }
    }

    // Colisão Projétil do Chefe vs Jogador
    for (int i = 0; i < MAX_BOSS_PROJECTILES; i++) {
        // Se o projétil estiver ativo, o protagonista estiver normal e não invulnerável
        if (boss_projectiles[i].ativa && combat_state->estado == PROT_NORMAL && !combat_state->invulnerable) {
            float proj_hb_w = boss_proj_largura_original * escala_boss_proj;
            float proj_hb_h = boss_proj_altura_original * escala_boss_proj;
            float proj_screen_x = boss_projectiles[i].x - *deslocamento_x; // Posição do projétil na tela

            // Se colidiu E o protagonista NÃO ESTIVER agachado (para permitir desviar agachando)
            if((check_collision(prot_hb_x, prot_hb_y, prot_hb_w, current_prot_hitbox_height, proj_screen_x, boss_projectiles[i].y, proj_hb_w, proj_hb_h)) && !movement_state->agachando) {
                boss_projectiles[i].ativa = false; // Desativa o projétil
                combat_state->health -= BOSS_PROJECTILE_DAMAGE; // Reduz a vida do protagonista
                fprintf(stderr, "JOGADOR ATINGIDO PELO CHEFE! Vida: %d\n", combat_state->health);

                // Ativa invulnerabilidade e efeito de piscar
                combat_state->invulnerable = true;
                combat_state->invulnerability_timer = PROTAGONIST_INVULNERABILITY_DURATION;
                combat_state->blink_timer = 0.0;
                combat_state->visible = false;

                if(combat_state->health <= 0) {
                    combat_state->estado = PROT_MORRENDO; // Manda o protagonista para o estado de morte
                }
            }
        }
    }

    // Colisão Protagonista-Obstáculo e Coleta de Dinheiro
    if (combat_state->estado == PROT_NORMAL) { // Só processa se o protagonista estiver vivo e normal
        bool on_obstacle = false; // Flag para saber se o personagem está sobre um obstáculo

        float prot_current_hb_x = movement_state->x + movement_state->prot_hitbox_offset_x;
        float prot_current_hb_y = prot_hb_y; // Já ajustado para agachamento
        float prot_current_hb_w = movement_state->prot_hitbox_width;
        float prot_current_hb_h = current_prot_hitbox_height; // Já ajustado para agachamento

        // Colisão horizontal com obstáculos (para impedir passagem)
        for (int i = 0; i < MAX_OBSTACULOS; i++) {
            if (obstaculos[i].ativa) {
                float obstacle_screen_x = obstaculos[i].x - *deslocamento_x;
                float obstacle_top = obstaculos[i].y;
                float obstacle_width = obstaculos[i].width;
                float obstacle_height = obstaculos[i].height;

                if (check_collision(prot_current_hb_x, prot_current_hb_y, prot_current_hb_w, prot_current_hb_h,
                                    obstacle_screen_x, obstacle_top, obstacle_width, obstacle_height)) {

                    // Se não estiver pulando E (não for um obstáculo de agachar OU não estiver agachado)
                    bool should_block_horizontally = !movement_state->pulando && !(obstaculos[i].only_crouch_pass && movement_state->agachando);

                    if (should_block_horizontally) {
                        *deslocamento_x = old_deslocamento_x_prot; // Reverte o movimento horizontal do protagonista
                        break; // Sai do loop, pois já lidou com a colisão mais próxima
                    }
                }
            }
        }

        // Colisão vertical com obstáculos (para pousar neles)
        for (int i = 0; i < MAX_OBSTACULOS; i++) {
            if (obstaculos[i].ativa) {
                float obstacle_screen_x = obstaculos[i].x - *deslocamento_x;
                float obstacle_top = obstaculos[i].y;

                // Se estiver caindo (vel_y > 0) e o topo do personagem no frame anterior estava acima do obstáculo, mas no atual está abaixo
                // E há sobreposição horizontal (para garantir que está "em cima" do obstáculo)
                if (movement_state->pulando && movement_state->vel_y > 0 &&
                    (movement_state->y + movement_state->prot_hitbox_offset_y + prot_current_hb_h) <= obstacle_top &&
                    (movement_state->y + movement_state->prot_hitbox_offset_y + prot_current_hb_h) > obstacle_top &&
                    (prot_current_hb_x + prot_current_hb_w > obstacle_screen_x) &&
                    (prot_current_hb_x < obstacle_screen_x + obstaculos[i].width)) {

                    if (!obstaculos[i].only_crouch_pass) { // Não pode pousar em obstáculos que só se passa agachado
                        movement_state->y = obstacle_top - movement_state->prot_hitbox_offset_y - prot_current_hb_h; // Ajusta Y para pousar
                        movement_state->pulando = false;
                        movement_state->vel_y = 0;
                        movement_state->vel_x_pulo = 0;
                        movement_state->current_ground_y = movement_state->y; // O novo "chão" é o topo do obstáculo
                        on_obstacle = true;
                        break;
                    }
                }

                // Se não estiver pulando e não estiver já sobre um obstáculo, e estiver "quase" na altura do topo do obstáculo
                // E há sobreposição horizontal
                if (!movement_state->pulando && !on_obstacle &&
                    (movement_state->y + movement_state->prot_hitbox_offset_y + prot_current_hb_h > obstacle_top - 5) && // Pequena tolerância para pouso
                    (movement_state->y + movement_state->prot_hitbox_offset_y + prot_current_hb_h < obstacle_top + 5) &&
                    (prot_current_hb_x + prot_current_hb_w > obstacle_screen_x) &&
                    (prot_current_hb_x < obstacle_screen_x + obstaculos[i].width)) {

                    if (!obstaculos[i].only_crouch_pass) {
                        on_obstacle = true;
                        movement_state->current_ground_y = movement_state->y; // O novo "chão" é o topo do obstáculo
                    }
                }
            }
        }

        // Se não pousou em nenhum obstáculo e está acima do chão base, reseta o chão para o chão base
        if (!on_obstacle && !movement_state->pulando && movement_state->y < movement_state->personagem_y_base) {
            movement_state->current_ground_y = movement_state->personagem_y_base;
        }
    }

    // Coleta de Dinheiro
    if (combat_state->estado == PROT_NORMAL) { // Só coleta se o protagonista estiver vivo e normal
        for (int i = 0; i < MAX_MONEY_NOTES; i++) {
            if (money_notes[i].ativa) {
                float money_note_hb_x = money_notes[i].x - *deslocamento_x;
                float money_note_hb_y = money_notes[i].y;
                float money_note_hb_w = money_notes[i].hitbox_width;
                float money_note_hb_h = money_notes[i].hitbox_height;

                // Colisão entre a hitbox do protagonista e a da nota de dinheiro
                if (check_collision(prot_hb_x, prot_hb_y, prot_hb_w, current_prot_hitbox_height,
                                    money_note_hb_x, money_note_hb_y, money_note_hb_w, money_note_hb_h)) {
                    *player_money += money_notes[i].value; // Adiciona o valor da nota ao dinheiro do jogador
                    money_notes[i].ativa = false; // Desativa a nota de dinheiro
                    fprintf(stderr, "DEBUG: Coletou R$%d! Total: R$%d\n", money_notes[i].value, *player_money);
                }
            }
        }
    }
}

/**
 * @brief Desenha o background e todos os obstáculos.
 * @param resources Ponteiro para GameResources.
 * @param deslocamento_x Deslocamento do mundo.
 * @param obstaculos Array de Obstaculo.
 */
void desenhar_mundo(const GameResources *resources, float deslocamento_x, const Obstaculo obstaculos[]) {
    int bg_width = al_get_bitmap_width(resources->background);
    int bg_height = al_get_bitmap_height(resources->background);

    // Calcula o offset para criar o efeito de tiling/paralaxe do background
    int offset_x_bg = (int)fmod(deslocamento_x, bg_width);
    if (offset_x_bg < 0) offset_x_bg += bg_width; // Garante que o offset seja positivo

    // Desenha a primeira parte do background
    al_draw_scaled_bitmap(resources->background,
                         offset_x_bg, 0,
                         bg_width - offset_x_bg, bg_height,
                         0, 0,
                         (float)(bg_width - offset_x_bg), (float)ALTURA, 0);

    // Se a primeira parte não preencheu a tela (porque o offset empurrou parte dela para fora), desenha a parte restante
    if ((bg_width - offset_x_bg) < LARGURA) {
        al_draw_scaled_bitmap(resources->background,
                             0, 0,
                             LARGURA - (bg_width - offset_x_bg), bg_height,
                             (float)(bg_width - offset_x_bg), 0,
                             (float)(LARGURA - (bg_width - offset_x_bg)), (float)ALTURA, 0);
    }

    // Desenha Obstáculos
    for (int i = 0; i < MAX_OBSTACULOS; i++) {
        if (obstaculos[i].ativa) {
            float draw_x = obstaculos[i].x - deslocamento_x; // Posição na tela = posição no mundo - deslocamento da câmera
            al_draw_scaled_bitmap(obstaculos[i].sprite_bitmap,
                                 0, 0,
                                 al_get_bitmap_width(obstaculos[i].sprite_bitmap), al_get_bitmap_height(obstaculos[i].sprite_bitmap),
                                 draw_x, obstaculos[i].y,
                                 obstaculos[i].width, obstaculos[i].height,
                                 0);
        }
    }
}

/**
 * @brief Desenha todos os personagens e projéteis (protagonista, inimigos, garrafas, granadas, chefe, projéteis do chefe, NPC).
 * @param resources Ponteiro para GameResources.
 * @param deslocamento_x Deslocamento do mundo.
 * @param movement_state Ponteiro para ProtagonistMovementState.
 * @param combat_state Ponteiro para ProtagonistCombatState.
 * @param dims Ponteiro para SpriteDimensions.
 * @param garrafas Array de Garrafa.
 * @param granadas Array de Granada.
 * @param inimigos Array de Inimigo.
 * @param money_notes Array de MoneyNote.
 * @param traficante Ponteiro para NPC.
 * @param boss Ponteiro para Boss.
 * @param boss_projectiles Array de BossProjectile.
 */
void desenhar_personagens_projeteis(const GameResources *resources, float deslocamento_x, const ProtagonistMovementState *movement_state, const ProtagonistCombatState *combat_state, const SpriteDimensions *dims, const Garrafa garrafas[], const Granada granadas[], const Inimigo inimigos[], const MoneyNote money_notes[], const NPC *traficante, const Boss *boss, const BossProjectile boss_projectiles[], ALLEGRO_FONT *fonte) {
    // Escalas fixas usadas para desenho
    float escala_traficante = NPC_TRAFICANTE_SCALE;
    float escala_garrafa = 1.0f;
    int garrafa_largura_original = al_get_bitmap_width(resources->sprite_garrafa);
    int garrafa_altura_original = al_get_bitmap_height(resources->sprite_garrafa);
    int boss_proj_largura_original = al_get_bitmap_width(resources->sprite_demon_projetil);
    int boss_proj_altura_original = al_get_bitmap_height(resources->sprite_demon_projetil);
    float escala_boss_proj = 2.0f;

    // É necessário obter o estado do teclado UMA VEZ e armazená-lo
    // em uma variável local, para depois passar o ENDEREÇO dessa variável
    // para a função al_key_down().
    ALLEGRO_KEYBOARD_STATE keyboard_state_for_drawing;
    al_get_keyboard_state(&keyboard_state_for_drawing);


    // Desenha NPC Traficante
    if (traficante->ativa) {
        float draw_x = traficante->x - deslocamento_x; // Posição do NPC na tela
        al_draw_scaled_bitmap(resources->sprite_traficante_parada,
                             traficante->frame_atual * traficante->largura_sprite, 0,
                             traficante->largura_sprite, traficante->altura_sprite,
                             draw_x, traficante->y,
                             traficante->largura_sprite * escala_traficante, traficante->altura_sprite * escala_traficante,
                             ALLEGRO_FLIP_HORIZONTAL); // Inverte horizontalmente para ele "olhar" para o jogador
    }

    // Desenha a Entrada do Chefe (gato preto)
    // Desenha se o chefe estiver ativo OU se ele já foi derrotado, mas a entrada ainda está visível na tela
    if (boss->ativa || (boss_defeated_permanently && (BOSS_SPAWN_X - deslocamento_x < LARGURA))) {
         al_draw_scaled_bitmap(resources->sprite_gatopreto,
                             0, 0,
                             al_get_bitmap_width(resources->sprite_gatopreto), al_get_bitmap_height(resources->sprite_gatopreto),
                             BOSS_SPAWN_X - deslocamento_x, 0, // Posição da entrada na tela
                             al_get_bitmap_width(resources->sprite_gatopreto), ALTURA, 0); // Escala para preencher a altura da tela
    }

    // Desenha o Chefe
    if (boss->ativa) {
        float draw_x = boss->x - deslocamento_x; // Posição do chefe na tela
        float draw_y = boss->y;
        ALLEGRO_BITMAP* current_boss_sprite = NULL;
        int current_frame_w = 0, current_frame_h = 0;

        // Seleciona o sprite e as dimensões do frame com base no estado atual do chefe
        switch(boss->estado) {
            case BOSS_PARADA:
                current_boss_sprite = boss->sprite_parada;
                current_frame_w = boss->frame_largura_parada; current_frame_h = boss->frame_altura_parada;
                break;
            case BOSS_TRANSFORMANDO:
                current_boss_sprite = boss->sprite_trans1;
                current_frame_w = boss->frame_largura_trans1; current_frame_h = boss->frame_altura_trans1;
                break;
            case BOSS_TRANSFORMANDO_2:
                current_boss_sprite = boss->sprite_trans2;
                current_frame_w = boss->frame_largura_trans2; current_frame_h = boss->frame_altura_trans2;
                break;
            case BOSS_DEMONIO_IDLE:
                current_boss_sprite = boss->sprite_demonio_idle;
                current_frame_w = boss->frame_largura_demonio_idle; current_frame_h = boss->frame_altura_demonio_idle;
                break;
            case BOSS_DEMONIO_ATACANDO:
                current_boss_sprite = boss->sprite_demonio_ataque;
                current_frame_w = boss->frame_largura_demonio_ataque; current_frame_h = boss->frame_altura_demonio_ataque;
                break;
            case BOSS_DEMONIO_DANO:
                current_boss_sprite = boss->sprite_demonio_dano;
                current_frame_w = boss->frame_largura_demonio_dano; current_frame_h = boss->frame_altura_demonio_dano;
                break;
            case BOSS_DEMONIO_MORRENDO:
                current_boss_sprite = boss->sprite_demonio_morte;
                current_frame_w = boss->frame_largura_demonio_morte; current_frame_h = boss->frame_altura_demonio_morte;
                break;
            default:
                break;
        }
        // Desenha o chefe se o sprite atual for válido
        if(current_boss_sprite) {
            al_draw_scaled_bitmap(current_boss_sprite,
                                 boss->frame_atual * current_frame_w, 0, // Frame de origem no bitmap
                                 current_frame_w, current_frame_h,
                                 draw_x, draw_y, // Posição de destino na tela
                                 current_frame_w * boss->escala, current_frame_h * boss->escala, // Tamanho de destino na tela
                                 ALLEGRO_FLIP_HORIZONTAL); // Chefe sempre virado para a esquerda
            if (boss->estado == BOSS_PARADA) {
    // Calcula a posição central do protagonista no mundo
    float prot_world_x_center = (movement_state->x + (dims->frame_largura_parado * movement_state->escala_personagens) / 2.0) + deslocamento_x;
    // Calcula a posição central do chefe no mundo
    float boss_world_x_center = boss->x + (boss->frame_largura_parada * boss->escala) / 2.0;
    // Calcula a distância entre eles
    float distance_to_boss = fabs(prot_world_x_center - boss_world_x_center);

    // Se o jogador estiver perto o suficiente, desenha o texto
    if (distance_to_boss < BOSS_INTERACTION_DISTANCE) {
        // Posição X centralizada acima do sprite do chefe na tela
        float text_x = (boss->x - deslocamento_x) + (boss->frame_largura_parada * boss->escala) / 2.0;
        // Posição Y um pouco acima da cabeça do chefe
        float text_y = boss->y - 40; 

        al_draw_text(fonte, al_map_rgb(255, 255, 255), text_x, text_y, ALLEGRO_ALIGN_CENTER, "Interagir - Pressione B");
    }
}
        }
    }

    // Desenho do Protagonista
    if (combat_state->estado == PROT_MORRENDO) {
        // Desenha a animação de morte do personagem
        al_draw_scaled_bitmap(resources->sprite_personagem_morte,
                             combat_state->personagem_frame_morte * dims->frame_largura_personagem_morte, 0,
                             dims->frame_largura_personagem_morte, dims->frame_altura,
                             movement_state->x, movement_state->y,
                             dims->frame_largura_personagem_morte * movement_state->escala_personagens, dims->frame_altura * movement_state->escala_personagens, 0);
    } else if (combat_state->visible) { // Desenha o personagem apenas se estiver visível (efeito de piscar)
        if (movement_state->pulando)
            al_draw_scaled_bitmap(resources->sprite_pulando, movement_state->frame_pulando * dims->frame_largura_pulando, 0, dims->frame_largura_pulando, dims->frame_altura,
                                 movement_state->x, movement_state->y, dims->frame_largura_pulando * movement_state->escala_personagens, dims->frame_altura * movement_state->escala_personagens, 0);
        else if (movement_state->agachando)
            al_draw_scaled_bitmap(resources->sprite_agachado, movement_state->frame_agachado * dims->frame_largura_agachado, 0, dims->frame_largura_agachado, dims->frame_altura,
                                 movement_state->x, movement_state->y, dims->frame_largura_agachado * movement_state->escala_personagens, dims->frame_altura * movement_state->escala_personagens, 0);
        else if (movement_state->atacando)
            al_draw_scaled_bitmap(resources->sprite_ataque1, movement_state->frame_ataque1 * dims->frame_largura_ataque1, 0, dims->frame_largura_ataque1, dims->frame_altura,
                                 movement_state->x, movement_state->y, dims->frame_largura_ataque1 * movement_state->escala_personagens, dims->frame_altura * movement_state->escala_personagens, 0);
        else if (movement_state->especial_ativo)
            al_draw_scaled_bitmap(resources->sprite_especial, movement_state->frame_especial * dims->frame_largura_especial, 0, dims->frame_largura_especial, dims->frame_altura,
                                 movement_state->x, movement_state->y, dims->frame_largura_especial * movement_state->escala_personagens, dims->frame_altura * movement_state->escala_personagens, 0);
        else if (movement_state->arremessando)
            al_draw_scaled_bitmap(resources->sprite_arremesso, movement_state->frame_arremesso * dims->frame_largura_arremesso, 0, dims->frame_largura_arremesso, dims->frame_altura,
                                 movement_state->x, movement_state->y, dims->frame_largura_arremesso * movement_state->escala_personagens, dims->frame_altura * movement_state->escala_personagens, 0);
        else if (movement_state->vel_x_pulo != 0 || al_key_down(&keyboard_state_for_drawing, ALLEGRO_KEY_RIGHT) || al_key_down(&keyboard_state_for_drawing, ALLEGRO_KEY_LEFT)) // Andando ou correndo
            al_draw_scaled_bitmap((movement_state->velocidade == movement_state->velocidade_correr) ? resources->sprite_correndo : resources->sprite_andando,
                                 ((movement_state->velocidade == movement_state->velocidade_correr) ? movement_state->frame_correndo : movement_state->frame_andando) * ((movement_state->velocidade == movement_state->velocidade_correr) ? dims->frame_largura_correndo : dims->frame_largura_andando),
                                 0,
                                 ((movement_state->velocidade == movement_state->velocidade_correr) ? dims->frame_largura_correndo : dims->frame_largura_andando),
                                 dims->frame_altura,
                                 movement_state->x, movement_state->y,
                                 ((movement_state->velocidade == movement_state->velocidade_correr) ? dims->frame_largura_correndo : dims->frame_largura_andando) * movement_state->escala_personagens,
                                 dims->frame_altura * movement_state->escala_personagens, 0);
        else // Parado
            al_draw_scaled_bitmap(resources->sprite_parado, movement_state->frame_parado * dims->frame_largura_parado, 0, dims->frame_largura_parado, dims->frame_altura,
                                 movement_state->x, movement_state->y, dims->frame_largura_parado * movement_state->escala_personagens, dims->frame_altura * movement_state->escala_personagens, 0);
    }

    // Desenha Garrafas
    for (int i = 0; i < MAX_GARRAFAS; i++) {
        if (garrafas[i].ativa) {
            al_draw_scaled_rotated_bitmap(resources->sprite_garrafa,
                                         garrafa_largura_original / 2.0, garrafa_altura_original / 2.0, // Centro de rotação
                                         garrafas[i].x - deslocamento_x, garrafas[i].y, // Posição na tela
                                         escala_garrafa, escala_garrafa, // Escala
                                         garrafas[i].angulo, // Ângulo de rotação
                                         0);
        }
    }

    // Desenha Projéteis do Chefe
    for (int i = 0; i < MAX_BOSS_PROJECTILES; i++) {
        if (boss_projectiles[i].ativa) {
            al_draw_scaled_rotated_bitmap(resources->sprite_demon_projetil,
                                         boss_proj_largura_original / 2.0, boss_proj_altura_original / 2.0, // Centro de rotação
                                         boss_projectiles[i].x - deslocamento_x, boss_projectiles[i].y, // Posição na tela
                                         escala_boss_proj, escala_boss_proj, // Escala
                                         boss_projectiles[i].angulo, 0); // Ângulo de rotação
        }
    }

    // Desenha Inimigos
    for (int i = 0; i < MAX_INIMIGOS; i++) {
        if (inimigos[i].ativa) {
            ALLEGRO_BITMAP *current_enemy_sprite = NULL;
            int current_frame_largura_inimigo = 0;

            // Seleciona o sprite e as dimensões do frame com base no tipo e estado do inimigo
            if (inimigos[i].type == NOIA) {
                switch(inimigos[i].estado) {
                    case INIMIGO_PARADO: current_enemy_sprite = resources->sprite_inimigo_parado; current_frame_largura_inimigo = dims->frame_largura_inimigo_parado; break;
                    case INIMIGO_ANDANDO: current_enemy_sprite = resources->sprite_inimigo_andando; current_frame_largura_inimigo = dims->frame_largura_inimigo_andando; break;
                    case INIMIGO_ATACANDO: current_enemy_sprite = resources->sprite_inimigo_ataque; current_frame_largura_inimigo = dims->frame_largura_inimigo_ataque; break;
                    case INIMIGO_MORRENDO: current_enemy_sprite = resources->sprite_inimigo_morte; current_frame_largura_inimigo = dims->frame_largura_inimigo_morte; break;
                }
            } else { // POLICIAL
                 switch(inimigos[i].estado) {
                    case INIMIGO_PARADO:
                    case INIMIGO_ANDANDO:
                        current_enemy_sprite = resources->sprite_policial_parado;
                        current_frame_largura_inimigo = dims->frame_largura_policial_parado;
                        break;
                    case INIMIGO_ATACANDO:
                        current_enemy_sprite = resources->sprite_policial_arremesso;
                        current_frame_largura_inimigo = dims->frame_largura_policial_arremesso;
                        break;
                    case INIMIGO_MORRENDO:
                        current_enemy_sprite = resources->sprite_policial_morte; // Usa o sprite de morte do policial
                        current_frame_largura_inimigo = dims->frame_largura_policial_morte; // Usa a largura do frame do policial
                        break;
                }
            }
            // Desenha o inimigo se o sprite atual for válido
            if (current_enemy_sprite) {
                al_draw_scaled_bitmap(current_enemy_sprite,
                                     inimigos[i].frame_atual * current_frame_largura_inimigo, 0, // Frame de origem no bitmap
                                     current_frame_largura_inimigo, dims->inimigo_altura_sprite,
                                     inimigos[i].x - deslocamento_x, inimigos[i].y, // Posição de destino na tela
                                     current_frame_largura_inimigo * movement_state->escala_personagens, dims->inimigo_altura_sprite * movement_state->escala_personagens,
                                     ALLEGRO_FLIP_HORIZONTAL); // Inimigos sempre virados para a esquerda
            }
        }
    }

    // Desenha Granadas e Explosões
    for (int i = 0; i < MAX_GRANADAS; i++) {
        if (granadas[i].ativa) {
            if (granadas[i].explodindo) {
                // Se está explodindo, desenha o frame da explosão
                float draw_x = (granadas[i].x - deslocamento_x) - (dims->frame_largura_explosao * movement_state->escala_personagens / 2.0);
                float draw_y = granadas[i].y - (dims->frame_altura_explosao * movement_state->escala_personagens / 2.0);
                al_draw_scaled_bitmap(resources->sprite_explosao,
                                     granadas[i].frame_explosao * dims->frame_largura_explosao, 0,
                                     dims->frame_largura_explosao, dims->frame_altura_explosao,
                                     draw_x, draw_y,
                                     dims->frame_largura_explosao * movement_state->escala_personagens, dims->frame_altura_explosao * movement_state->escala_personagens,
                                     0);
            } else {
                // Se não está explodindo, desenha a granada como um círculo (não temos sprite para a granada não explodida)
                al_draw_filled_circle(granadas[i].x - deslocamento_x, granadas[i].y, 8, al_map_rgb(50, 80, 50));
                al_draw_circle(granadas[i].x - deslocamento_x, granadas[i].y, 8, al_map_rgb(20, 30, 20), 2);
            }
        }
    }

    // Desenha Notas de Dinheiro
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
}

/**
 * @brief Desenha o HUD (barra de vida do protagonista, dinheiro do jogador, texto do NPC, barra de vida do chefe).
 * @param fonte Fonte para o texto.
 * @param combat_state Ponteiro para ProtagonistCombatState.
 * @param player_money Dinheiro do jogador.
 * @param traficante Ponteiro para NPC.
 * @param boss Ponteiro para Boss.
 * @param deslocamento_x Deslocamento do mundo (usado para o texto do NPC).
 */
void desenhar_hud(ALLEGRO_FONT *fonte, const ProtagonistCombatState *combat_state, int player_money, const NPC *traficante, const Boss *boss, float deslocamento_x) {
    // HUD do Protagonista (Barra de Vida)
    float health_bar_width = (float)combat_state->health / (MAX_PROTAGONIST_HEALTH + NPC_HEAL_AMOUNT) * 200;
    if (health_bar_width < 0) health_bar_width = 0; // Garante que a barra não seja negativa
    ALLEGRO_COLOR health_color = al_map_rgb(0, 255, 0); // Verde padrão
    if (combat_state->health > MAX_PROTAGONIST_HEALTH) {
        health_color = al_map_rgb(0, 255, 255); // Azul ciano se a vida estiver acima do máximo
    }
    al_draw_filled_rectangle(10, 10, 10 + health_bar_width, 30, health_color); // Barra preenchida
    al_draw_rectangle(10, 10, 10 + 200, 30, al_map_rgb(255, 255, 255), 2); // Borda da barra
    al_draw_textf(fonte, al_map_rgb(255, 255, 255), 220, 15, 0, "HP: %d", combat_state->health); // Texto da vida

    // HUD do Protagonista (Dinheiro)
    al_draw_textf(fonte, al_map_rgb(255, 255, 0), LARGURA - 10, 10, ALLEGRO_ALIGN_RIGHT, "R$ %d", player_money);

    // Texto de interação do NPC
    if (traficante->ativa) {
        float escala_traficante = NPC_TRAFICANTE_SCALE;
        // Posição do texto na tela, alinhada com o NPC e considerando o deslocamento do mundo
        float text_x_on_screen = (traficante->x - deslocamento_x) + (traficante->largura_sprite * escala_traficante) / 2.0;
        float text_y = traficante->y + 70;
        al_draw_text(fonte, al_map_rgb(255, 255, 255), text_x_on_screen, text_y, ALLEGRO_ALIGN_CENTER, "Aperte B para");
        al_draw_text(fonte, al_map_rgb(255, 255, 255), text_x_on_screen, text_y + al_get_font_line_height(fonte), ALLEGRO_ALIGN_CENTER, "comprar pó");
        al_draw_textf(fonte, al_map_rgb(255, 255, 255), text_x_on_screen, text_y + al_get_font_line_height(fonte) * 2, ALLEGRO_ALIGN_CENTER, "(+20 HP)");
        al_draw_textf(fonte, al_map_rgb(255, 255, 255), text_x_on_screen, text_y + al_get_font_line_height(fonte) * 3, ALLEGRO_ALIGN_CENTER, "R$%d", NPC_HEAL_COST);
    }

    // Barra de Vida do Chefe
    if (boss->ativa && boss->estado >= BOSS_DEMONIO_IDLE) { // Só mostra a barra se o chefe estiver transformado e ativo
        float boss_health_bar_w = 400;
        float boss_health_bar_h = 25;
        float boss_health_bar_x = LARGURA / 2 - boss_health_bar_w / 2; // Centraliza a barra horizontalmente
        float boss_health_bar_y = 20;

        float current_health_w = ((float)boss->health / MAX_BOSS_HEALTH) * boss_health_bar_w;
        if(current_health_w < 0) current_health_w = 0;

        al_draw_filled_rectangle(boss_health_bar_x, boss_health_bar_y, boss_health_bar_x + boss_health_bar_w, boss_health_bar_y + boss_health_bar_h, al_map_rgb(50, 0, 0)); // Fundo da barra (vermelho escuro)
        al_draw_filled_rectangle(boss_health_bar_x, boss_health_bar_y, boss_health_bar_x + current_health_w, boss_health_bar_y + boss_health_bar_h, al_map_rgb(200, 0, 50)); // Vida atual (vermelho brilhante)
        al_draw_rectangle(boss_health_bar_x, boss_health_bar_y, boss_health_bar_x + boss_health_bar_w, boss_health_bar_y + boss_health_bar_h, al_map_rgb(255, 255, 255), 2); // Borda branca
        al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 2, boss_health_bar_y + boss_health_bar_h + 5, ALLEGRO_ALIGN_CENTER, "Lilith, a Profana"); // Nome do chefe
    }
}


// --- Função da Tela de Vitória ---
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
        al_wait_for_event(fila, &ev); // Espera por um evento

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
                    sair_victory_screen = true; // Sai da tela de vitória
                    break;
                case ALLEGRO_KEY_ESCAPE: // Permite sair também com ESC
                    restart_game = false;
                    sair_victory_screen = true;
                    break;
            }
        } else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) { // Se o usuário fechar a janela
            restart_game = false;
            sair_victory_screen = true;
        }

        // Desenha o background da tela de vitória (escalado para preencher a janela)
        al_draw_scaled_bitmap(background_victory, 0, 0,
                                 al_get_bitmap_width(background_victory), al_get_bitmap_height(background_victory),
                                 0, 0, LARGURA, ALTURA, 0);

        // Desenha a imagem de vitória (centralizada)
        al_draw_scaled_bitmap(victory_image, 0, 0,
                                 al_get_bitmap_width(victory_image), al_get_bitmap_height(victory_image),
                                 (LARGURA - al_get_bitmap_width(victory_image)) / 2.0,
                                 (ALTURA / 2.0 - al_get_bitmap_height(victory_image) / 2.0) - 50,
                                 al_get_bitmap_width(victory_image), al_get_bitmap_height(victory_image), 0);

        // Mensagem principal
        al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 2, ALTURA / 2 + 50, ALLEGRO_ALIGN_CENTER, "VOCÊ CHEGOU AO PARAÍSO DE MAOMÉ!");

        // Desenha as opções do menu de vitória
        int start_y = ALTURA / 2 + 130;
        for (int i = 0; i < num_opcoes_victory; i++) {
            ALLEGRO_COLOR cor_texto = (i == opcao_selecionada_v) ? al_map_rgb(128, 0, 128) : al_map_rgb(255, 255, 255); // Roxo para selecionado, branco para outros
            al_draw_text(fonte, cor_texto, LARGURA / 2, start_y + i * al_get_font_line_height(fonte) * 1.5, ALLEGRO_ALIGN_CENTER, opcoes_victory[i]);
        }

        al_flip_display(); // Atualiza a tela
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
        al_wait_for_event(fila, &ev); // Espera por um evento

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
                    sair_game_over_screen = true; // Sai da tela de game over
                    break;
                case ALLEGRO_KEY_ESCAPE: // Permite sair também com ESC
                    restart_game = false;
                    sair_game_over_screen = true;
                    break;
            }
        } else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) { // Se o usuário fechar a janela
            restart_game = false;
            sair_game_over_screen = true;
        }

        // Desenha o background da tela de game over (escalado para preencher a janela)
        al_draw_scaled_bitmap(background_game_over, 0, 0,
                                 al_get_bitmap_width(background_game_over), al_get_bitmap_height(background_game_over),
                                 0, 0, LARGURA, ALTURA, 0);

        // Desenha a imagem de game over (centralizada)
        al_draw_scaled_bitmap(game_over_image, 0, 0,
                                 al_get_bitmap_width(game_over_image), al_get_bitmap_height(game_over_image),
                                 (LARGURA - al_get_bitmap_width(game_over_image)) / 2.0,
                                 (ALTURA / 2.0 - al_get_bitmap_height(game_over_image) / 2.0) - 50,
                                 al_get_bitmap_width(game_over_image), al_get_bitmap_height(game_over_image), 0);

        // Desenha as opções do menu de game over
        int start_y = ALTURA / 2 + 130;
        for (int i = 0; i < num_opcoes_game_over; i++) {
            ALLEGRO_COLOR cor_texto = (i == opcao_selecionada_go) ? al_map_rgb(0, 255, 100) : al_map_rgb(255, 255, 255); // Verde para selecionado, branco para outros
            al_draw_text(fonte, cor_texto, LARGURA / 2, start_y + i * al_get_font_line_height(fonte) * 1.5, ALLEGRO_ALIGN_CENTER, opcoes_game_over[i]);
        }

        al_flip_display(); // Atualiza a tela
    }

    al_destroy_bitmap(game_over_image);
    al_destroy_bitmap(background_game_over);
    return restart_game;
}

/**
 * @brief Reinicia o estado do jogo para uma nova partida.
 * Esta função é chamada ao iniciar um novo jogo ou após Game Over/Vitória.
 * @param deslocamento_x Ponteiro para o deslocamento do mundo.
 * @param player_money Ponteiro para o dinheiro do jogador.
 * @param combat_state Ponteiro para ProtagonistCombatState.
 * @param movement_state Ponteiro para ProtagonistMovementState.
 * @param garrafas Array de Garrafa.
 * @param granadas Array de Granada.
 * @param boss_projectiles Array de BossProjectile.
 * @param inimigos Array de Inimigo.
 * @param obstaculos Array de Obstaculo.
 * @param money_notes Array de MoneyNote.
 * @param traficante Ponteiro para NPC.
 * @param boss Ponteiro para Boss.
 * @param dims Ponteiro para SpriteDimensions.
 * @param resources Ponteiro para GameResources.
 * @param out_proximo_ponto_spawn_obstaculo Ponteiro para float, próximo ponto de spawn de obstáculo.
 * @param out_time_to_spawn_money Ponteiro para float, tempo para o próximo spawn de dinheiro.
 * @param out_tempo_para_spawn_inimigo Ponteiro para float, tempo para o próximo spawn de inimigo.
 */
void reset_game_state(float *deslocamento_x, int *player_money, ProtagonistCombatState *combat_state, ProtagonistMovementState *movement_state, Garrafa garrafas[], Granada granadas[], BossProjectile boss_projectiles[], Inimigo inimigos[], Obstaculo obstaculos[], MoneyNote money_notes[], NPC *traficante, Boss *boss, const SpriteDimensions *dims, const GameResources *resources, float *out_proximo_ponto_spawn_obstaculo, float *out_time_to_spawn_money, float *out_tempo_para_spawn_inimigo) {
    // Reseta o deslocamento do mundo e o dinheiro do jogador
    *deslocamento_x = 0.0f;
    *player_money = 2; // Dinheiro inicial padrão

    // Re-inicializa os estados do protagonista
    inicializar_protagonista_combate(combat_state, dims);
    inicializar_protagonista_movimento_animacao(movement_state, dims);

    // Re-inicializa todos os objetos do jogo (inimigos, obstáculos, dinheiro, etc.)
    inicializar_objetos_jogo(garrafas, granadas, boss_projectiles, inimigos, obstaculos, money_notes, traficante, resources, dims, movement_state, out_proximo_ponto_spawn_obstaculo, out_time_to_spawn_money, out_tempo_para_spawn_inimigo);

    // Re-inicializa o chefe (gato preto e seus estados)
    inicializar_chefe(&((BossEntrance){0}), boss, resources, dims, movement_state); // Passa uma BossEntrance temporária, pois a lógica de spawn a configura

    // Reseta a flag de derrota permanente do chefe, permitindo que ele reapareça em uma nova partida
    boss_defeated_permanently = false;
}


// --- Função Principal do Jogo ---
// Retorna true se o jogo deve ser reiniciado, false se deve sair
bool jogo(ALLEGRO_DISPLAY *janela, ALLEGRO_EVENT_QUEUE *fila, ALLEGRO_FONT *fonte) {
    fprintf(stderr, "DEBUG: Início da função jogo.\n");
    srand(time(NULL)); // Semeia o gerador de números aleatórios para aleatoriedade consistente

    // 1. Inicialização de Recursos do Jogo (Bitmaps, Áudios)
    GameResources resources;
    if (!inicializar_jogo_recursos(&resources)) {
        liberar_jogo_recursos(&resources); // Libera o que foi carregado em caso de erro
        return false; // Retorna falso para sair do jogo
    }

    // 2. Inicialização de Dimensões de Sprites
    SpriteDimensions dims;
    inicializar_dimensoes_sprites(&dims, &resources);

    // 3. Inicialização de Estados do Protagonista
    ProtagonistCombatState protagonist_combat_state;
    inicializar_protagonista_combate(&protagonist_combat_state, &dims);

    ProtagonistMovementState protagonist_movement_state;
    inicializar_protagonista_movimento_animacao(&protagonist_movement_state, &dims);

    // 4. Declaração de Arrays para Objetos do Jogo
    Garrafa garrafas[MAX_GARRAFAS];
    Granada granadas[MAX_GRANADAS];
    BossProjectile boss_projectiles[MAX_BOSS_PROJECTILES];
    Inimigo inimigos[MAX_INIMIGOS];
    Obstaculo obstaculos[MAX_OBSTACULOS];
    MoneyNote money_notes[MAX_MONEY_NOTES];
    NPC traficante;
    BossEntrance boss_entrance; // Declaração da estrutura para a entrada do chefe
    Boss boss; // Declaração da estrutura do chefe

    // Variáveis de estado do jogo
    float deslocamento_x = 0.0; // Deslocamento global do mundo (simula a câmera)
    int player_money = 2; // Dinheiro inicial do jogador

    // Variáveis para controle de spawn
    float proximo_ponto_spawn_obstaculo;
    float time_to_spawn_money;
    float tempo_para_spawn_inimigo;

    // 5. Inicialização de Objetos do Jogo e Chefe
    inicializar_objetos_jogo(garrafas, granadas, boss_projectiles, inimigos, obstaculos, money_notes, &traficante, &resources, &dims, &protagonist_movement_state, &proximo_ponto_spawn_obstaculo, &time_to_spawn_money, &tempo_para_spawn_inimigo);
    inicializar_chefe(&boss_entrance, &boss, &resources, &dims, &protagonist_movement_state);

    // 6. Configuração do Timer e Fila de Eventos
    ALLEGRO_TIMER *timer = al_create_timer(1.0 / 60.0); // Timer para 60 FPS
    al_start_timer(timer);
    al_register_event_source(fila, al_get_display_event_source(janela));
    al_register_event_source(fila, al_get_keyboard_event_source());
    al_register_event_source(fila, al_get_timer_event_source(timer));

    bool jogando = true; // Flag para o loop principal do jogo
    bool should_restart = false; // Flag para indicar se o jogo deve reiniciar após Game Over/Vitória
    float old_deslocamento_x_prot_collision = 0.0; // Salva o deslocamento X do protagonista para detecção de colisão

    // --- Loop Principal do Jogo ---
    while (jogando) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(fila, &ev); // Espera por um evento

        // --- Tratamento de Eventos da Janela e Pausa ---
        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            jogando = false;
            should_restart = false; // Não reinicia se a janela for fechada
        } else if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                // Lógica de Pausa
                al_stop_timer(timer); // Para o timer para pausar o jogo
                // Desenha a tela de pausa
                al_draw_filled_rectangle(0, 0, LARGURA, ALTURA, al_map_rgba(0, 0, 0, 150)); // Fundo escurecido
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 2, (ALTURA / 2) - al_get_font_line_height(fonte), ALLEGRO_ALIGN_CENTER, "PAUSADO");
                al_draw_text(fonte, al_map_rgb(200, 200, 200), LARGURA / 2, (ALTURA / 2) + al_get_font_line_height(fonte), ALLEGRO_ALIGN_CENTER, "Pressione ESC para continuar");
                al_flip_display();

                bool despausar = false;
                while (!despausar) {
                    ALLEGRO_EVENT evento_pausa;
                    al_wait_for_event(fila, &evento_pausa);
                    if (evento_pausa.type == ALLEGRO_EVENT_KEY_DOWN) {
                        if (evento_pausa.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                            despausar = true; // Sai do loop de pausa
                        }
                    } else if (evento_pausa.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
                        jogando = false; // Sai do jogo se a janela for fechada durante a pausa
                        should_restart = false;
                        despausar = true;
                    }
                }
                if (jogando) { // Se o jogo não foi encerrado, reinicia o timer
                    al_start_timer(timer);
                }
            } else {
                // Processa outras entradas de teclado do protagonista (se não estiver pausado)
                handle_protagonist_input(ev, &protagonist_movement_state, &protagonist_combat_state, garrafas, &traficante, &player_money, &boss, &dims, deslocamento_x);
            }
        }
        else if (ev.type == ALLEGRO_EVENT_TIMER) {
            // Salva o deslocamento X do protagonista antes das atualizações para uso em colisões
            old_deslocamento_x_prot_collision = deslocamento_x;

            // --- Atualização da Lógica do Jogo (Chamadas das Funções Modularizadas) ---
            float dt = 1.0 / 60.0; // Delta tempo para simulação baseada em 60 FPS

            atualizar_personagem_movimento_animacao(&protagonist_movement_state, &protagonist_combat_state, garrafas, al_get_bitmap_width(resources.sprite_garrafa), 1.0, &dims, &deslocamento_x, dt);
            atualizar_npc(&traficante, resources.instancia_som_verdebalaraio, &protagonist_movement_state, &dims, deslocamento_x, dt);
            atualizar_projeteis(garrafas, granadas, boss_projectiles, deslocamento_x, dt, al_get_bitmap_width(resources.sprite_garrafa), 1.0, &dims, &protagonist_movement_state);
            gerenciar_inimigos(inimigos, deslocamento_x, dt, &tempo_para_spawn_inimigo, 2.0, 5.0, &protagonist_movement_state, &dims, granadas, &boss);
            gerenciar_dinheiro(money_notes, &resources, deslocamento_x, dt, &time_to_spawn_money, &boss, &dims, &protagonist_movement_state);
            reciclar_objetos_mundo(deslocamento_x, obstaculos, inimigos, money_notes, &traficante, &boss, &dims, &protagonist_movement_state, &resources);
            gerenciar_chefe_e_combate(&boss, &boss_entrance, inimigos, obstaculos, money_notes, boss_projectiles, deslocamento_x, dt, &protagonist_movement_state, &dims);
            processar_colisoes(&deslocamento_x, old_deslocamento_x_prot_collision, &protagonist_movement_state, &protagonist_combat_state, inimigos, garrafas, granadas, obstaculos, money_notes, &player_money, &boss, boss_projectiles, al_get_bitmap_width(resources.sprite_garrafa) * 1.0 * 0.8, al_get_bitmap_height(resources.sprite_garrafa) * 1.0 * 0.8, al_get_bitmap_width(resources.sprite_demon_projetil), al_get_bitmap_height(resources.sprite_demon_projetil), 2.0, &dims);

            // Verificação de fim de jogo
            if (protagonist_combat_state.health <= 0 && protagonist_combat_state.animacao_morte_finalizada) {
                jogando = false;
                should_restart = false; // Será definido na tela de Game Over
            } else if (boss_defeated_permanently) {
                jogando = false;
                should_restart = true; // Será definido na tela de vitória
            }

            // --- SEÇÃO DE DESENHO ---
            al_clear_to_color(al_map_rgb(0, 0, 0)); // Limpa a tela com preto
            desenhar_mundo(&resources, deslocamento_x, obstaculos); // Desenha o background e obstáculos
            desenhar_personagens_projeteis(&resources, deslocamento_x, &protagonist_movement_state, &protagonist_combat_state, &dims, garrafas, granadas, inimigos, money_notes, &traficante, &boss, boss_projectiles, fonte); // Desenha personagens e projéteis
            desenhar_hud(fonte, &protagonist_combat_state, player_money, &traficante, &boss, deslocamento_x); // Desenha a interface do usuário
            al_flip_display(); // Troca os buffers para exibir o que foi desenhado
        }
    }

   /* // --- Limpeza de Recursos (Liberação de Memória) ---
    fprintf(stderr, "DEBUG: Iniciando limpeza de recursos do jogo.\n");
    // Desregistrar fontes de evento específicas do jogo antes de destruir o timer
    al_unregister_event_source(fila, al_get_timer_event_source(timer));
    al_unregister_event_source(fila, al_get_keyboard_event_source()); // Desregistra o teclado para este loop do jogo

    liberar_jogo_recursos(&resources); // Libera todos os bitmaps e áudios
    al_destroy_timer(timer); // Destrói o timer
    fprintf(stderr, "DEBUG: Limpeza de recursos do jogo concluída.\n");*/

    // Após o loop principal, determina qual tela de fim de jogo exibir
    if (protagonist_combat_state.estado == PROT_MORRENDO && protagonist_combat_state.animacao_morte_finalizada) {
        should_restart = game_over_screen(janela, fila, fonte); // Chama a tela de Game Over
    } else if (boss_defeated_permanently) {
        should_restart = victory_screen(janela, fila, fonte); // Chama a tela de vitória
    }

    return should_restart; // Retorna se o jogo deve ser reiniciado
}


int main() {
    // 1. Inicialização do Allegro e Addons Essenciais
    if (!al_init()) {
        fprintf(stderr, "Falha ao inicializar Allegro.\n");
        return -1;
    }
    if (!al_install_keyboard()) {
        fprintf(stderr, "Falha ao inicializar teclado.\n");
        return -1;
    }

    // Inicialização de Addons Gráficos, Texto, Vídeo e Primitivas
    al_init_image_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_video_addon();
    al_init_primitives_addon();

    // Inicialização de Addons de Áudio
    if (!al_install_audio()) {
        fprintf(stderr, "Falha ao inicializar o sistema de áudio.\n");
        return -1;
    }
    if (!al_init_acodec_addon()) {
        fprintf(stderr, "Falha ao inicializar codecs de áudio.\n");
        return -1;
    }
    if (!al_reserve_samples(1)) { // Reserva canais de áudio
        fprintf(stderr, "Falha ao reservar canais de áudio.\n");
        return -1;
    }

    // 2. Criação da Janela (Display)
    ALLEGRO_DISPLAY *janela = al_create_display(LARGURA, ALTURA);
    if (!janela) {
        fprintf(stderr, "Falha ao criar janela.\n");
        return -1;
    }

    // 3. Criação da Fila de Eventos
    ALLEGRO_EVENT_QUEUE *fila = al_create_event_queue();
    if (!fila) {
        fprintf(stderr, "Falha ao criar fila de eventos.\n");
        al_destroy_display(janela);
        return -1;
    }

    // 4. Carregamento da Fonte
    ALLEGRO_FONT *fonte = al_load_ttf_font("arial.ttf", 36, 0); // Carrega a fonte Arial tamanho 36
    if (!fonte) {
        al_show_native_message_box(janela, "Erro", "Fonte", "Não foi possível carregar a fonte arial.ttf. Certifique-se de que está na mesma pasta do executável.", NULL, 0);
        al_destroy_event_queue(fila);
        al_destroy_display(janela);
        return -1;
    }

    // 5. Registro de Fontes de Eventos para o Main (Teclado e Display)
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

    al_draw_scaled_bitmap(load_img, 0, 0, al_get_bitmap_width(load_img), al_get_bitmap_height(load_img), 0, 0, LARGURA, ALTURA, 0);
    const char *msg = "Aperte qualquer tecla para iniciar";
    al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA/2, ALTURA - 100, ALLEGRO_ALIGN_CENTER, msg);
    al_flip_display();

    esperar_tecla(fila);
    al_destroy_bitmap(load_img);

    // === RECURSOS DO MENU ===
    ALLEGRO_VIDEO *video = al_open_video("fundo_menu.ogv");
    if (!video) {
        al_show_native_message_box(janela, "Erro", "Vídeo", "Erro ao carregar vídeo! Certifique-se de que 'fundo_menu.ogv' está na pasta.", NULL, 0);
        al_destroy_font(fonte);
        al_destroy_event_queue(fila);
        al_destroy_display(janela);
        return -1;
    }

    // NOVO: Carrega a imagem do tutorial junto com os outros recursos do menu
    ALLEGRO_BITMAP *tutorial_img = al_load_bitmap("tutorial.png");
    if (!tutorial_img) {
        al_show_native_message_box(janela, "Erro", "Imagem", "Não foi possível carregar tutorial.png", NULL, 0);
        al_close_video(video);
        al_destroy_font(fonte);
        al_destroy_event_queue(fila);
        al_destroy_display(janela);
        return -1;
    }


    al_register_event_source(fila, al_get_video_event_source(video));
    al_start_video(video, al_get_default_mixer());

    // ALTERADO: Opções do menu principal com "Tutorial"
    const char *opcoes[NUM_OPCOES] = {
        "Iniciar jogo",
        "Tutorial",
        "Configuracoes",
        "Sair"
    };
    int opcao_selecionada = 0;

    bool sair_menu = false;
    bool mostrando_tutorial = false; // NOVO: Variável de estado para a tela de tutorial

    while (!sair_menu) {
        ALLEGRO_EVENT ev;
        while (al_get_next_event(fila, &ev)) {
            if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
                sair_menu = true;
                break; // Sai do loop de eventos
            }

            // NOVO: Lógica de eventos separada para a tela de tutorial
            if (mostrando_tutorial) {
                if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
                    if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                        mostrando_tutorial = false; // Volta para o menu
                    }
                }
            }
            // Lógica de eventos para o menu principal
            else {
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
                            // NOVO: Usa um switch para lidar com as opções, fica mais limpo
                            switch(opcao_selecionada) {
                                case 0: // "Iniciar jogo"
                                    al_close_video(video);
                                    bool restart_game = true;
                                    while (restart_game) {
                                        restart_game = jogo(janela, fila, fonte);
                                        if (!restart_game) {
                                            sair_menu = true;
                                            break;
                                        }
                                    }
                                    if (sair_menu) break;
                                    al_register_event_source(fila, al_get_video_event_source(video));
                                    al_start_video(video, al_get_default_mixer());
                                    break;

                                case 1: // "Tutorial"
                                    mostrando_tutorial = true; // Ativa a tela de tutorial
                                    break;

                                case 2: // "Configuracoes"
                                    printf("Opção selecionada: %s\n", opcoes[opcao_selecionada]);
                                    break;

                                case 3: // "Sair"
                                    sair_menu = true;
                                    break;
                            }
                            break; // Fim do case ALLEGRO_KEY_ENTER
                        case ALLEGRO_KEY_ESCAPE:
                            sair_menu = true;
                            break;
                    } // Fim do switch(keycode)
                } // Fim do if (key_down)
            } // Fim do else (não está mostrando tutorial)

            if (ev.type == ALLEGRO_EVENT_VIDEO_FINISHED) {
                al_seek_video(video, 0.0);
                al_start_video(video, al_get_default_mixer());
            }
        } // Fim do while (eventos)

        // --- LÓGICA DE DESENHO ---

        // Desenha o vídeo de fundo (sempre)
        ALLEGRO_BITMAP *frame = al_get_video_frame(video);
        if (frame) {
            desenhar_video_proporcional(frame);
        }

        // NOVO: Lógica de desenho baseada no estado
        if (mostrando_tutorial) {
            // Desenha a imagem do tutorial por cima do vídeo
            al_draw_scaled_bitmap(tutorial_img, 0, 0,
                                 al_get_bitmap_width(tutorial_img),
                                 al_get_bitmap_height(tutorial_img),
                                 0, 0, LARGURA, ALTURA, 0);

            // Adiciona uma dica para o usuário
            al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 2, ALTURA - 60,
                         ALLEGRO_ALIGN_CENTER, "Pressione ESC para voltar ao menu");
        } else {
            // Desenha as opções do menu por cima do vídeo
            int base_y = ALTURA / 2 - (NUM_OPCOES * 40) / 2;
            for (int i = 0; i < NUM_OPCOES; i++) {
                ALLEGRO_COLOR cor = (i == opcao_selecionada) ? al_map_rgb(255, 165, 0) : al_map_rgb(255, 255, 255);
                al_draw_text(fonte, cor, LARGURA / 2, base_y + i * 50, ALLEGRO_ALIGN_CENTER, opcoes[i]);
            }
        }

        al_flip_display();
        al_rest(0.01);
    }
   /* // 7. Limpeza de Recursos do Menu e Allegro
    fprintf(stderr, "DEBUG: Iniciando limpeza de recursos do Allegro no main.\n");
    al_close_video(video);
    al_destroy_bitmap(tutorial_img); // NOVO: Libera a imagem do tutorial
    al_destroy_font(fonte);
    al_destroy_event_queue(fila);
    al_destroy_display(janela);
    al_uninstall_audio();
    al_shutdown_image_addon();
    al_shutdown_font_addon();
    al_shutdown_ttf_addon();
    al_shutdown_video_addon();
    al_shutdown_primitives_addon();
    al_uninstall_keyboard();
    al_uninstall_system();
    fprintf(stderr, "DEBUG: Limpeza de recursos do Allegro no main concluída. Programa encerrado.\n");*/

    return 0;
}