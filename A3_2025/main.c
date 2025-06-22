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

// --- Constantes da Janela ---
#define LARGURA 1536
#define ALTURA 864

#define NUM_OPCOES 3 // Não usado diretamente na função jogo, mas mantido

const int VIDEO_LARGURA = 1920;
const int VIDEO_ALTURA = 1080;

// --- Estrutura para propriedades das garrafas ---
typedef struct {
    float x, y;
    float vel_x;
    float angulo;
    bool ativa;
} Garrafa;

// --- NOVO: Estrutura para a granada ---
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

// --- NOVO: Enumeração dos Tipos de Inimigo ---
typedef enum {
    NOIA,
    POLICIAL
} InimigoType;

// --- Enumeração dos Estados do Protagonista ---
typedef enum {
    PROT_NORMAL,     // Estado normal (parado, andando, pulando, agachando, atacando, arremessando)
    PROT_MORRENDO    // Estado de morte do protagonista
} ProtagonistState;

// Estrutura para propriedades do inimigo (ATUALIZADA)
typedef struct {
    float x, y;    // Posição do inimigo no MUNDO (não na TELA)
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
    float velocidade = velocidade_andar;
    float escala_personagens = 3.0;
    float escala_traficante = 1.7;

    // --- Carregamento de Bitmaps (Sprites) ---
    ALLEGRO_BITMAP *background = al_load_bitmap("background.png");
    ALLEGRO_BITMAP *sprite_parado = al_load_bitmap("personagem.png");
    ALLEGRO_BITMAP *sprite_andando = al_load_bitmap("personagem_andando.png");
    ALLEGRO_BITMAP *sprite_correndo = al_load_bitmap("personagem_correndo.png");
    ALLEGRO_BITMAP *sprite_pulando = al_load_bitmap("personagem_pulando.png");
    ALLEGRO_BITMAP *sprite_agachado = al_load_bitmap("personagem_agachado.png");
    ALLEGRO_BITMAP *sprite_especial = al_load_bitmap("personagem_especial.png");
    ALLEGRO_BITMAP *sprite_ataque1 = al_load_bitmap("personagem_ataque1.png");
    ALLEGRO_BITMAP *sprite_arremesso = al_load_bitmap("personagem_arremessando.png");
    ALLEGRO_BITMAP *sprite_garrafa = al_load_bitmap("garrafa.png");
    ALLEGRO_BITMAP *sprite_personagem_morte = al_load_bitmap("personagem_morte.png");
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

    // --- Carregamento de Áudio ---
    ALLEGRO_SAMPLE *som_verdebalaraio = al_load_sample("verdebalaraio.ogg");
    ALLEGRO_SAMPLE_INSTANCE *instancia_som_verdebalaraio = NULL;

    // --- Verificação de Carregamento de Ativos ---
    if (!background || !sprite_parado || !sprite_andando || !sprite_correndo || !sprite_pulando ||
        !sprite_agachado || !sprite_especial || !sprite_ataque1 || !sprite_arremesso || !sprite_garrafa ||
        !sprite_inimigo_parado || !sprite_inimigo_andando || !sprite_inimigo_morte || !sprite_inimigo_ataque ||
        !sprite_personagem_morte || !sprite_traficante_parada || !som_verdebalaraio || !sprite_sacos_lixo ||
        !sprite_placa_radar || !sprite_2reais || !sprite_5reais || !sprite_10reais ||
        !sprite_policial_parado || !sprite_policial_arremesso || !sprite_explosao || !sprite_policial_morte) { // NOVO: Checagem dos novos sprites
        fprintf(stderr, "ERRO CRÍTICO: Falha ao carregar um ou mais bitmaps/áudios! Verifique nomes/caminhos dos arquivos.\n");
        // Limpeza de todos os bitmaps carregados para evitar vazamento de memória
        al_destroy_bitmap(background); al_destroy_bitmap(sprite_parado); al_destroy_bitmap(sprite_andando);
        al_destroy_bitmap(sprite_correndo); al_destroy_bitmap(sprite_pulando); al_destroy_bitmap(sprite_agachado);
        al_destroy_bitmap(sprite_especial); al_destroy_bitmap(sprite_ataque1); al_destroy_bitmap(sprite_arremesso);
        al_destroy_bitmap(sprite_garrafa); al_destroy_bitmap(sprite_inimigo_parado); al_destroy_bitmap(sprite_inimigo_andando);
        al_destroy_bitmap(sprite_inimigo_morte); al_destroy_bitmap(sprite_inimigo_ataque); al_destroy_bitmap(sprite_personagem_morte);
        al_destroy_bitmap(sprite_traficante_parada); al_destroy_sample(som_verdebalaraio); al_destroy_bitmap(sprite_sacos_lixo);
        al_destroy_bitmap(sprite_placa_radar); al_destroy_bitmap(sprite_2reais); al_destroy_bitmap(sprite_5reais);
        al_destroy_bitmap(sprite_10reais); al_destroy_bitmap(sprite_policial_parado); al_destroy_bitmap(sprite_policial_arremesso);
        al_destroy_bitmap(sprite_explosao); al_destroy_bitmap(sprite_policial_morte);
        return false;
    }
    fprintf(stderr, "DEBUG: Todos os bitmaps e áudios carregados com sucesso.\n");

    // --- Cria e anexa instância de áudio ---
    instancia_som_verdebalaraio = al_create_sample_instance(som_verdebalaraio);
    if (!instancia_som_verdebalaraio) {
        fprintf(stderr, "ERRO CRÍTICO: Falha ao criar instancia_som_verdebalaraio!\n");
        al_destroy_bitmap(background); al_destroy_bitmap(sprite_parado); al_destroy_bitmap(sprite_andando);
        al_destroy_bitmap(sprite_correndo); al_destroy_bitmap(sprite_pulando); al_destroy_bitmap(sprite_agachado);
        al_destroy_bitmap(sprite_especial); al_destroy_bitmap(sprite_ataque1); al_destroy_bitmap(sprite_arremesso);
        al_destroy_bitmap(sprite_garrafa); al_destroy_bitmap(sprite_inimigo_parado); al_destroy_bitmap(sprite_inimigo_andando);
        al_destroy_bitmap(sprite_inimigo_morte); al_destroy_bitmap(sprite_inimigo_ataque);
        al_destroy_bitmap(sprite_personagem_morte); al_destroy_bitmap(sprite_traficante_parada);
        al_destroy_sample(som_verdebalaraio);
        al_destroy_bitmap(sprite_sacos_lixo);
        al_destroy_bitmap(sprite_placa_radar);
        al_destroy_bitmap(sprite_2reais); al_destroy_bitmap(sprite_5reais); al_destroy_bitmap(sprite_10reais);
        al_destroy_bitmap(sprite_policial_parado); al_destroy_bitmap(sprite_policial_arremesso); al_destroy_bitmap(sprite_explosao);
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
    int frame_total_parado = 6, frame_total_andando = 8, frame_total_correndo = 8, frame_total_pulando = 16, frame_total_agachado = 8;
    int frame_total_especial = 13;
    int frame_total_ataque1 = 5;
    int frame_total_arremesso = 4;
    int frame_total_personagem_morte = 4;
    // Frames do Noia
    int frame_total_inimigo_parado = 7;
    int frame_total_inimigo_andando = 8;
    int frame_total_inimigo_morte = 4;
    int frame_total_inimigo_ataque = 4;
    // NOVO: Frames do Policial e Explosão
    int frame_total_policial_parado = 7;
    int frame_total_policial_arremesso = 9;
    int frame_total_explosao = 9;
    int frame_total_policial_morte = 4;

    int frame_altura = 128; // Altura padrão

    // Larguras de frame do personagem
    int frame_largura_parado = 768 / frame_total_parado;
    int frame_largura_andando = 1024 / frame_total_andando;
    int frame_largura_correndo = 1024 / frame_total_correndo;
    int frame_largura_pulando = 2048 / frame_total_pulando;
    int frame_largura_agachado = 1024 / frame_total_agachado;
    int frame_largura_especial = 1664 / frame_total_especial;
    int frame_largura_ataque1 = 640 / frame_total_ataque1;
    int frame_largura_arremesso = 512 / frame_total_arremesso;
    int frame_largura_personagem_morte = al_get_bitmap_width(sprite_personagem_morte) / frame_total_personagem_morte;

    // Larguras de frame do Noia
    int frame_largura_inimigo_parado = al_get_bitmap_width(sprite_inimigo_parado) / frame_total_inimigo_parado;
    int frame_largura_inimigo_andando = al_get_bitmap_width(sprite_inimigo_andando) / frame_total_inimigo_andando;
    int frame_largura_inimigo_morte = al_get_bitmap_width(sprite_inimigo_morte) / frame_total_inimigo_morte;
    int frame_largura_inimigo_ataque = al_get_bitmap_width(sprite_inimigo_ataque) / frame_total_inimigo_ataque;

    // NOVO: Larguras de frame do Policial e Explosão
    int frame_largura_policial_parado = al_get_bitmap_width(sprite_policial_parado) / frame_total_policial_parado;
    int frame_largura_policial_arremesso = al_get_bitmap_width(sprite_policial_arremesso) / frame_total_policial_arremesso;
    int frame_largura_policial_morte = al_get_bitmap_width(sprite_policial_morte) / frame_total_policial_morte;
    int frame_largura_explosao = al_get_bitmap_width(sprite_explosao) / frame_total_explosao;
    int frame_altura_explosao = al_get_bitmap_height(sprite_explosao);

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
    int inimigo_altura_sprite = frame_altura;


    // --- Propriedades da Hitbox do Protagonista ---
    float prot_hitbox_offset_x = 40.0 * escala_personagens;
    float prot_hitbox_offset_y = 5.0 * escala_personagens;
    float prot_hitbox_width = (frame_largura_parado - 80) * escala_personagens;
    float prot_hitbox_height = (frame_altura - 10) * escala_personagens;
    float prot_crouch_hitbox_height = (frame_altura - 50) * escala_personagens;

    // --- Propriedades da Hitbox de ATAQUE do Protagonista ---
    float prot_attack_hitbox_offset_x = 60.0 * escala_personagens;
    float prot_attack_hitbox_offset_y = 50.0 * escala_personagens;
    float prot_attack_hitbox_width = 50 * escala_personagens;
    float prot_attack_hitbox_height = 80.0 * escala_personagens;

    // --- Propriedades da Garrafa ---
    int garrafa_largura_original = al_get_bitmap_width(sprite_garrafa);
    int garrafa_altura_original = al_get_bitmap_height(sprite_garrafa);
    float escala_garrafa = 1.0;
    float garrafa_hitbox_scale = 0.8;
    float garrafa_hitbox_width = garrafa_largura_original * escala_garrafa * garrafa_hitbox_scale;
    float garrafa_hitbox_height = garrafa_altura_original * escala_garrafa * garrafa_hitbox_scale;

    // --- Propriedades de Obstáculos ---
    float escala_obstaculo_sacos = 0.1;
    float escala_placa_radar = 0.3;

    float personagem_x = LARGURA / 2.0 - (frame_largura_correndo * escala_personagens) / 2.0;
    float personagem_y_base = (ALTURA - 300) - (frame_altura * escala_personagens) / 2.0;
    float personagem_y = personagem_y_base;

    // --- Variáveis de Animação do Personagem ---
    int frame_parado = 0, frame_andando = 0, frame_correndo = 0, frame_pulando = 0, frame_agachado = 0, frame_especial = 0, frame_ataque1 = 0;
    int frame_arremesso = 0;
    int personagem_frame_morte = 0;
    float personagem_acc_morte = 0;
    float tpf_personagem_morte = 1.0 / 8.0;
    bool personagem_morte_animacao_finalizada = false;

    // --- Tempos Por Frame (TPF) para Animações ---
    float tpf_parado = 1.0 / 5.0, tpf_andando = 1.0 / 10.0, tpf_correndo = 1.0 / 10.0, tpf_pulando = 1.0 / 12, tpf_agachado = 1.0 / 8.0;
    float tpf_especial = 1.0 / 15.0;
    float tpf_ataque1 = 1.0 / 10.0;
    float tpf_arremesso = 1.0 / 8.0;
    // TPF Inimigos
    float tpf_inimigo_parado = 1.0 / 8.0;
    float tpf_inimigo_andando = 1.0 / 10.0;
    float tpf_inimigo_ataque = 1.0 / 8.0;
    float tpf_inimigo_morte = 1.0 / 8.0;
    // NOVO: TPF Policial
    float tpf_policial_parado = 1.0 / 8.0;
    float tpf_policial_arremesso = 1.0 / 12.0;
    float tpf_explosao = 1.0 / 15.0;
    float tpf_policial_morte = 1.0 / 8.0; // Velocidade da animação de morte

    // --- Acumuladores de Tempo para Animações ---
    float acc_parado = 0, acc_andando = 0, acc_correndo = 0, acc_pulando = 0, acc_agachado = 0, acc_especial = 0, acc_ataque1 = 0;
    float acc_arremesso = 0;

    // --- Variáveis de Estado do Protagonista ---
    ProtagonistState protagonista_estado = PROT_NORMAL;
    int protagonist_health = MAX_PROTAGONIST_HEALTH;
    bool protagonist_invulnerable = false;
    float protagonist_invulnerability_timer = 0.0;
    float protagonist_blink_timer = 0.0;
    bool protagonist_visible = true;

    int player_money = 2;

    // --- Inicialização do NPC Traficante ---
    NPC traficante;
    traficante.largura_sprite = al_get_bitmap_width(sprite_traficante_parada) / NPC_TRAFICANTE_FRAME_COUNT;
    traficante.altura_sprite = al_get_bitmap_height(sprite_traficante_parada);
    traficante.x = 4000;
    traficante.y = personagem_y_base - (traficante.altura_sprite * escala_traficante - frame_altura * escala_personagens);
    traficante.ativa = true;
    traficante.frame_atual = 0;
    traficante.acc_animacao = 0;

    // --- Variáveis de Movimento do Protagonista ---
    bool pulando = false, agachando = false, especial_ativo = false, especial_finalizado = false, atacando = false, arremessando = false;
    bool crouch_animation_finished = false;
    float vel_y = 0.0, gravidade = 0.5, vel_x_pulo = 0.0;
    float current_ground_y = personagem_y_base;

    // --- Inicialização de Garrafas e Granadas ---
    Garrafa garrafas[MAX_GARRAFAS];
    Granada granadas[MAX_GRANADAS]; // NOVO
    float garrafa_velocidade_angular = 0.2;
    for (int i = 0; i < MAX_GARRAFAS; i++) {
        garrafas[i].ativa = false;
    }
    for (int i = 0; i < MAX_GRANADAS; i++) { // NOVO
        granadas[i].ativa = false;
    }

    // --- Inicialização de Inimigos ---
    Inimigo inimigos[MAX_INIMIGOS];
    float tempo_para_spawn_inimigo = 2.0;
    float min_intervalo_spawn_inimigo = 2.0;
    float max_intervalo_spawn_inimigo = 5.0;

    float inimigo_velocidade_parado_base = -0.5;
    float inimigo_velocidade_andando_base = -2.5;
    float inimigo_velocidade_ataque_base = 0.0;
    // Distâncias de IA
    float noia_distancia_deteccao = 500.0;
    float noia_distancia_ataque = 180.0;
    float policial_distancia_deteccao = 800.0; // Policial detecta de mais longe
    float policial_distancia_ataque = 700.0;   // E ataca de mais longe

    for (int i = 0; i < MAX_INIMIGOS; i++) {
        inimigos[i].ativa = false;
        // O resto da inicialização acontece no spawn
    }

    // --- Inicialização de Obstáculos ---
    Obstaculo obstaculos[MAX_OBSTACULOS];
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
        obstaculos[i].y = personagem_y_base + (frame_altura * escala_personagens) - obstaculos[i].height;

        proximo_ponto_spawn_obstaculo = obstaculos[i].x + obstaculos[i].width + min_distancia_entre_obstaculos;
    }

    // --- Inicialização de Notas de Dinheiro ---
    MoneyNote money_notes[MAX_MONEY_NOTES];
    ALLEGRO_BITMAP *money_sprites[] = {sprite_2reais, sprite_5reais, sprite_10reais};
    int money_values[] = {2, 5, 10};
    int num_money_types = sizeof(money_sprites) / sizeof(money_sprites[0]);
    float time_to_spawn_money = MONEY_NOTE_SPAWN_INTERVAL;

    for (int i = 0; i < MAX_MONEY_NOTES; i++) {
        money_notes[i].ativa = false;
    }


    // --- Configuração do Timer do Jogo ---
    ALLEGRO_TIMER *timer = al_create_timer(1.0 / 60.0);
    al_start_timer(timer);

    // --- Registro de Fontes de Eventos ---
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
                if (protagonista_estado == PROT_NORMAL) {
                    if (ev.keyboard.keycode == ALLEGRO_KEY_UP && !pulando && !agachando && !atacando && !arremessando) {
                        pulando = true; vel_y = -10.0; frame_pulando = 0; acc_pulando = 0; especial_ativo = false; especial_finalizado = false;
                        ALLEGRO_KEYBOARD_STATE estado; al_get_keyboard_state(&estado);
                        if (al_key_down(&estado, ALLEGRO_KEY_RIGHT)) vel_x_pulo = velocidade;
                        else if (al_key_down(&estado, ALLEGRO_KEY_LEFT)) vel_x_pulo = -velocidade;
                        else vel_x_pulo = 0;
                        current_ground_y = personagem_y_base;
                    }
                    else if (ev.keyboard.keycode == ALLEGRO_KEY_DOWN && !pulando && !atacando && !arremessando) {
                        agachando = true; frame_agachado = 0; acc_agachado = 0; especial_ativo = false; especial_finalizado = false;
                        crouch_animation_finished = false;
                    }
                    else if (ev.keyboard.keycode == ALLEGRO_KEY_Z && !pulando && !agachando && !especial_ativo && !atacando && !arremessando) { especial_ativo = true; especial_finalizado = false; frame_especial = 0; acc_especial = 0; }
                    else if (ev.keyboard.keycode == ALLEGRO_KEY_SPACE && especial_ativo && especial_finalizado && !atacando && !arremessando) { atacando = true; frame_ataque1 = 0; acc_ataque1 = 0; }
                    else if (ev.keyboard.keycode == ALLEGRO_KEY_Q && !agachando && !especial_ativo && !atacando && !arremessando) { arremessando = true; frame_arremesso = 0; acc_arremesso = 0; }
                    else if (ev.keyboard.keycode == ALLEGRO_KEY_B) {
                        float prot_world_x_center = personagem_x + (frame_largura_parado * escala_personagens) / 2.0 + deslocamento_x;
                        float npc_world_x_center = traficante.x + (traficante.largura_sprite * escala_traficante) / 2.0;
                        float distance = fabs(prot_world_x_center - npc_world_x_center);

                        if (traficante.ativa && distance < NPC_INTERACTION_DISTANCE) {
                            if (player_money >= NPC_HEAL_COST) {
                                if (protagonist_health >= MAX_PROTAGONIST_HEALTH + NPC_HEAL_AMOUNT) {
                                    fprintf(stderr, "DEBUG: Vida já está no máximo (120 HP)! Não é possível comprar mais pó.\n");
                                } else {
                                    player_money -= NPC_HEAL_COST;
                                    protagonist_health += NPC_HEAL_AMOUNT;
                                    if (protagonist_health > MAX_PROTAGONIST_HEALTH + NPC_HEAL_AMOUNT) {
                                        protagonist_health = MAX_PROTAGONIST_HEALTH + NPC_HEAL_AMOUNT;
                                    }
                                    fprintf(stderr, "DEBUG: Vida recuperada! HP: %d, Dinheiro: R$%d\n", protagonist_health, player_money);
                                }
                            } else {
                                fprintf(stderr, "DEBUG: Sem dinheiro suficiente para comprar! Dinheiro atual: R$%d, Custo: R$%d\n", player_money, NPC_HEAL_COST);
                            }
                        }
                    }
                }
            }
        }
        else if (ev.type == ALLEGRO_EVENT_KEY_UP) {
            if (protagonista_estado == PROT_NORMAL) {
                if (ev.keyboard.keycode == ALLEGRO_KEY_DOWN) {
                    // Handled in timer loop when animation is finished and key is up
                }
            }
        }

        else if (ev.type == ALLEGRO_EVENT_TIMER) {
            float delta_deslocamento_x = deslocamento_x - deslocamento_x_anterior;
            deslocamento_x_anterior = deslocamento_x;

            ALLEGRO_KEYBOARD_STATE estado;
            al_get_keyboard_state(&estado);
            bool andando = false, correndo = false;

            float old_deslocamento_x_prot = deslocamento_x;
            float old_personagem_x = personagem_x;
            float old_personagem_y = personagem_y;

            if (protagonista_estado == PROT_NORMAL) {
                if (protagonist_invulnerable) {
                    protagonist_invulnerability_timer -= 1.0 / 60.0;
                    protagonist_blink_timer += 1.0 / 60.0;
                    if (protagonist_blink_timer >= PROTAGONIST_BLINK_INTERVAL) {
                        protagonist_blink_timer = 0.0;
                        protagonist_visible = !protagonist_visible;
                    }
                    if (protagonist_invulnerability_timer <= 0) {
                        protagonist_invulnerable = false;
                        protagonist_visible = true;
                    }
                }

                if (!atacando && (!especial_ativo || especial_finalizado) && !arremessando) {
                    velocidade = (al_key_down(&estado, ALLEGRO_KEY_LSHIFT) || al_key_down(&estado, ALLEGRO_KEY_RSHIFT)) ? velocidade_correr : velocidade_andar;
                    if (!pulando) {
                        if (al_key_down(&estado, ALLEGRO_KEY_RIGHT)) {
                            deslocamento_x += velocidade;
                            andando = true;
                            if (velocidade == velocidade_correr) correndo = true;
                        }
                        if (al_key_down(&estado, ALLEGRO_KEY_LEFT)) {
                            deslocamento_x -= velocidade;
                            andando = true;
                            if (velocidade == velocidade_correr) correndo = true;
                        }
                    }
                } else if (pulando) {
                    velocidade = (al_key_down(&estado, ALLEGRO_KEY_LSHIFT) || al_key_down(&estado, ALLEGRO_KEY_RSHIFT)) ? velocidade_correr : velocidade_andar;
                }

                if (pulando) {
                    personagem_y += vel_y;
                    vel_y += gravidade;
                    deslocamento_x += vel_x_pulo;
                    if (personagem_y >= personagem_y_base && vel_y >= 0) {
                        personagem_y = personagem_y_base;
                        pulando = false;
                        vel_y = 0;
                        vel_x_pulo = 0;
                        current_ground_y = personagem_y_base;
                    }
                    acc_pulando += 1.0 / 60.0; if (acc_pulando >= tpf_pulando) { acc_pulando -= tpf_pulando; if (frame_pulando < frame_total_pulando - 1) frame_pulando++; }
                } else if (agachando) {
                    acc_agachado += 1.0 / 60.0;
                    if (acc_agachado >= tpf_agachado) {
                        acc_agachado -= tpf_agachado;
                        if (frame_agachado < frame_total_agachado - 1) {
                            frame_agachado++;
                        } else {
                            crouch_animation_finished = true;
                            frame_agachado = frame_total_agachado - 1;
                        }
                    }
                    if (!al_key_down(&estado, ALLEGRO_KEY_DOWN) && crouch_animation_finished) {
                        agachando = false;
                        crouch_animation_finished = false;
                        frame_agachado = 0;
                        acc_agachado = 0;
                    }
                    frame_parado = 0; acc_parado = 0; frame_andando = 0; acc_andando = 0; frame_correndo = 0; acc_correndo = 0; frame_especial = 0; acc_especial = 0; atacando = false; acc_ataque1 = 0; frame_ataque1 = 0; arremessando = false; acc_arremesso = 0; frame_arremesso = 0;
                } else if (atacando) {
                    acc_ataque1 += 1.0 / 60.0; if (acc_ataque1 >= tpf_ataque1) { acc_ataque1 -= tpf_ataque1; if (frame_ataque1 < frame_total_ataque1 - 1) { frame_ataque1++; } else { atacando = false; frame_ataque1 = 0; } }
                    frame_parado = 0; acc_parado = 0; frame_andando = 0; acc_andando = 0; frame_correndo = 0; acc_correndo = 0; frame_pulando = 0; acc_pulando = 0; frame_agachado = 0; acc_agachado = 0; arremessando = false; acc_arremesso = 0; frame_arremesso = 0;
                } else if (especial_ativo) {
                    acc_especial += 1.0 / 60.0; if (acc_especial >= tpf_especial) { acc_especial -= tpf_especial; if (frame_especial < frame_total_especial - 1) { frame_especial++; } else { especial_finalizado = true; } }
                    frame_parado = 0; acc_parado = 0; frame_andando = 0; acc_andando = 0; frame_correndo = 0; acc_correndo = 0; frame_pulando = 0; acc_pulando = 0; frame_agachado = 0; acc_agachado = 0; atacando = false; acc_ataque1 = 0; frame_ataque1 = 0; arremessando = false; acc_arremesso = 0; frame_arremesso = 0;
                } else if (arremessando) {
                    acc_arremesso += 1.0 / 60.0; if (acc_arremesso >= tpf_arremesso) { acc_arremesso -= tpf_arremesso; if (frame_arremesso < frame_total_arremesso - 1) { frame_arremesso++; } else { arremessando = false; for (int i = 0; i < MAX_GARRAFAS; i++) { if (!garrafas[i].ativa) { garrafas[i].x = personagem_x + (frame_largura_arremesso * escala_personagens) / 2.0 + deslocamento_x; garrafas[i].y = personagem_y + (frame_altura * escala_personagens) / 2.0; garrafas[i].vel_x = 15.0; garrafas[i].ativa = true; garrafas[i].angulo = 0.0; break; } } frame_arremesso = 0; } }
                    frame_parado = 0; acc_parado = 0; frame_andando = 0; acc_andando = 0; frame_agachado = 0; acc_agachado = 0; atacando = false; acc_ataque1 = 0; frame_ataque1 = 0;
                } else {
                    if (personagem_y < current_ground_y) {
                        personagem_y += gravidade * 8;
                        if (personagem_y > current_ground_y) {
                            personagem_y = current_ground_y;
                            vel_y = 0;
                        }
                    } else if (personagem_y > current_ground_y) {
                        personagem_y = current_ground_y;
                        vel_y = 0;
                    }

                    if (andando) {
                        if (correndo) { acc_correndo += 1.0 / 60.0; if (acc_correndo >= tpf_correndo) { acc_correndo -= tpf_correndo; frame_correndo = (frame_correndo + 1) % frame_total_correndo; } frame_andando = 0; acc_andando = 0; frame_parado = 0; acc_parado = 0; }
                        else { acc_andando += 1.0 / 60.0; if (acc_andando >= tpf_andando) { acc_andando -= tpf_andando; frame_andando = (frame_andando + 1) % frame_total_andando; } frame_correndo = 0; acc_correndo = 0; frame_parado = 0; acc_parado = 0; }
                    } else { acc_parado += 1.0 / 60.0; if (acc_parado >= tpf_parado) { acc_parado -= tpf_parado; frame_parado = (frame_parado + 1) % frame_total_parado; } frame_correndo = 0; acc_correndo = 0; frame_andando = 0; acc_andando = 0; }
                }
            } else if (protagonista_estado == PROT_MORRENDO) {
                personagem_acc_morte += 1.0 / 60.0;
                if (personagem_acc_morte >= tpf_personagem_morte) {
                    personagem_acc_morte -= tpf_personagem_morte;
                    if (personagem_frame_morte < frame_total_personagem_morte - 1) {
                        personagem_frame_morte++;
                    } else {
                        personagem_morte_animacao_finalizada = true;
                        jogando = false;
                        should_restart = false;
                        fprintf(stderr, "DEBUG: Animação de morte do protagonista finalizada. Indo para tela de Game Over.\n");
                    }
                }
            }

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
                float prot_world_x_center = personagem_x + (frame_largura_parado * escala_personagens) / 2.0 + deslocamento_x;
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

            // --- NOVO: Atualização das Granadas ---
            for (int i = 0; i < MAX_GRANADAS; i++) {
                if (granadas[i].ativa) {
                    if (!granadas[i].explodindo) {
                        granadas[i].x += granadas[i].vel_x;
                        granadas[i].y += granadas[i].vel_y;
                        granadas[i].vel_y += gravidade * 0.5; // Granada mais leve que o personagem

                        // Checa se atingiu o chão
                        if (granadas[i].y >= personagem_y_base + (frame_altura * escala_personagens) - (frame_altura_explosao * escala_personagens / 2.0)) {
                            granadas[i].y = personagem_y_base + (frame_altura * escala_personagens) - (frame_altura_explosao * escala_personagens / 2.0);
                            granadas[i].explodindo = true;
                            granadas[i].frame_explosao = 0;
                            granadas[i].acc_animacao = 0;
                            granadas[i].dano_aplicado = false; // Permite aplicar dano
                        }
                    } else { // Está explodindo
                        granadas[i].acc_animacao += 1.0 / 60.0;
                        if (granadas[i].acc_animacao >= tpf_explosao) {
                            granadas[i].acc_animacao -= tpf_explosao;
                            if (granadas[i].frame_explosao < frame_total_explosao - 1) {
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
            if (tempo_para_spawn_inimigo <= 0) {
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
                            inimigos[i].hitbox_height = (frame_altura - 20) * escala_personagens;
                        } else { // POLICIAL
                            inimigos[i].vel_x = inimigo_velocidade_parado_base;
                            inimigos[i].hitbox_offset_x = 57.0 * escala_personagens;
                            inimigos[i].hitbox_offset_y = 20 * escala_personagens;
                            inimigos[i].hitbox_width = (frame_largura_policial_parado - 100) * escala_personagens;
                            inimigos[i].hitbox_height = (frame_altura - 20) * escala_personagens;
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
            if (time_to_spawn_money <= 0) {
                for (int i = 0; i < MAX_MONEY_NOTES; i++) {
                    if (!money_notes[i].ativa) {
                        int type_index = rand() % num_money_types;
                        money_notes[i].sprite_bitmap = money_sprites[type_index];
                        money_notes[i].value = money_values[type_index];
                        money_notes[i].hitbox_width = al_get_bitmap_width(money_notes[i].sprite_bitmap) * MONEY_NOTE_SCALE;
                        money_notes[i].hitbox_height = al_get_bitmap_height(money_notes[i].sprite_bitmap) * MONEY_NOTE_SCALE;

                        money_notes[i].x = deslocamento_x + LARGURA + (float)(rand() % 400 + 100);
                        money_notes[i].y = personagem_y_base + (frame_altura * escala_personagens) - money_notes[i].hitbox_height;
                        money_notes[i].ativa = true;
                        money_notes[i].spawn_time = al_get_time();
                        fprintf(stderr, "DEBUG: Nota de R$%d SPAWNADA em (%.2f, %.2f).\n", money_notes[i].value, money_notes[i].x, money_notes[i].y);
                        break;
                    }
                }
            }

            for (int i = 0; i < MAX_MONEY_NOTES; i++) {
                if (money_notes[i].ativa) {
                    if (money_notes[i].x - deslocamento_x < -money_notes[i].hitbox_width ||
                        (al_get_time() - money_notes[i].spawn_time > MONEY_NOTE_LIFETIME)) {
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
                if (obstaculos[i].ativa && (obstaculos[i].x + obstaculos[i].width) < deslocamento_x - LARGURA / 2) {
                    fprintf(stderr, "DEBUG: Reciclando obstaculo %d.\n", i);
                    int type_index = rand() % num_obstacle_types;
                    obstaculos[i].sprite_bitmap = obstacle_types[type_index].sprite;
                    obstaculos[i].only_crouch_pass = obstacle_types[type_index].crouch_pass;
                    float current_scale = obstacle_types[type_index].scale;
                    obstaculos[i].width = al_get_bitmap_width(obstaculos[i].sprite_bitmap) * current_scale;
                    obstaculos[i].height = al_get_bitmap_height(obstaculos[i].sprite_bitmap) * current_scale;
                    obstaculos[i].x = max_x_existente + min_distancia_entre_obstaculos + (float)rand() / RAND_MAX * variacao_distancia_obstaculos;
                    obstaculos[i].y = personagem_y_base + (frame_altura * escala_personagens) - obstaculos[i].height;
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

            // --- Atualiza Estados, Movimento e Verifica Colisões dos Inimigos (ATUALIZADA) ---
            float prot_hb_x = personagem_x + prot_hitbox_offset_x;
            float prot_hb_y = personagem_y + prot_hitbox_offset_y;
            float prot_hb_w = prot_hitbox_width;
            float current_prot_hitbox_height = prot_hitbox_height;
            if (agachando) {
                current_prot_hitbox_height = prot_crouch_hitbox_height;
                prot_hb_y = personagem_y + (prot_hitbox_height - prot_crouch_hitbox_height) + prot_hitbox_offset_y;
            } else {
                prot_hb_y = personagem_y + prot_hitbox_offset_y;
            }
            
            float prot_attack_hb_x = personagem_x + prot_attack_hitbox_offset_x;
            float prot_attack_hb_y = personagem_y + prot_attack_hitbox_offset_y;

            for (int i = 0; i < MAX_INIMIGOS; i++) {
                if (inimigos[i].ativa) {
                    // Atualiza cooldown de ataque
                    if (inimigos[i].attack_cooldown > 0) {
                        inimigos[i].attack_cooldown -= 1.0 / 60.0;
                    }

                    float inimigo_x_antes_movimento = inimigos[i].x;
                    inimigos[i].x += inimigos[i].vel_x;

                    float inimigo_screen_x = inimigos[i].x - deslocamento_x;
                    float inimigo_screen_hb_x = inimigo_screen_x + inimigos[i].hitbox_offset_x;
                    float inimigo_hb_y = inimigos[i].y + inimigos[i].hitbox_offset_y;
                    float inimigo_hb_w = inimigos[i].hitbox_width;
                    float inimigo_hb_h = inimigos[i].hitbox_height;

                    // --- Colisão: Ataque do Protagonista vs Inimigo ---
                    if (atacando && inimigos[i].estado != INIMIGO_MORRENDO) {
                        if (check_collision(prot_attack_hb_x, prot_attack_hb_y, prot_attack_hitbox_width, prot_attack_hitbox_height,
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
                    if (inimigos[i].estado != INIMIGO_MORRENDO && protagonista_estado == PROT_NORMAL) {
                        if (check_collision(inimigo_screen_hb_x, inimigo_hb_y, inimigo_hb_w, inimigo_hb_h,
                                            prot_hb_x, prot_hb_y, prot_hb_w, current_prot_hitbox_height)) {

                            deslocamento_x = old_deslocamento_x_prot;
                            inimigos[i].x = inimigo_x_antes_movimento;
                            delta_deslocamento_x = deslocamento_x - deslocamento_x_anterior;
                            deslocamento_x_anterior = deslocamento_x;

                            if (inimigos[i].inimigo_pode_dar_dano && !protagonist_invulnerable) {
                                if (inimigos[i].type == NOIA && inimigos[i].estado == INIMIGO_ATACANDO) {
                                    protagonist_health -= PROTAGONIST_DAMAGE_PER_HIT;
                                    inimigos[i].inimigo_pode_dar_dano = false;
                                    fprintf(stderr, "PROTAGONISTA SOFREU DANO DO NOIA! Vida restante: %d\n", protagonist_health);

                                    protagonist_invulnerable = true;
                                    protagonist_invulnerability_timer = PROTAGONIST_INVULNERABILITY_DURATION;
                                    protagonist_blink_timer = 0.0;
                                    protagonist_visible = false;
                                }
                                // Policial causa dano via granada, não contato direto
                            }

                            if (protagonist_health <= 0) {
                                protagonista_estado = PROT_MORRENDO;
                                personagem_frame_morte = 0;
                                personagem_acc_morte = 0;
                                personagem_morte_animacao_finalizada = false;
                                fprintf(stderr, "PROTAGONISTA MORREU! Iniciando animação de morte do protagonista.\n");
                            }
                        }
                    }

                    float inimigo_world_hb_x = inimigos[i].x + inimigos[i].hitbox_offset_x;
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
                        float prot_world_x_center_hb = (personagem_x + prot_hitbox_offset_x + prot_hb_w / 2.0) + deslocamento_x;
                        float inimigo_world_x_center_hb = inimigos[i].x + inimigos[i].hitbox_offset_x + inimigos[i].hitbox_width / 2.0;
                        float dist_to_protagonist_center_x = fabs(inimigo_world_x_center_hb - prot_world_x_center_hb);

                        float distancia_ataque_atual = (inimigos[i].type == NOIA) ? noia_distancia_ataque : policial_distancia_ataque;
                        float distancia_deteccao_atual = (inimigos[i].type == NOIA) ? noia_distancia_deteccao : policial_distancia_deteccao;

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
                    float tpf_current_inimigo = 0;
                    int frame_total_current_inimigo = 0;

                    if (inimigos[i].type == NOIA) {
                        switch(inimigos[i].estado) {
                            case INIMIGO_PARADO: tpf_current_inimigo = tpf_inimigo_parado; frame_total_current_inimigo = frame_total_inimigo_parado; break;
                            case INIMIGO_ANDANDO: tpf_current_inimigo = tpf_inimigo_andando; frame_total_current_inimigo = frame_total_inimigo_andando; break;
                            case INIMIGO_ATACANDO: tpf_current_inimigo = tpf_inimigo_ataque; frame_total_current_inimigo = frame_total_inimigo_ataque; break;
                            case INIMIGO_MORRENDO: tpf_current_inimigo = tpf_inimigo_morte; frame_total_current_inimigo = frame_total_inimigo_morte; break;
                        }
                    } else { // POLICIAL
                        switch(inimigos[i].estado) {
                            case INIMIGO_PARADO:
                            case INIMIGO_ANDANDO: // Usa a mesma animação parada para andar
                                tpf_current_inimigo = tpf_policial_parado;
                                frame_total_current_inimigo = frame_total_policial_parado;
                                break;
                            case INIMIGO_ATACANDO:
                                tpf_current_inimigo = tpf_policial_arremesso;
                                frame_total_current_inimigo = frame_total_policial_arremesso;
                                break;
                            case INIMIGO_MORRENDO: // Reusa a morte do noia por enquanto
                                tpf_current_inimigo = tpf_policial_morte; // Usa o tempo por frame do policial
                                frame_total_current_inimigo = frame_total_policial_morte; // Usa o total de frames do policial
                                break;
                        }
                    }
                    inimigos[i].acc_animacao += 1.0 / 60.0;
                    if (inimigos[i].acc_animacao >= tpf_current_inimigo) {
                        inimigos[i].acc_animacao -= tpf_current_inimigo;
                        if (inimigos[i].frame_atual < frame_total_current_inimigo - 1) {
                            inimigos[i].frame_atual++;
                            // NOVO: Lançar granada em um frame específico
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

                    // Detecção de colisão (Garrafa vs Inimigo)
                    if (inimigos[i].estado != INIMIGO_MORRENDO) {
                        for (int j = 0; j < MAX_GARRAFAS; j++) {
                            if (garrafas[j].ativa) {
                                float garrafa_hb_x = garrafas[j].x - garrafa_hitbox_width / 2.0;
                                float garrafa_hb_y = garrafas[j].y - garrafa_hitbox_height / 2.0;

                                if (check_collision(garrafa_hb_x, garrafa_hb_y, garrafa_hitbox_width, garrafa_hitbox_height,
                                                    inimigos[i].x + inimigos[i].hitbox_offset_x, inimigo_hb_y, inimigo_hb_w, inimigo_hb_h)) {
                                    fprintf(stderr, "COLISÃO! Garrafa %d atingiu Inimigo %d! Iniciando animação de morte.\n", j, i);
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
            }

            // NOVO: Colisão Explosão da Granada vs Protagonista
            for (int i = 0; i < MAX_GRANADAS; i++) {
                if (granadas[i].ativa && granadas[i].explodindo && !granadas[i].dano_aplicado && protagonista_estado == PROT_NORMAL && !protagonist_invulnerable) {
                     float explosao_hb_w = frame_largura_explosao * escala_personagens;
                     float explosao_hb_h = frame_altura_explosao * escala_personagens;
                     float explosao_hb_x = (granadas[i].x - deslocamento_x) - explosao_hb_w / 2.0;
                     float explosao_hb_y = granadas[i].y - explosao_hb_h / 2.0;
                    
                     if (check_collision(prot_hb_x, prot_hb_y, prot_hb_w, current_prot_hitbox_height,
                                          explosao_hb_x, explosao_hb_y, explosao_hb_w, explosao_hb_h)) {
                         protagonist_health -= GRENADE_DAMAGE;
                         granadas[i].dano_aplicado = true;
                         fprintf(stderr, "PROTAGONISTA ATINGIDO PELA GRANADA! Vida: %d\n", protagonist_health);

                         protagonist_invulnerable = true;
                         protagonist_invulnerability_timer = PROTAGONIST_INVULNERABILITY_DURATION;
                         protagonist_blink_timer = 0.0;
                         protagonist_visible = false;

                         if (protagonist_health <= 0) {
                             protagonista_estado = PROT_MORRENDO;
                             personagem_frame_morte = 0;
                             personagem_acc_morte = 0;
                             personagem_morte_animacao_finalizada = false;
                         }
                     }
                }
            }

            // --- Lógica de Colisão Protagonista-Obstáculo e Coleta de Dinheiro ---
            if (protagonista_estado == PROT_NORMAL) {
                bool on_obstacle = false;

                float prot_current_hb_x = personagem_x + prot_hitbox_offset_x;
                float prot_current_hb_y = prot_hb_y;
                float prot_current_hb_w = prot_hitbox_width;
                float prot_current_hb_h = current_prot_hitbox_height;

                for (int i = 0; i < MAX_OBSTACULOS; i++) {
                    if (obstaculos[i].ativa) {
                        float obstacle_screen_x = obstaculos[i].x - deslocamento_x;
                        float obstacle_top = obstaculos[i].y;
                        float obstacle_width = obstaculos[i].width;
                        float obstacle_height = obstaculos[i].height;

                        if (check_collision(prot_current_hb_x, prot_current_hb_y, prot_current_hb_w, prot_current_hb_h,
                                            obstacle_screen_x, obstacle_top, obstacle_width, obstacle_height)) {

                            bool should_block_horizontally = !pulando && !(obstaculos[i].only_crouch_pass && agachando);

                            if (should_block_horizontally) {
                                deslocamento_x = old_deslocamento_x_prot;
                                break;
                            }
                        }
                    }
                }

                for (int i = 0; i < MAX_OBSTACULOS; i++) {
                    if (obstaculos[i].ativa) {
                        float obstacle_screen_x = obstaculos[i].x - deslocamento_x;
                        float obstacle_top = obstaculos[i].y;

                        if (pulando && vel_y > 0 &&
                            (old_personagem_y + prot_hitbox_offset_y + prot_current_hb_h) <= obstacle_top &&
                            (personagem_y + prot_hitbox_offset_y + prot_current_hb_h) > obstacle_top &&
                            (prot_current_hb_x + prot_current_hb_w > obstacle_screen_x) &&
                            (prot_current_hb_x < obstacle_screen_x + obstaculos[i].width)) {

                            if (!obstaculos[i].only_crouch_pass) {
                                personagem_y = obstacle_top - prot_hitbox_offset_y - prot_current_hb_h;
                                pulando = false;
                                vel_y = 0;
                                vel_x_pulo = 0;
                                current_ground_y = personagem_y;
                                on_obstacle = true;
                                break;
                            }
                        }

                        if (!pulando && !on_obstacle &&
                            (personagem_y + prot_hitbox_offset_y + prot_current_hb_h > obstacle_top - 5) &&
                            (personagem_y + prot_hitbox_offset_y + prot_current_hb_h < obstacle_top + 5) &&
                            (prot_current_hb_x + prot_current_hb_w > obstacle_screen_x) &&
                            (prot_current_hb_x < obstacle_screen_x + obstaculos[i].width)) {

                            if (!obstaculos[i].only_crouch_pass) {
                                on_obstacle = true;
                                current_ground_y = personagem_y;
                            }
                        }
                    }
                }

                if (!on_obstacle && !pulando && personagem_y < personagem_y_base) {
                    current_ground_y = personagem_y_base;
                }
            }

            if (protagonista_estado == PROT_NORMAL) {
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
            
            // --- DESENHO ANIMAÇÃO DO PERSONAGEM ---
            if (protagonista_estado == PROT_MORRENDO) {
                al_draw_scaled_bitmap(sprite_personagem_morte,
                                      personagem_frame_morte * frame_largura_personagem_morte, 0,
                                      frame_largura_personagem_morte, frame_altura,
                                      personagem_x, personagem_y,
                                      frame_largura_personagem_morte * escala_personagens, frame_altura * escala_personagens, 0);
            } else if (protagonist_visible) {
                if (pulando)
                    al_draw_scaled_bitmap(sprite_pulando, frame_pulando * frame_largura_pulando, 0, frame_largura_pulando, frame_altura,
                                         personagem_x, personagem_y, frame_largura_pulando * escala_personagens, frame_altura * escala_personagens, 0);
                else if (agachando)
                    al_draw_scaled_bitmap(sprite_agachado, frame_agachado * frame_largura_agachado, 0, frame_largura_agachado, frame_altura,
                                         personagem_x, personagem_y, frame_largura_agachado * escala_personagens, frame_altura * escala_personagens, 0);
                else if (atacando)
                    al_draw_scaled_bitmap(sprite_ataque1, frame_ataque1 * frame_largura_ataque1, 0, frame_largura_ataque1, frame_altura,
                                         personagem_x, personagem_y, frame_largura_ataque1 * escala_personagens, frame_altura * escala_personagens, 0);
                else if (especial_ativo)
                    al_draw_scaled_bitmap(sprite_especial, frame_especial * frame_largura_especial, 0, frame_largura_especial, frame_altura,
                                         personagem_x, personagem_y, frame_largura_especial * escala_personagens, frame_altura * escala_personagens, 0);
                else if (arremessando)
                    al_draw_scaled_bitmap(sprite_arremesso, frame_arremesso * frame_largura_arremesso, 0, frame_largura_arremesso, frame_altura,
                                         personagem_x, personagem_y, frame_largura_arremesso * escala_personagens, frame_altura * escala_personagens, 0);
                else if (andando)
                    al_draw_scaled_bitmap(correndo ? sprite_correndo : sprite_andando,
                                         (correndo ? frame_correndo : frame_andando) * (correndo ? frame_largura_correndo : frame_largura_andando),
                                         0,
                                         (correndo ? frame_largura_correndo : frame_largura_andando),
                                         frame_altura,
                                         personagem_x, personagem_y,
                                         (correndo ? frame_largura_correndo : frame_largura_andando) * escala_personagens,
                                         frame_altura * escala_personagens, 0);
                else
                    al_draw_scaled_bitmap(sprite_parado, frame_parado * frame_largura_parado, 0, frame_largura_parado, frame_altura,
                                         personagem_x, personagem_y, frame_largura_parado * escala_personagens, frame_altura * escala_personagens, 0);
            }

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

            float health_bar_width = (float)protagonist_health / (MAX_PROTAGONIST_HEALTH + NPC_HEAL_AMOUNT) * 200;
            if (health_bar_width < 0) health_bar_width = 0;
            ALLEGRO_COLOR health_color = al_map_rgb(0, 255, 0);
            if (protagonist_health > MAX_PROTAGONIST_HEALTH) {
                health_color = al_map_rgb(0, 255, 255);
            }
            al_draw_filled_rectangle(10, 10, 10 + health_bar_width, 30, health_color);
            al_draw_rectangle(10, 10, 10 + 200, 30, al_map_rgb(255, 255, 255), 2);
            al_draw_textf(fonte, al_map_rgb(255, 255, 255), 220, 15, 0, "HP: %d", protagonist_health);
            
            al_draw_textf(fonte, al_map_rgb(255, 255, 0), LARGURA - 10, 10, ALLEGRO_ALIGN_RIGHT, "R$ %d", player_money);

            al_flip_display();
        }
    }

    // --- Limpeza de Recursos (Liberação de Memória) ---
    if (instancia_som_verdebalaraio) {
        al_stop_sample_instance(instancia_som_verdebalaraio);
        al_destroy_sample_instance(instancia_som_verdebalaraio);
    }
    if (som_verdebalaraio) al_destroy_sample(som_verdebalaraio);

    al_destroy_bitmap(background);
    al_destroy_bitmap(sprite_parado);
    al_destroy_bitmap(sprite_andando);
    al_destroy_bitmap(sprite_correndo);
    al_destroy_bitmap(sprite_pulando);
    al_destroy_bitmap(sprite_agachado);
    al_destroy_bitmap(sprite_especial);
    al_destroy_bitmap(sprite_ataque1);
    al_destroy_bitmap(sprite_arremesso);
    al_destroy_bitmap(sprite_garrafa);
    al_destroy_bitmap(sprite_inimigo_parado);
    al_destroy_bitmap(sprite_inimigo_andando);
    al_destroy_bitmap(sprite_inimigo_morte);
    al_destroy_bitmap(sprite_inimigo_ataque);
    al_destroy_bitmap(sprite_personagem_morte);
    al_destroy_bitmap(sprite_traficante_parada);
    al_destroy_bitmap(sprite_sacos_lixo);
    al_destroy_bitmap(sprite_placa_radar);
    al_destroy_bitmap(sprite_2reais);
    al_destroy_bitmap(sprite_5reais);
    al_destroy_bitmap(sprite_10reais);
    al_destroy_bitmap(sprite_policial_parado);
    al_destroy_bitmap(sprite_policial_arremesso);
    al_destroy_bitmap(sprite_explosao);
    al_destroy_timer(timer);

    if (protagonista_estado == PROT_MORRENDO && personagem_morte_animacao_finalizada) {
        should_restart = game_over_screen(janela, fila, fonte);
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