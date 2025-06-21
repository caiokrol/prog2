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

// --- Define número máximo de garrafas ---
#define MAX_GARRAFAS 5 // Você pode ajustar este número

// --- Enumeração dos Estados do Inimigo ---
typedef enum {
    INIMIGO_PARADO,
    INIMIGO_ANDANDO,
    INIMIGO_ATACANDO, // NOVO: Estado para atacar
    INIMIGO_MORRENDO
} EnemyState;

// --- Enumeração dos Estados do Protagonista ---
typedef enum {
    PROT_NORMAL,     // Estado normal (parado, andando, pulando, agachando, atacando, arremessando)
    PROT_MORRENDO    // Estado de morte do protagonista
} ProtagonistState;

// Estrutura para propriedades do inimigo
typedef struct {
    float x, y;    // Posição do inimigo no MUNDO (não na TELA)
    float vel_x;    // Velocidade horizontal RELATIVA AO MUNDO
    int frame_atual;
    float acc_animacao;
    bool ativa;
    EnemyState estado;
    bool animacao_morte_finalizada; // NOVO: Flag para saber se a animação de morte foi concluída
    float hitbox_offset_x;
    float hitbox_offset_y;
    float hitbox_width;
    float hitbox_height;
    bool inimigo_pode_dar_dano; // NOVO: Controla se o inimigo pode dar dano neste momento
} Inimigo;

// --- Define número máximo de inimigos ---
#define MAX_INIMIGOS 3 // Você pode ajustar este número

// --- Definições de Vida do Protagonista ---
#define MAX_PROTAGONIST_HEALTH 100
#define PROTAGONIST_DAMAGE_PER_HIT 20 // Quantidade de vida perdida por acerto
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
    bool only_crouch_pass; // NOVO: Flag para obstáculos que só podem ser passados agachado
} Obstaculo;

// --- ATUALIZADO: Define número máximo de obstáculos ---
#define MAX_OBSTACULOS 40 // Aumentado para 4 para o novo obstáculo

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
    // Carrega a imagem de Game Over principal (tipo "GAME OVER" escrito)
    ALLEGRO_BITMAP *game_over_image = al_load_bitmap("gameover.png");
    if (!game_over_image) {
        fprintf(stderr, "ERRO: Não foi possível carregar gameover.png\n");
        return false; // Não pode tentar novamente se a imagem não carregar
    }

    // NOVO: Carrega a imagem de fundo para a tela de Game Over
    ALLEGRO_BITMAP *background_game_over = al_load_bitmap("backgroundGameOver.png");
    if (!background_game_over) {
        fprintf(stderr, "ERRO: Não foi possível carregar backgroundGameOver.png\n");
        al_destroy_bitmap(game_over_image); // Libera a imagem que já carregou
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
                case ALLEGRO_KEY_ESCAPE: // Permite sair da tela de game over também com ESC
                    restart_game = false;
                    sair_game_over_screen = true;
                    break;
            }
        } else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            restart_game = false;
            sair_game_over_screen = true;
        }

        // --- MUDANÇA PRINCIPAL AQUI ---
        // Desenha a imagem de fundo para cobrir toda a tela
        al_draw_scaled_bitmap(background_game_over, 0, 0,
                             al_get_bitmap_width(background_game_over), al_get_bitmap_height(background_game_over),
                             0, 0, LARGURA, ALTURA, 0);

        // Desenha a imagem de Game Over centralizada (por cima do fundo)
        // Note que o cálculo do 'dy' foi corrigido para melhor centralização vertical na sua última versão.
        al_draw_scaled_bitmap(game_over_image, 0, 0,
                             al_get_bitmap_width(game_over_image), al_get_bitmap_height(game_over_image),
                             (LARGURA - al_get_bitmap_width(game_over_image)) / 2.0,
                             (ALTURA / 2.0 - al_get_bitmap_height(game_over_image) / 2.0) - 50, // Ajuste vertical para ficar um pouco acima do centro
                             al_get_bitmap_width(game_over_image), al_get_bitmap_height(game_over_image), 0);
        
        // Desenha as opções (o texto das opções agora é preto para contraste no fundo branco,
        // mas com sua nova imagem, talvez seja preciso ajustar a cor novamente).
        int start_y = ALTURA / 2 + 130; // Posição inicial das opções abaixo da imagem
        for (int i = 0; i < num_opcoes_game_over; i++) {
            ALLEGRO_COLOR cor_texto = (i == opcao_selecionada_go) ? al_map_rgb(0, 255, 100) : al_map_rgb(255, 255, 255); // Texto preto, selecionado laranja
            // Se sua imagem de fundo for escura, mude a cor padrão do texto para branco (al_map_rgb(255, 255, 255)).
            al_draw_text(fonte, cor_texto, LARGURA / 2, start_y + i * al_get_font_line_height(fonte) * 1.5, ALLEGRO_ALIGN_CENTER, opcoes_game_over[i]);
        }

        al_flip_display();
    }

    al_destroy_bitmap(game_over_image);
    al_destroy_bitmap(background_game_over); // NOVO: Libera o bitmap do fundo
    return restart_game;
}

// --- Função Principal do Jogo (COM SISTEMA DE PAUSA E GAME OVER) ---
// Retorna true se o jogo deve ser reiniciado, false se deve sair
bool jogo(ALLEGRO_DISPLAY *janela, ALLEGRO_EVENT_QUEUE *fila, ALLEGRO_FONT *fonte) {
    fprintf(stderr, "DEBUG: Início da função jogo.\n");

    // --- Variáveis de Jogo ---
    float deslocamento_x = 0.0; // Deslocamento global do mundo (mundo "infinito")
    float velocidade_andar = 3.0;
    float velocidade_correr = 6.0;
    float velocidade = velocidade_andar;
    float escala_personagens = 3.0;
    float escala_traficante = 1.7; // Escala específica para o NPC Traficante

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
    ALLEGRO_BITMAP *sprite_inimigo_parado = al_load_bitmap("noia1_parado.png");
    ALLEGRO_BITMAP *sprite_inimigo_andando = al_load_bitmap("noia1_andando.png");
    ALLEGRO_BITMAP *sprite_inimigo_morte = al_load_bitmap("noia1_morte.png");
    ALLEGRO_BITMAP *sprite_inimigo_ataque = al_load_bitmap("noia1_ataque.png");
    ALLEGRO_BITMAP *sprite_personagem_morte = al_load_bitmap("personagem_morte.png");
    ALLEGRO_BITMAP *sprite_traficante_parada = al_load_bitmap("traficante_parada.png");
    ALLEGRO_BITMAP *sprite_sacos_lixo = al_load_bitmap("sacos_lixo.png");
    ALLEGRO_BITMAP *sprite_placa_radar = al_load_bitmap("placa_radar.png"); // NOVO: Carrega sprite de novo obstáculo

    // --- NOVO: Carrega Sprites de Notas de Dinheiro ---
    ALLEGRO_BITMAP *sprite_2reais = al_load_bitmap("2reais.png");
    ALLEGRO_BITMAP *sprite_5reais = al_load_bitmap("5reais.png");
    ALLEGRO_BITMAP *sprite_10reais = al_load_bitmap("10reais.png");
    // --- FIM NOVO ---

    // --- Carregamento de Áudio ---
    ALLEGRO_SAMPLE *som_verdebalaraio = al_load_sample("verdebalaraio.ogg");
    ALLEGRO_SAMPLE_INSTANCE *instancia_som_verdebalaraio = NULL;

    // --- Verificação de Carregamento de Ativos (Bitmaps e Áudios) ---
    if (!background || !sprite_parado || !sprite_andando || !sprite_correndo || !sprite_pulando ||
        !sprite_agachado || !sprite_especial || !sprite_ataque1 || !sprite_arremesso || !sprite_garrafa ||
        !sprite_inimigo_parado || !sprite_inimigo_andando || !sprite_inimigo_morte || !sprite_inimigo_ataque ||
        !sprite_personagem_morte || !sprite_traficante_parada || !som_verdebalaraio || !sprite_sacos_lixo ||
        !sprite_placa_radar || !sprite_2reais || !sprite_5reais || !sprite_10reais) {
        fprintf(stderr, "ERRO CRÍTICO: Falha ao carregar um ou mais bitmaps/áudios! Verifique nomes/caminhos dos arquivos.\n");
        // Mensagens de erro detalhadas para depuração
        if (!background) fprintf(stderr, "Falha ao carregar background.png\n");
        if (!sprite_parado) fprintf(stderr, "Falha ao carregar personagem.png\n");
        if (!sprite_traficante_parada) fprintf(stderr, "Falha ao carregar traficante_parada.png\n");
        if (!som_verdebalaraio) fprintf(stderr, "Falha ao carregar verdebalaraio.ogg\n");
        if (!sprite_sacos_lixo) fprintf(stderr, "Falha ao carregar sacos_lixo.png\n");
        if (!sprite_placa_radar) fprintf(stderr, "Falha ao carerrar placa_radar.png\n");
        if (!sprite_2reais) fprintf(stderr, "Falha ao carregar 2reais.png\n");
        if (!sprite_5reais) fprintf(stderr, "Falha ao carregar 5reais.png\n");
        if (!sprite_10reais) fprintf(stderr, "Falha ao carregar 10reais.png\n");

        // Limpa todos os ativos carregados antes de retornar em caso de falha
        if (background) al_destroy_bitmap(background);
        if (sprite_parado) al_destroy_bitmap(sprite_parado);
        if (sprite_andando) al_destroy_bitmap(sprite_andando);
        if (sprite_correndo) al_destroy_bitmap(sprite_correndo);
        if (sprite_pulando) al_destroy_bitmap(sprite_pulando);
        if (sprite_agachado) al_destroy_bitmap(sprite_agachado);
        if (sprite_especial) al_destroy_bitmap(sprite_especial);
        if (sprite_ataque1) al_destroy_bitmap(sprite_ataque1);
        if (sprite_arremesso) al_destroy_bitmap(sprite_arremesso);
        if (sprite_garrafa) al_destroy_bitmap(sprite_garrafa);
        if (sprite_inimigo_parado) al_destroy_bitmap(sprite_inimigo_parado);
        if (sprite_inimigo_andando) al_destroy_bitmap(sprite_inimigo_andando);
        if (sprite_inimigo_morte) al_destroy_bitmap(sprite_inimigo_morte);
        if (sprite_inimigo_ataque) al_destroy_bitmap(sprite_inimigo_ataque);
        if (sprite_personagem_morte) al_destroy_bitmap(sprite_personagem_morte);
        if (sprite_traficante_parada) al_destroy_bitmap(sprite_traficante_parada);
        if (som_verdebalaraio) al_destroy_sample(som_verdebalaraio);
        if (sprite_sacos_lixo) al_destroy_bitmap(sprite_sacos_lixo);
        if (sprite_placa_radar) al_destroy_bitmap(sprite_placa_radar);
        if (sprite_2reais) al_destroy_bitmap(sprite_2reais);
        if (sprite_5reais) al_destroy_bitmap(sprite_5reais);
        if (sprite_10reais) al_destroy_bitmap(sprite_10reais);
        return false; // Indica falha fatal, não reiniciar
    }
    fprintf(stderr, "DEBUG: Todos os bitmaps e áudios carregados com sucesso.\n");

    // --- Cria e anexa instância de áudio ---
    instancia_som_verdebalaraio = al_create_sample_instance(som_verdebalaraio);
    if (!instancia_som_verdebalaraio) {
        fprintf(stderr, "ERRO CRÍTICO: Falha ao criar instancia_som_verdebalaraio!\n");
        // Limpa os ativos carregados
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
        return false; // Indica falha fatal, não reiniciar
    }
    al_set_sample_instance_playmode(instancia_som_verdebalaraio, ALLEGRO_PLAYMODE_LOOP);
    al_attach_sample_instance_to_mixer(instancia_som_verdebalaraio, al_get_default_mixer());
    al_set_sample_instance_gain(instancia_som_verdebalaraio, 0.0); // Começa com volume zero
    al_play_sample_instance(instancia_som_verdebalaraio); // Começa a tocar (silenciosamente)
    // --- FIM NOVO ---

    // --- Dimensões do Background para Tiling ---
    int bg_width = al_get_bitmap_width(background);
    int bg_height = al_get_bitmap_height(background);

    // --- Definições de Frames e Alturas dos Sprites ---
    int frame_total_parado = 6, frame_total_andando = 8, frame_total_correndo = 8, frame_total_pulando = 16, frame_total_agachado = 8;
    int frame_total_especial = 13;
    int frame_total_ataque1 = 5;
    int frame_total_arremesso = 4;
    int frame_total_inimigo_parado = 7;
    int frame_total_inimigo_andando = 8;
    int frame_total_inimigo_morte = 4;
    int frame_total_inimigo_ataque = 4;
    int frame_total_personagem_morte = 4;

    int frame_largura_parado = 768 / frame_total_parado;
    int frame_largura_andando = 1024 / frame_total_andando;
    int frame_largura_correndo = 1024 / frame_total_correndo;
    int frame_largura_pulando = 2048 / frame_total_pulando;
    int frame_largura_agachado = 1024 / frame_total_agachado;
    int frame_largura_especial = 1664 / frame_total_especial;
    int frame_largura_ataque1 = 640 / frame_total_ataque1;
    int frame_largura_arremesso = 512 / frame_total_arremesso;
    int frame_altura = 128; // Altura do sprite do personagem (e do inimigo)

    int frame_largura_inimigo_parado = al_get_bitmap_width(sprite_inimigo_parado) / frame_total_inimigo_parado;
    int frame_largura_inimigo_andando = al_get_bitmap_width(sprite_inimigo_andando) / frame_total_inimigo_andando;
    int frame_largura_inimigo_morte = al_get_bitmap_width(sprite_inimigo_morte) / frame_total_inimigo_morte;
    int frame_largura_inimigo_ataque = al_get_bitmap_width(sprite_inimigo_ataque) / frame_total_inimigo_ataque;
    int frame_largura_personagem_morte = al_get_bitmap_width(sprite_personagem_morte) / frame_total_personagem_morte;

    int inimigo_largura_sprite_max = (frame_largura_inimigo_parado > frame_largura_inimigo_andando) ? frame_largura_inimigo_parado : frame_largura_inimigo_andando;
    if (frame_largura_inimigo_morte > inimigo_largura_sprite_max) {
        inimigo_largura_sprite_max = frame_largura_inimigo_morte;
    }
    if (frame_largura_inimigo_ataque > inimigo_largura_sprite_max) {
        inimigo_largura_sprite_max = frame_largura_inimigo_ataque;
    }
    int inimigo_altura_sprite = frame_altura;

    // --- Propriedades da Hitbox do Protagonista ---
    float prot_hitbox_offset_x = 40.0 * escala_personagens;
    float prot_hitbox_offset_y = 5.0 * escala_personagens;  // Ajustado para os pés
    float prot_hitbox_width = (frame_largura_parado - 80) * escala_personagens;
    float prot_hitbox_height = (frame_altura - 10) * escala_personagens; // Ajustado para os pés
    // --- Fim Hitbox do Protagonista ---

    // --- Propriedades da Hitbox de ATAQUE do Protagonista ---
    float prot_attack_hitbox_offset_x = 60.0 * escala_personagens;
    float prot_attack_hitbox_offset_y = 50.0 * escala_personagens;
    float prot_attack_hitbox_width = 50 * escala_personagens;
    float prot_attack_hitbox_height = 80.0 * escala_personagens;
    // --- FIM Hitbox de ATAQUE do Protagonista ---

    // --- Propriedades da Garrafa ---
    int garrafa_largura_original = al_get_bitmap_width(sprite_garrafa);
    int garrafa_altura_original = al_get_bitmap_height(sprite_garrafa);
    float escala_garrafa = 1.0;
    float garrafa_hitbox_scale = 0.8;
    float garrafa_hitbox_width = garrafa_largura_original * escala_garrafa * garrafa_hitbox_scale;
    float garrafa_hitbox_height = garrafa_altura_original * escala_garrafa * garrafa_hitbox_scale;

    // --- Propriedades de Obstáculos (para diferentes tipos) ---
    // Sacos de lixo (existente)
    float escala_obstaculo_sacos = 0.1;
    int obstaculo_largura_original_sacos = al_get_bitmap_width(sprite_sacos_lixo);
    int obstaculo_altura_original_sacos = al_get_bitmap_height(sprite_sacos_lixo);
    float obstaculo_width_sacos = obstaculo_largura_original_sacos * escala_obstaculo_sacos;
    float obstaculo_height_sacos = obstaculo_altura_original_sacos * escala_obstaculo_sacos;

    // NOVO: Propriedades da Placa de Radar
    float escala_placa_radar = 0.3; // Ajuste conforme necessário para o tamanho
    int placa_radar_largura_original = al_get_bitmap_width(sprite_placa_radar);
    int placa_radar_altura_original = al_get_bitmap_height(sprite_placa_radar);
    float placa_radar_width = placa_radar_largura_original * escala_placa_radar;
    float placa_radar_height = placa_radar_altura_original * escala_placa_radar;

    float personagem_x = LARGURA / 2.0 - (frame_largura_correndo * escala_personagens) / 2.0;
    float personagem_y_base = (ALTURA - 300) - (frame_altura * escala_personagens) / 2.0;
    float personagem_y = personagem_y_base;

    // Altura da hitbox do protagonista agachado (Aproximação)
    float prot_crouch_hitbox_height = (frame_altura - 50) * escala_personagens; // Exemplo: 50 pixels mais curta ao agachar

    // --- Variáveis de Animação do Personagem ---
    int frame_parado = 0, frame_andando = 0, frame_correndo = 0, frame_pulando = 0, frame_agachado = 0, frame_especial = 0, frame_ataque1 = 0;
    int frame_arremesso = 0;
    // --- Variáveis de Animação de Morte do Protagonista ---
    int personagem_frame_morte = 0;
    float personagem_acc_morte = 0;
    float tpf_personagem_morte = 1.0 / 8.0; // Velocidade da animação de morte do protagonista
    bool personagem_morte_animacao_finalizada = false;

    // --- Tempos Por Frame (TPF) para Animações ---
    float tpf_parado = 1.0 / 5.0, tpf_andando = 1.0 / 10.0, tpf_correndo = 1.0 / 10.0, tpf_pulando = 1.0 / 12, tpf_agachado = 1.0 / 8.0;
    float tpf_especial = 1.0 / 15.0;
    float tpf_ataque1 = 1.0 / 10.0;
    float tpf_arremesso = 1.0 / 8.0;
    float tpf_inimigo_parado = 1.0 / 8.0;
    float tpf_inimigo_andando = 1.0 / 10.0;
    float tpf_inimigo_ataque = 1.0 / 8.0;
    float tpf_inimigo_morte = 1.0 / 8.0;

    // --- Acumuladores de Tempo para Animações ---
    float acc_parado = 0, acc_andando = 0, acc_correndo = 0, acc_pulando = 0, acc_agachado = 0, acc_especial = 0, acc_ataque1 = 0;
    float acc_arremesso = 0;

    // --- Variáveis de Estado do Protagonista ---
    ProtagonistState protagonista_estado = PROT_NORMAL;
    int protagonist_health = MAX_PROTAGONIST_HEALTH; // Vida atual do protagonista
    // --- Variáveis de Invulnerabilidade ---
    bool protagonist_invulnerable = false;
    float protagonist_invulnerability_timer = 0.0;
    float protagonist_blink_timer = 0.0;
    bool protagonist_visible = true; // Para efeito de piscar

    // --- NOVO: Variável de Dinheiro do Jogador ---
    int player_money = 2; // Dinheiro inicial

    // --- Inicialização do NPC Traficante ---
    NPC traficante;
    traficante.largura_sprite = al_get_bitmap_width(sprite_traficante_parada) / NPC_TRAFICANTE_FRAME_COUNT;
    traficante.altura_sprite = al_get_bitmap_height(sprite_traficante_parada);
    // MODIFICADO: Posição inicial do traficante para geração procedural no mundo infinito
    traficante.x = 4000; // Coloca o primeiro traficante bem à frente no início do jogo (coordenada de mundo)
    traficante.y = personagem_y_base - (traficante.altura_sprite * escala_traficante - frame_altura * escala_personagens);
    traficante.ativa = true;
    traficante.frame_atual = 0;
    traficante.acc_animacao = 0;

    // --- Variáveis de Movimento do Protagonista ---
    bool pulando = false;
    bool agachando = false;
    bool especial_ativo = false;
    bool especial_finalizado = false;
    bool atacando = false;
    bool arremessando = false;
    bool crouch_animation_finished = false; // Flag para controlar o fim da animação de agachar

    float vel_y = 0.0;
    float gravidade = 0.5; // Gravidade aumentada para quedas mais rápidas
    float vel_x_pulo = 0.0;

    // --- NOVO: Variável para o chão atual do protagonista (base ou topo de obstáculo) ---
    float current_ground_y = personagem_y_base;

    // --- Inicialização de Garrafas ---
    Garrafa garrafas[MAX_GARRAFAS];
    float garrafa_velocidade_angular = 0.2;
    for (int i = 0; i < MAX_GARRAFAS; i++) {
        garrafas[i].ativa = false;
    }

    // --- Inicialização de Inimigos ---
    Inimigo inimigos[MAX_INIMIGOS];
    float tempo_para_spawn_inimigo = 0.5;
    float min_intervalo_spawn_inimigo = 0.5;
    float max_intervalo_spawn_inimigo = 2.0;

    float inimigo_velocidade_parado_base = -0.5; // Ajustado para ser negativo para mover para a esquerda
    float inimigo_velocidade_andando_base = -2.5; // Ajustado para ser negativo para mover para a esquerda
    float inimigo_velocidade_ataque_base = 0.0; // Inimigo para ao atacar

    float inimigo_distancia_deteccao = 500.0;
    float inimigo_distancia_ataque = 180.0;

    float deslocamento_x_anterior = deslocamento_x; // Para calcular a movimentação do mundo

    fprintf(stderr, "DEBUG: Inicializando array de inimigos (MAX_INIMIGOS: %d).\n", MAX_INIMIGOS);
    for (int i = 0; i < MAX_INIMIGOS; i++) {
        inimigos[i].ativa = false;
        inimigos[i].frame_atual = 0;
        inimigos[i].acc_animacao = 0;
        inimigos[i].estado = INIMIGO_PARADO;
        inimigos[i].vel_x = inimigo_velocidade_parado_base;
        inimigos[i].animacao_morte_finalizada = false;
        inimigos[i].inimigo_pode_dar_dano = true;

        inimigos[i].hitbox_offset_x = 57.0 * escala_personagens;
        inimigos[i].hitbox_offset_y = 20 * escala_personagens;
        inimigos[i].hitbox_width = (frame_largura_inimigo_parado - 100) * escala_personagens;
        inimigos[i].hitbox_height = (inimigo_altura_sprite - 20) * escala_personagens;

        fprintf(stderr, "DEBUG: Inimigo %d inicializado como inativo.\n", i);
    }
    fprintf(stderr, "DEBUG: Tempo inicial para spawn de inimigo: %.2f\n", tempo_para_spawn_inimigo);

    // --- Inicialização de Obstáculos para Geração Procedural ---
    Obstaculo obstaculos[MAX_OBSTACULOS];
    srand(time(NULL)); // Semeia o gerador de números aleatórios

    // Array para conter os tipos de obstáculos e suas propriedades para facilitar a randomização
    struct {
        ALLEGRO_BITMAP *sprite;
        float scale;
        bool crouch_pass;
    } obstacle_types[] = {
        {sprite_sacos_lixo, escala_obstaculo_sacos, false},
        {sprite_placa_radar, escala_placa_radar, true}
    };
    int num_obstacle_types = sizeof(obstacle_types) / sizeof(obstacle_types[0]);

    // Variáveis para controlar a geração procedural de obstáculos
    float proximo_ponto_spawn_obstaculo = LARGURA; // Onde o próximo obstáculo pode aparecer (coordenada de mundo)
    float min_distancia_entre_obstaculos = 500.0;
    float variacao_distancia_obstaculos = 600.0; // Distância aleatória adicional

    // Posiciona os obstáculos iniciais de forma procedural
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

        // Posiciona o obstáculo à frente do último que foi posicionado
        obstaculos[i].x = proximo_ponto_spawn_obstaculo + (float)rand() / RAND_MAX * variacao_distancia_obstaculos;
        obstaculos[i].y = personagem_y_base + (frame_altura * escala_personagens) - obstaculos[i].height;

        // Atualiza o ponto para o próximo spawn
        proximo_ponto_spawn_obstaculo = obstaculos[i].x + obstaculos[i].width + min_distancia_entre_obstaculos;
    }
    // --- FIM DA MODIFICAÇÃO DE INICIALIZAÇÃO DE OBSTÁCULOS ---

    // --- Inicialização de Notas de Dinheiro ---
    MoneyNote money_notes[MAX_MONEY_NOTES];
    ALLEGRO_BITMAP *money_sprites[] = {sprite_2reais, sprite_5reais, sprite_10reais};
    int money_values[] = {2, 5, 10};
    int num_money_types = sizeof(money_sprites) / sizeof(money_sprites[0]);
    float time_to_spawn_money = MONEY_NOTE_SPAWN_INTERVAL; // Tempo para o próximo spawn de dinheiro

    for (int i = 0; i < MAX_MONEY_NOTES; i++) {
        money_notes[i].ativa = false;
    }
    // --- FIM NOVO ---

    // --- Configuração do Timer do Jogo ---
    ALLEGRO_TIMER *timer = al_create_timer(1.0 / 60.0); // 60 FPS
    al_start_timer(timer);

    // --- Registro de Fontes de Eventos ---
    al_register_event_source(fila, al_get_display_event_source(janela));
    al_register_event_source(fila, al_get_keyboard_event_source());
    al_register_event_source(fila, al_get_timer_event_source(timer));

    bool jogando = true;
    bool should_restart = false; // Flag para indicar se o jogo deve ser reiniciado

    // --- Loop Principal do Jogo ---
    while (jogando) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(fila, &ev); // Espera por um evento

        // --- Tratamento de Eventos da Janela ---
        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            jogando = false; // Sai do loop se a janela for fechada
            should_restart = false; // Não reinicia
        } else if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            
            // --- NOVO: LÓGICA DE PAUSA ---
            // Esta lógica agora é a primeira a ser verificada, funcionando em qualquer estado do jogo.
            if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                al_stop_timer(timer); // Para o tempo do jogo, congelando todas as atualizações de lógica.

                // Desenha uma sobreposição semi-transparente e o texto de pausa.
                al_draw_filled_rectangle(0, 0, LARGURA, ALTURA, al_map_rgba(0, 0, 0, 150));
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 2, (ALTURA / 2) - al_get_font_line_height(fonte), ALLEGRO_ALIGN_CENTER, "PAUSADO");
                al_draw_text(fonte, al_map_rgb(200, 200, 200), LARGURA / 2, (ALTURA / 2) + al_get_font_line_height(fonte), ALLEGRO_ALIGN_CENTER, "Pressione ESC para continuar");
                al_flip_display();

                // Entra em um loop de pausa, esperando apenas a tecla ESC ou o fechamento da janela.
                bool despausar = false;
                while (!despausar) {
                    ALLEGRO_EVENT evento_pausa;
                    al_wait_for_event(fila, &evento_pausa);

                    if (evento_pausa.type == ALLEGRO_EVENT_KEY_DOWN) {
                        if (evento_pausa.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                            despausar = true; // Sinaliza para sair do loop de pausa.
                        }
                    } else if (evento_pausa.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
                        jogando = false;     // Sinaliza para sair do loop principal do jogo.
                        should_restart = false; // Não reinicia
                        despausar = true;    // Sai do loop de pausa para poder encerrar o jogo.
                    }
                }

                // Se o jogo não foi fechado, reinicia o timer para continuar.
                if (jogando) {
                    al_start_timer(timer);
                }
            } 
            // --- FIM DA LÓGICA DE PAUSA ---
            
            // O resto do tratamento de teclas continua aqui.
            else { // Adicionado um 'else' para aninhar a lógica de teclas anterior
                 // --- Protagonista não pode realizar ações se estiver morrendo ---
                if (protagonista_estado == PROT_MORRENDO) {
                    // --- REMOVIDO: a checagem 'if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE)' foi movida para a lógica de pausa global ---
                }
            
                // --- REMOVIDO: a checagem 'if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE)' foi movida para a lógica de pausa global ---

                if (protagonista_estado == PROT_NORMAL) { // Só permite ações se o protagonista estiver normal
                    if (ev.keyboard.keycode == ALLEGRO_KEY_UP && !pulando && !agachando && !atacando && !arremessando) {
                        pulando = true; vel_y = -10.0; frame_pulando = 0; acc_pulando = 0; especial_ativo = false; especial_finalizado = false;
                        ALLEGRO_KEYBOARD_STATE estado; al_get_keyboard_state(&estado);
                        if (al_key_down(&estado, ALLEGRO_KEY_RIGHT)) vel_x_pulo = velocidade;
                        else if (al_key_down(&estado, ALLEGRO_KEY_LEFT)) vel_x_pulo = -velocidade;
                        else vel_x_pulo = 0;
                        // Ao pular, imediatamente define o chão como a base, a menos que ele pouse em outra coisa
                        current_ground_y = personagem_y_base;
                    }
                    // Ao pressionar DOWN, entra no estado de agachar
                    else if (ev.keyboard.keycode == ALLEGRO_KEY_DOWN && !pulando && !atacando && !arremessando) {
                        agachando = true; frame_agachado = 0; acc_agachado = 0; especial_ativo = false; especial_finalizado = false;
                        crouch_animation_finished = false; // Garante que a flag esteja falsa ao iniciar a animação
                    }
                    else if (ev.keyboard.keycode == ALLEGRO_KEY_Z && !pulando && !agachando && !especial_ativo && !atacando && !arremessando) { especial_ativo = true; especial_finalizado = false; frame_especial = 0; acc_especial = 0; }
                    else if (ev.keyboard.keycode == ALLEGRO_KEY_SPACE && especial_ativo && especial_finalizado && !atacando && !arremessando) { atacando = true; frame_ataque1 = 0; acc_ataque1 = 0; }
                    else if (ev.keyboard.keycode == ALLEGRO_KEY_Q && !agachando && !especial_ativo && !atacando && !arremessando) { arremessando = true; frame_arremesso = 0; acc_arremesso = 0; }
                    // --- Tecla 'B' para Interação com NPC ---
                    else if (ev.keyboard.keycode == ALLEGRO_KEY_B) {
                        // Calcula o centro da hitbox do protagonista (coordenadas do mundo)
                        float prot_world_x_center = personagem_x + (frame_largura_parado * escala_personagens) / 2.0 + deslocamento_x;
                        float npc_world_x_center = traficante.x + (traficante.largura_sprite * escala_traficante) / 2.0;

                        float distance = fabs(prot_world_x_center - npc_world_x_center);

                        if (traficante.ativa && distance < NPC_INTERACTION_DISTANCE) {
                            if (player_money >= NPC_HEAL_COST) {
                                // NOVO: Verifica se a vida do protagonista já está no máximo ou acima do novo máximo
                                if (protagonist_health >= MAX_PROTAGONIST_HEALTH + NPC_HEAL_AMOUNT) { // Máximo 100 + 20 de uma compra
                                    fprintf(stderr, "DEBUG: Vida já está no máximo (120 HP)! Não é possível comprar mais pó.\n");
                                } else {
                                    player_money -= NPC_HEAL_COST;
                                    protagonist_health += NPC_HEAL_AMOUNT;
                                    // NOVO: Limita a vida em 120 (MAX_PROTAGONIST_HEALTH + NPC_HEAL_AMOUNT)
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
                    // --- FIM NOVO ---
                }
            }

        }
        // Quando a tecla é liberada, define uma flag para que a tecla não esteja mais pressionada
        else if (ev.type == ALLEGRO_EVENT_KEY_UP) {
            if (protagonista_estado == PROT_NORMAL) {
                if (ev.keyboard.keycode == ALLEGRO_KEY_DOWN) {
                    // 'agachando' NÃO é definido como falso aqui diretamente.
                    // Ele só se tornará falso no loop do timer quando a animação terminar E a tecla for liberada.
                }
            }
        }

        else if (ev.type == ALLEGRO_EVENT_TIMER) {
            float delta_deslocamento_x = deslocamento_x - deslocamento_x_anterior;
            deslocamento_x_anterior = deslocamento_x;

            ALLEGRO_KEYBOARD_STATE estado;
            al_get_keyboard_state(&estado);
            bool andando = false, correndo = false;

            // --- Lógica de Movimento do Protagonista ---
            float old_deslocamento_x_prot = deslocamento_x;
            float old_personagem_x = personagem_x; // Armazena o X antigo para rollback de colisão
            float old_personagem_y = personagem_y; // Armazena o Y antigo para rollback de colisão

            if (protagonista_estado == PROT_NORMAL) {
                if (!atacando && (!especial_ativo || especial_finalizado) && !arremessando) {
                    velocidade = (al_key_down(&estado, ALLEGRO_KEY_LSHIFT) || al_key_down(&estado, ALLEGRO_KEY_RSHIFT)) ? velocidade_correr : velocidade_andar;
                    if (!pulando) {
                        if (al_key_down(&estado, ALLEGRO_KEY_RIGHT)) {
                            deslocamento_x += velocidade;
                            // Não faz wrap aqui para permitir o mundo infinito
                            andando = true;
                            if (velocidade == velocidade_correr) correndo = true;
                        }
                        if (al_key_down(&estado, ALLEGRO_KEY_LEFT)) {
                            deslocamento_x -= velocidade;
                            // Não faz wrap aqui para permitir o mundo infinito
                            andando = true;
                            if (velocidade == velocidade_correr) correndo = true;
                        }
                    }
                } else if (pulando) {
                    velocidade = (al_key_down(&estado, ALLEGRO_KEY_LSHIFT) || al_key_down(&estado, ALLEGRO_KEY_RSHIFT)) ? velocidade_correr : velocidade_andar;
                }
            } else { // Se o protagonista não está no estado NORMAL (ex: morrendo), reseta as ações
                andando = false; correndo = false;
                pulando = false; agachando = false; atacando = false; arremessando = false;
                vel_y = 0; vel_x_pulo = 0;
                especial_ativo = false; especial_finalizado = false;
                frame_ataque1 = 0; acc_ataque1 = 0;

                frame_arremesso = 0; // Reseta animação de arremesso ao morrer
                acc_arremesso = 0; // Reseta acumulador de animação de arremesso ao morrer
            }

            if (protagonista_estado == PROT_NORMAL) {
                // --- Atualização do Temporizador de Invulnerabilidade ---
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
                // --- FIM NOVO ---

                if (pulando) {
                    personagem_y += vel_y;
                    vel_y += gravidade;
                    deslocamento_x += vel_x_pulo;
                    // Não faz wrap aqui para permitir o mundo infinito

                    // Verifica se aterrissou no chão base
                    if (personagem_y >= personagem_y_base && vel_y >= 0) {
                        personagem_y = personagem_y_base;
                        pulando = false;
                        vel_y = 0;
                        vel_x_pulo = 0;
                        current_ground_y = personagem_y_base; // Aterrissou no chão base
                    }

                    acc_pulando += 1.0 / 60.0; if (acc_pulando >= tpf_pulando) { acc_pulando -= tpf_pulando; if (frame_pulando < frame_total_pulando - 1) frame_pulando++; }
                } else if (agachando) {
                    acc_agachado += 1.0 / 60.0;
                    if (acc_agachado >= tpf_agachado) {
                        acc_agachado -= tpf_agachado;
                        if (frame_agachado < frame_total_agachado - 1) {
                            frame_agachado++;
                        } else {
                            // Animação finalizada.
                            crouch_animation_finished = true; // Seta a flag para indicar que a animação terminou
                            frame_agachado = frame_total_agachado - 1; // Trava no último frame
                        }
                    }
                    // Se o personagem NÃO está pressionando a tecla DOWN
                    // E a animação de agachar já terminou, então ele para de agachar.
                    // Caso contrário, ele continua agachado (no último frame se a animação já terminou).
                    if (!al_key_down(&estado, ALLEGRO_KEY_DOWN) && crouch_animation_finished) {
                        agachando = false;
                        crouch_animation_finished = false; // Reseta a flag para a próxima vez
                        frame_agachado = 0; // Reseta o frame da animação para começar do zero na próxima agachada
                        acc_agachado = 0;   // Reseta o acumulador da animação
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
                    frame_parado = 0; acc_parado = 0; frame_andando = 0; acc_andando = 0; frame_correndo = 0; acc_correndo = 0; frame_agachado = 0; acc_agachado = 0; atacando = false; acc_ataque1 = 0; frame_ataque1 = 0;
                } else {
                    // Aplica gravidade mesmo quando não está "pulando" para cair de obstáculos
                    if (personagem_y < current_ground_y) {
                        personagem_y += gravidade * 8;
                        // Prende a current_ground_y se ultrapassar
                        if (personagem_y > current_ground_y) {
                            personagem_y = current_ground_y;
                            vel_y = 0;
                        }
                    } else if (personagem_y > current_ground_y) { // Se de alguma forma estiver abaixo, ajusta para o chão
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
                        jogando = false; // Sai do loop principal do jogo para ir para a tela de Game Over
                        should_restart = false; // Define como false inicialmente, a tela de GO decidirá
                        fprintf(stderr, "DEBUG: Animação de morte do protagonista finalizada. Indo para tela de Game Over.\n");
                    }
                }
            }

            // --- Atualiza Temporizador de Animação do NPC ---
            if (traficante.ativa) {
                traficante.acc_animacao += 1.0 / 60.0;
                if (traficante.acc_animacao >= NPC_TRAFICANTE_TPF) {
                    traficante.acc_animacao -= NPC_TRAFICANTE_TPF;
                    traficante.frame_atual = (traficante.frame_atual + 1) % NPC_TRAFICANTE_FRAME_COUNT;
                }
            }
            // --- FIM NOVO ---

            // --- Atualiza Volume do Áudio do NPC (Fade) ---
            if (traficante.ativa && instancia_som_verdebalaraio) {
                // Calcula o centro da hitbox do protagonista (coordenadas do mundo)
                float prot_world_x_center = personagem_x + (frame_largura_parado * escala_personagens) / 2.0 + deslocamento_x;
                float npc_world_x_center = traficante.x + (traficante.largura_sprite * escala_traficante) / 2.0;

                float distance = fabs(prot_world_x_center - npc_world_x_center);
                float volume;

                if (distance <= AUDIO_MIN_DISTANCE) {
                    volume = 1.0; // Volume total
                } else if (distance >= AUDIO_MAX_DISTANCE) {
                    volume = 0.0; // Mudo
                } else {
                    // Interpolação linear: o volume diminui à medida que a distância aumenta
                    volume = 1.0 - ((distance - AUDIO_MIN_DISTANCE) / (AUDIO_MAX_DISTANCE - AUDIO_MIN_DISTANCE));
                    if (volume < 0.0) volume = 0.0; // Limita a 0
                    if (volume > 1.0) volume = 1.0; // Limita a 1
                }
                al_set_sample_instance_gain(instancia_som_verdebalaraio, volume);
            }
            // --- FIM NOVO ---

            // --- Atualização de Garrafas ---
            for (int i = 0; i < MAX_GARRAFAS; i++) {
                if (garrafas[i].ativa) {
                    // Garrafas se movem no mundo, e sua posição 'x' é uma coordenada de mundo.
                    // O desenho já compensa o scroll.
                    garrafas[i].x += garrafas[i].vel_x;
                    garrafas[i].angulo += garrafa_velocidade_angular;
                    if (garrafas[i].angulo > ALLEGRO_PI * 2) { // Normaliza o ângulo
                        garrafas[i].angulo -= ALLEGRO_PI * 2;
                    }
                    // Desativa se sair da tela (à direita, em relação ao scroll)
                    if (garrafas[i].x - deslocamento_x > LARGURA + (garrafa_largura_original * escala_garrafa)) {
                        garrafas[i].ativa = false;
                    }
                }
            }

            // --- Lógica de Spawn de Inimigos ---
            tempo_para_spawn_inimigo -= 1.0 / 60.0;
            if (tempo_para_spawn_inimigo <= 0) {
                for (int i = 0; i < MAX_INIMIGOS; i++) {
                    if (!inimigos[i].ativa) {
                        // Inimigo nasce fora da tela à direita, com posição no mundo (deslocamento_x da câmera + largura da tela + offset)
                        inimigos[i].x = deslocamento_x + LARGURA + (float)(rand() % 200 + 50);
                        inimigos[i].y = personagem_y_base;
                        inimigos[i].vel_x = inimigo_velocidade_parado_base; // Começa parado ou movendo lentamente para a esquerda
                        inimigos[i].ativa = true;
                        // --- Reset completo no SPAWN ---
                        inimigos[i].frame_atual = 0;
                        inimigos[i].acc_animacao = 0;
                        inimigos[i].estado = INIMIGO_PARADO;
                        inimigos[i].animacao_morte_finalizada = false;
                        inimigos[i].inimigo_pode_dar_dano = true;
                        // --- Fim do reset no SPAWN ---

                        tempo_para_spawn_inimigo = min_intervalo_spawn_inimigo +
                                                    ((float)rand() / RAND_MAX) * (max_intervalo_spawn_inimigo - min_intervalo_spawn_inimigo);
                        fprintf(stderr, "DEBUG: Inimigo %d SPAWNADO em (%.2f, %.2f) com vel_x_base: %.2f. Próximo spawn em: %.2f. Estado: %d\n",
                                    i, inimigos[i].x, inimigos[i].y, inimigos[i].vel_x, tempo_para_spawn_inimigo, inimigos[i].estado);
                        break;
                    }
                }
            }
            // --- Fim da Lógica de Spawn de Inimigos ---

            // --- NOVO: Lógica de Spawn de Notas de Dinheiro ---
            time_to_spawn_money -= 1.0 / 60.0;
            if (time_to_spawn_money <= 0) {
                for (int i = 0; i < MAX_MONEY_NOTES; i++) {
                    if (!money_notes[i].ativa) {
                        int type_index = rand() % num_money_types;
                        money_notes[i].sprite_bitmap = money_sprites[type_index];
                        money_notes[i].value = money_values[type_index];
                        money_notes[i].hitbox_width = al_get_bitmap_width(money_notes[i].sprite_bitmap) * MONEY_NOTE_SCALE;
                        money_notes[i].hitbox_height = al_get_bitmap_height(money_notes[i].sprite_bitmap) * MONEY_NOTE_SCALE;

                        money_notes[i].x = deslocamento_x + LARGURA + (float)(rand() % 400 + 100); // Spawn off-screen right
                        money_notes[i].y = personagem_y_base + (frame_altura * escala_personagens) - money_notes[i].hitbox_height; // No chão
                        money_notes[i].ativa = true;
                        money_notes[i].spawn_time = al_get_time(); // Registra o tempo de spawn
                        time_to_spawn_money = MONEY_NOTE_SPAWN_INTERVAL;
                        fprintf(stderr, "DEBUG: Nota de R$%d SPAWNADA em (%.2f, %.2f).\n", money_notes[i].value, money_notes[i].x, money_notes[i].y);
                        break;
                    }
                }
            }

            // --- Atualiza Posição e Tempo de Vida das Notas de Dinheiro ---
            for (int i = 0; i < MAX_MONEY_NOTES; i++) {
                if (money_notes[i].ativa) {
                    // Notas de dinheiro têm sua posição no mundo (x) e são desenhadas com o offset do scroll.
                    // Não precisam de ajuste aqui, o desenho já compensa pelo scroll.
                    // FIM CORRIGIDO
                    // Despawn se sair da tela à esquerda ou se o tempo de vida expirou
                    if (money_notes[i].x - deslocamento_x < -money_notes[i].hitbox_width ||
                        (al_get_time() - money_notes[i].spawn_time > MONEY_NOTE_LIFETIME)) {
                        money_notes[i].ativa = false;
                        fprintf(stderr, "DEBUG: Nota de dinheiro desativada (fora da tela ou tempo de vida expirou).\n");
                    }
                }
            }
            // --- FIM NOVO ---

            // --- NOVO: Lógica para reciclar objetos do mundo (Obstáculos e NPC) ---
            // Encontra a coordenada X mais distante dos objetos atualmente ativos
            float max_x_existente = deslocamento_x + LARGURA; // Começa na borda direita da tela

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

            // Recicla obstáculos que saíram da tela à esquerda
            for (int i = 0; i < MAX_OBSTACULOS; i++) {
                if (obstaculos[i].ativa && (obstaculos[i].x + obstaculos[i].width) < deslocamento_x - LARGURA / 2) { // Considera um pouco mais para garantir que está fora
                    fprintf(stderr, "DEBUG: Reciclando obstaculo %d.\n", i);
                    int type_index = rand() % num_obstacle_types;
                    obstaculos[i].sprite_bitmap = obstacle_types[type_index].sprite;
                    obstaculos[i].only_crouch_pass = obstacle_types[type_index].crouch_pass;
                    float current_scale = obstacle_types[type_index].scale;
                    obstaculos[i].width = al_get_bitmap_width(obstaculos[i].sprite_bitmap) * current_scale;
                    obstaculos[i].height = al_get_bitmap_height(obstaculos[i].sprite_bitmap) * current_scale;
                    // Reposiciona o obstáculo bem à frente do objeto mais distante
                    obstaculos[i].x = max_x_existente + min_distancia_entre_obstaculos + (float)rand() / RAND_MAX * variacao_distancia_obstaculos;
                    obstaculos[i].y = personagem_y_base + (frame_altura * escala_personagens) - obstaculos[i].height;
                    obstaculos[i].ativa = true; // Garante que está ativo novamente
                }
            }

            // Recicla o traficante se ele sair da tela à esquerda
            // Recicla o traficante se ele sair da tela à esquerda
            if (traficante.ativa && (traficante.x + traficante.largura_sprite * escala_traficante) < deslocamento_x - LARGURA / 2) {
                fprintf(stderr, "DEBUG: Reciclando traficante.\n");

                // --- CORREÇÃO ---
                // Reposiciona o traficante a uma distância mais razoável.
                // Ex: entre 1000 e 2500 pixels à frente do objeto mais distante.
                float distancia_base = 5000.0;
                float variacao_distancia = 1500.0;
                traficante.x = deslocamento_x + distancia_base + (float)rand() / RAND_MAX * variacao_distancia;
                
                // Opcional: Adicione um log para ver a nova posição e ajustar se necessário
                fprintf(stderr, "DEBUG: Traficante reposicionado para x = %.2f\n", traficante.x);
            }

            // Recicla inimigos que saíram da tela à esquerda
            for (int i = 0; i < MAX_INIMIGOS; i++) {
                if (inimigos[i].ativa && (inimigos[i].x + inimigo_largura_sprite_max * escala_personagens) < deslocamento_x - LARGURA / 2) {
                    fprintf(stderr, "DEBUG: Inimigo %d DESATIVADO (saiu da tela à esquerda).\n", i);
                    inimigos[i].ativa = false;
                    inimigos[i].estado = INIMIGO_PARADO; // Reseta estado
                    inimigos[i].vel_x = inimigo_velocidade_parado_base;
                    inimigos[i].animacao_morte_finalizada = false;
                    inimigos[i].frame_atual = 0;
                    inimigos[i].acc_animacao = 0;
                    inimigos[i].inimigo_pode_dar_dano = true;
                }
            }

            // Recicla notas de dinheiro que saíram da tela à esquerda ou expiraram
            for (int i = 0; i < MAX_MONEY_NOTES; i++) {
                if (money_notes[i].ativa && (money_notes[i].x + money_notes[i].hitbox_width) < deslocamento_x - LARGURA / 2) {
                        money_notes[i].ativa = false;
                        fprintf(stderr, "DEBUG: Nota de dinheiro desativada (fora da tela à esquerda).\n");
                }
            }
            // --- FIM DA LÓGICA DE RECICLAGEM ---


            // --- Atualiza Estados, Movimento e Verifica Colisões dos Inimigos ---
            // Calcula a hitbox do protagonista para verificações de colisão
            float prot_hb_x = personagem_x + prot_hitbox_offset_x;
            float prot_hb_y = personagem_y + prot_hitbox_offset_y;
            float prot_hb_w = prot_hitbox_width;

            // Ajusta a altura da hitbox do protagonista se estiver agachado
            float current_prot_hitbox_height = prot_hitbox_height;
            if (agachando) {
                current_prot_hitbox_height = prot_crouch_hitbox_height;
                // Ajusta também a posição Y da hitbox para que ela permaneça no chão
                prot_hb_y = personagem_y + (prot_hitbox_height - prot_crouch_hitbox_height) + prot_hitbox_offset_y;
            } else {
                prot_hb_y = personagem_y + prot_hitbox_offset_y; // Reseta se não estiver agachado
            }

            // Calcula a hitbox de ATAQUE do protagonista para verificações de colisão
            float prot_attack_hb_x = personagem_x + prot_attack_hitbox_offset_x;
            float prot_attack_hb_y = personagem_y + prot_attack_hitbox_offset_y;
            float prot_attack_hb_w = prot_attack_hitbox_width;
            float prot_attack_hb_h = prot_attack_hitbox_height;

            for (int i = 0; i < MAX_INIMIGOS; i++) {
                if (inimigos[i].ativa) {
                    float inimigo_x_antes_movimento = inimigos[i].x;

                    // INIMIGO SE MOVE NO MUNDO: Apenas atualiza sua posição X baseada em sua velocidade
                    inimigos[i].x += inimigos[i].vel_x;

                    // =========================================================================
                    // INÍCIO DA CORREÇÃO PRINCIPAL
                    // =========================================================================

                    // Hitbox do inimigo em coordenadas de TELA (para colisão com protagonista)
                    float inimigo_screen_x = inimigos[i].x - deslocamento_x;
                    float inimigo_screen_hb_x = inimigo_screen_x + inimigos[i].hitbox_offset_x;
                    float inimigo_hb_y = inimigos[i].y + inimigos[i].hitbox_offset_y;
                    float inimigo_hb_w = inimigos[i].hitbox_width;
                    float inimigo_hb_h = inimigos[i].hitbox_height;

                    // --- Colisão: Ataque do Protagonista vs Inimigo ---
                    if (atacando && inimigos[i].estado != INIMIGO_MORRENDO) {
                        // USA HITBOX DE TELA DO INIMIGO
                        if (check_collision(prot_attack_hb_x, prot_attack_hb_y, prot_attack_hb_w, prot_attack_hb_h,
                                            inimigo_screen_hb_x, inimigo_hb_y, inimigo_hb_w, inimigo_hb_h)) {
                            fprintf(stderr, "COLISÃO! Ataque Especial atingiu Inimigo %d! Iniciando animação de morte.\n", i);
                            inimigos[i].estado = INIMIGO_MORRENDO;
                            inimigos[i].vel_x = 0; // Para o inimigo ao morrer
                            inimigos[i].frame_atual = 0;
                            inimigos[i].acc_animacao = 0;
                            inimigos[i].animacao_morte_finalizada = false;
                        }
                    }
                    // --- FIM Colisão: Ataque do Protagonista vs Inimigo ---


                    // --- Colisão com Protagonista: Lógica de Bloqueio e Ataque ---
                    if (inimigos[i].estado != INIMIGO_MORRENDO && protagonista_estado == PROT_NORMAL) {
                        // USA HITBOX DE TELA DO INIMIGO para verificar colisão
                        if (check_collision(inimigo_screen_hb_x, inimigo_hb_y, inimigo_hb_w, inimigo_hb_h,
                                            prot_hb_x, prot_hb_y, prot_hb_w, current_prot_hitbox_height)) {

                            // Reverte o movimento do MUNDO se o inimigo colidiu com o protagonista (para impedir atravessar)
                            deslocamento_x = old_deslocamento_x_prot;
                            inimigos[i].x = inimigo_x_antes_movimento; // Reverte o movimento do próprio inimigo também

                            // Recalcula delta_deslocamento_x_anterior após o rollback
                            delta_deslocamento_x = deslocamento_x - deslocamento_x_anterior;
                            deslocamento_x_anterior = deslocamento_x;

                            // APLICAR DANO AQUI: SE o inimigo está ATACANDO E PODE DAR DANO E protagonista NÃO está invulnerável
                            // Esta checagem agora funciona porque a colisão é detectada corretamente.
                            if (inimigos[i].estado == INIMIGO_ATACANDO && inimigos[i].inimigo_pode_dar_dano && !protagonist_invulnerable) {
                                protagonist_health -= PROTAGONIST_DAMAGE_PER_HIT;
                                inimigos[i].inimigo_pode_dar_dano = false; // Aplica o dano UMA VEZ por este ataque
                                fprintf(stderr, "PROTAGONISTA SOFREU DANO! Vida restante: %d\n", protagonist_health);

                                // Ativa invulnerabilidade para o protagonista
                                protagonist_invulnerable = true;
                                protagonist_invulnerability_timer = PROTAGONIST_INVULNERABILITY_DURATION;
                                protagonist_blink_timer = 0.0;
                                protagonist_visible = false; // Começa a piscar

                                if (protagonist_health <= 0) {
                                    protagonista_estado = PROT_MORRENDO;
                                    personagem_frame_morte = 0;
                                    personagem_acc_morte = 0;
                                    personagem_morte_animacao_finalizada = false;
                                    fprintf(stderr, "PROTAGONISTA MORREU! Iniciando animação de morte do protagonista.\n");
                                }
                            }
                        }
                    }
                    // --- FIM Colisão com Protagonista ---

                    // =========================================================================
                    // FIM DA CORREÇÃO PRINCIPAL
                    // =========================================================================

                    // Hitbox do inimigo em coordenadas de MUNDO (para colisões com outros inimigos)
                    // MOVIDO PARA CÁ: Esta declaração e inicialização deve ser feita para cada inimigo[i]
                    // antes de verificar colisões com outros inimigos[k].
                    float inimigo_world_hb_x = inimigos[i].x + inimigos[i].hitbox_offset_x;

                    // --- Lógica de Bloqueio (Não atravessar outros inimigos) ---
                    if (inimigos[i].estado != INIMIGO_MORRENDO) {
                        for (int k = 0; k < MAX_INIMIGOS; k++) {
                            if (i == k) continue; // Não verifica colisão consigo mesmo
                            if (inimigos[k].ativa && inimigos[k].estado != INIMIGO_MORRENDO) {
                                // USA HITBOX DE MUNDO para colisão inimigo-inimigo
                                float other_inimigo_world_hb_x = inimigos[k].x + inimigos[k].hitbox_offset_x;
                                float other_inimigo_hb_y = inimigos[k].y + inimigos[k].hitbox_offset_y;
                                float other_inimigo_hb_w = inimigos[k].hitbox_width;
                                float other_inimigo_hb_h = inimigos[k].hitbox_height;
                                // REMOVIDO: float inimigo_world_hb_x = inimigos[i].x + inimigos[i].hitbox_offset_x; // Esta linha estava errada aqui!

                                if (check_collision(inimigo_world_hb_x, inimigo_hb_y, inimigo_hb_w, inimigo_hb_h,
                                                    other_inimigo_world_hb_x, other_inimigo_hb_y, other_inimigo_hb_w, other_inimigo_hb_h)) {
                                    float overlap_amount = 0;
                                    if (inimigo_world_hb_x < other_inimigo_world_hb_x) {
                                        overlap_amount = (inimigo_world_hb_x + inimigo_hb_w) - other_inimigo_world_hb_x;
                                        inimigos[i].x -= overlap_amount; // Move para trás
                                    } else {
                                        overlap_amount = (other_inimigo_world_hb_x + other_inimigo_hb_w) - inimigo_world_hb_x;
                                        inimigos[i].x += overlap_amount; // Move para frente
                                    }
                                    // Após o ajuste, o inimigo_world_hb_x precisa ser atualizado para a próxima verificação dentro do loop k.
                                    inimigo_world_hb_x = inimigos[i].x + inimigos[i].hitbox_offset_x; 
                                }
                            }
                        }
                    }
                    // --- FIM Lógica de Bloqueio entre Inimigos ---


                    // Calcula a distância para o protagonista (para detecção e perseguição)
                    // Esta lógica continua usando coordenadas de MUNDO, o que está CORRETO.
                    float prot_world_x_center_hb = (personagem_x + prot_hitbox_offset_x + prot_hitbox_width / 2.0) + deslocamento_x;
                    float inimigo_world_x_center_hb = inimigos[i].x + inimigos[i].hitbox_offset_x + inimigos[i].hitbox_width / 2.0; // Adicionado offset e largura
                    float dist_to_protagonist_center_x = fabs( inimigo_world_x_center_hb - prot_world_x_center_hb);

                    // --- Lógica de Transição de Estado do Inimigo (Corrigida e Prioritária) ---
                    if (inimigos[i].estado != INIMIGO_MORRENDO) {
                        // PRIORIDADE 1: ATACAR se estiver no alcance de ataque
                        if (dist_to_protagonist_center_x <= inimigo_distancia_ataque) {
                            if (inimigos[i].estado != INIMIGO_ATACANDO) {
                                fprintf(stderr, "DEBUG: Inimigo %d no alcance de ataque. Mudando para ATACANDO.\n", i);
                                inimigos[i].estado = INIMIGO_ATACANDO;
                                inimigos[i].vel_x = inimigo_velocidade_ataque_base; // Para ao atacar
                                inimigos[i].frame_atual = 0; // Reinicia animação de ataque
                                inimigos[i].acc_animacao = 0;
                                inimigos[i].inimigo_pode_dar_dano = true; // Permite dar dano ao iniciar a animação
                            }
                        }
                        // PRIORIDADE 2: ANDAR se estiver no alcance de detecção (mas fora do ataque)
                        else if (dist_to_protagonist_center_x < inimigo_distancia_deteccao) {
                            if (inimigos[i].estado != INIMIGO_ANDANDO) {
                                fprintf(stderr, "DEBUG: Inimigo %d detectou. Mudando para ANDANDO.\n", i);
                                inimigos[i].estado = INIMIGO_ANDANDO;
                                inimigos[i].vel_x = inimigo_velocidade_andando_base; // Inimigo anda
                                inimigos[i].frame_atual = 0; // Reinicia animação de andar
                                inimigos[i].acc_animacao = 0;
                                // inimigo_pode_dar_dano não precisa ser resetado aqui, só no ataque
                            }
                        }
                        // PRIORIDADE 3: PARADO se estiver fora de ambos os alcances
                        else {
                            if (inimigos[i].estado != INIMIGO_PARADO) {
                                fprintf(stderr, "DEBUG: Inimigo %d saiu do alcance de detecção. Voltando para PARADO.\n", i);
                                inimigos[i].estado = INIMIGO_PARADO;
                                inimigos[i].vel_x = inimigo_velocidade_parado_base; // Inimigo para
                                inimigos[i].frame_atual = 0; // Reinicia animação de parado
                                inimigos[i].acc_animacao = 0;
                                // inimigo_pode_dar_dano não precisa ser resetado aqui, só no ataque
                            }
                        }
                    }
                    // --- FIM Lógica de Transição de Estado do Inimigo ---

                    // Atualiza a animação do inimigo com base no estado
                    float tpf_current_inimigo;
                    int frame_total_current_inimigo;

                    if (inimigos[i].estado == INIMIGO_ANDANDO) {
                        tpf_current_inimigo = tpf_inimigo_andando;
                        frame_total_current_inimigo = frame_total_inimigo_andando;
                    } else if (inimigos[i].estado == INIMIGO_MORRENDO) {
                        tpf_current_inimigo = tpf_inimigo_morte;
                        frame_total_current_inimigo = frame_total_inimigo_morte;
                    } else if (inimigos[i].estado == INIMIGO_ATACANDO) {
                        tpf_current_inimigo = tpf_inimigo_ataque;
                        frame_total_current_inimigo = frame_total_inimigo_ataque;
                    } else { // INIMIGO_PARADO
                        tpf_current_inimigo = tpf_inimigo_parado;
                        frame_total_current_inimigo = frame_total_inimigo_parado;
                    }

                    inimigos[i].acc_animacao += 1.0 / 60.0;
                    if (inimigos[i].acc_animacao >= tpf_current_inimigo) {
                        inimigos[i].acc_animacao -= tpf_current_inimigo;
                        if (inimigos[i].frame_atual < frame_total_current_inimigo - 1) {
                            inimigos[i].frame_atual++;
                        } else {
                            if (inimigos[i].estado == INIMIGO_ATACANDO) {
                                inimigos[i].frame_atual = 0; // Repete animação de ataque
                                inimigos[i].inimigo_pode_dar_dano = true; // Reinicia a habilidade de dar dano para o próximo loop de ataque
                            } else if (inimigos[i].estado == INIMIGO_MORRENDO) {
                                inimigos[i].animacao_morte_finalizada = true; // Animação de morte concluída
                            } else {
                                inimigos[i].frame_atual = 0; // Repete outras animações (PARADO, ANDANDO)
                            }
                        }
                    }

                    // Detecção de colisão (Garrafa vs Inimigo)
                    if (inimigos[i].estado != INIMIGO_MORRENDO) {
                        for (int j = 0; j < MAX_GARRAFAS; j++) {
                            if (garrafas[j].ativa) {
                                // Ambas as hitboxes estão em coordenadas de MUNDO, então esta colisão está CORRETA.
                                float garrafa_hb_x = garrafas[j].x - garrafa_hitbox_width / 2.0;
                                float garrafa_hb_y = garrafas[j].y - garrafa_hitbox_height / 2.0;

                                if (check_collision(garrafa_hb_x, garrafa_hb_y, garrafa_hitbox_width, garrafa_hitbox_height,
                                                    inimigos[i].x + inimigos[i].hitbox_offset_x, inimigo_hb_y, inimigo_hb_w, inimigo_hb_h)) {
                                    fprintf(stderr, "COLISÃO! Garrafa %d atingiu Inimigo %d! Iniciando animação de morte.\n", j, i);
                                    garrafas[j].ativa = false; // Garrafa é destruída

                                    inimigos[i].estado = INIMIGO_MORRENDO;
                                    inimigos[i].vel_x = 0;
                                    inimigos[i].frame_atual = 0;
                                    inimigos[i].acc_animacao = 0;
                                    inimigos[i].animacao_morte_finalizada = false;
                                    break; // Sai do loop de garrafas, pois uma já atingiu este inimigo
                                }
                            }
                        }
                    }

                    // Lógica de desativação para inimigos
                    if (inimigos[i].animacao_morte_finalizada) {
                                inimigos[i].ativa = false;
                    }
                }
            }

            // --- Lógica de Colisão Protagonista-Obstáculo (CORRIGIDO) ---
            if (protagonista_estado == PROT_NORMAL) {
                // Rastreia se o protagonista está em cima de um obstáculo
                bool on_obstacle = false;

                // A hitbox do protagonista já foi calculada (prot_hb_x, prot_hb_y, etc.)
                // Vamos usá-la para verificar as colisões.
                float prot_current_hb_x = personagem_x + prot_hitbox_offset_x;
                float prot_current_hb_y = prot_hb_y; // prot_hb_y já considera se o personagem está agachado
                float prot_current_hb_w = prot_hitbox_width;
                float prot_current_hb_h = current_prot_hitbox_height;

                // Loop único para verificar todos os tipos de colisão com obstáculos
                for (int i = 0; i < MAX_OBSTACULOS; i++) {
                    if (obstaculos[i].ativa) {
                        // Calcula as coordenadas de tela do obstáculo com o movimento JÁ APLICADO
                        float obstacle_screen_x = obstaculos[i].x - deslocamento_x;
                        float obstacle_top = obstaculos[i].y;
                        float obstacle_width = obstaculos[i].width;
                        float obstacle_height = obstaculos[i].height;

                        // --- 1. VERIFICAÇÃO DE COLISÃO GERAL ---
                        if (check_collision(prot_current_hb_x, prot_current_hb_y, prot_current_hb_w, prot_current_hb_h,
                                            obstacle_screen_x, obstacle_top, obstacle_width, obstacle_height)) {

                            // Se colidiu, decidimos o que fazer.
                            bool should_block_horizontally = !pulando && !(obstaculos[i].only_crouch_pass && agachando);

                            if (should_block_horizontally) {
                                // É uma colisão de bloqueio horizontal. Reverte o movimento.
                                deslocamento_x = old_deslocamento_x_prot;
                                // Como o movimento foi revertido, saímos do loop de obstáculos para esta iteração.
                                // A posição será reavaliada no próximo quadro.
                                break;
                            }
                        }
                    }
                }


                // --- 2. Lógica de Aterrisagem e Manutenção no Obstáculo ---
                // Este loop é necessário para garantir que o personagem pouse ou permaneça em cima de plataformas.
                // Usamos a posição do personagem (potencialmente já corrigida pela colisão horizontal)
                for (int i = 0; i < MAX_OBSTACULOS; i++) {
                    if (obstaculos[i].ativa) {
                        // Recalcula a posição do obstáculo, pois o deslocamento_x pode ter sido revertido
                        float obstacle_screen_x = obstaculos[i].x - deslocamento_x;
                        float obstacle_top = obstaculos[i].y;

                        // A. Lógica de Pouso (Vertical): Verifica se o personagem estava acima e agora cruzou o topo do obstáculo.
                        // Esta é uma verificação preditiva, usando a posição Y antiga.
                        if (pulando && vel_y > 0 &&
                            (old_personagem_y + prot_hitbox_offset_y + prot_current_hb_h) <= obstacle_top && // Estava acima
                            (personagem_y + prot_hitbox_offset_y + prot_current_hb_h) > obstacle_top &&      // Agora está abaixo
                            (prot_current_hb_x + prot_current_hb_w > obstacle_screen_x) &&                  // Sobreposição horizontal
                            (prot_current_hb_x < obstacle_screen_x + obstaculos[i].width)) {

                            // Não se pode pousar em obstáculos que são para passar agachado
                            if (!obstaculos[i].only_crouch_pass) {
                                personagem_y = obstacle_top - prot_hitbox_offset_y - prot_current_hb_h; // Pousa no topo
                                pulando = false;
                                vel_y = 0;
                                vel_x_pulo = 0;
                                current_ground_y = personagem_y;
                                on_obstacle = true;
                                break; // Pousou em um obstáculo, não precisa verificar outros para pouso.
                            }
                        }

                        // B. Lógica de Manutenção: Verifica se o personagem já está em cima do obstáculo.
                        if (!pulando && !on_obstacle &&
                            (personagem_y + prot_hitbox_offset_y + prot_current_hb_h > obstacle_top - 5) &&
                            (personagem_y + prot_hitbox_offset_y + prot_current_hb_h < obstacle_top + 5) &&
                            (prot_current_hb_x + prot_current_hb_w > obstacle_screen_x) &&
                            (prot_current_hb_x < obstacle_screen_x + obstaculos[i].width)) {

                            if (!obstaculos[i].only_crouch_pass) {
                                on_obstacle = true;
                                current_ground_y = personagem_y; // O chão atual é a posição Y atual (no obstáculo)
                            }
                        }
                    }
                }

                // Se, após todas as verificações, o personagem não está pulando e nem sobre um obstáculo,
                // ele deve cair em direção ao chão principal.
                if (!on_obstacle && !pulando && personagem_y < personagem_y_base) {
                    current_ground_y = personagem_y_base;
                }
            }
            // --- FIM DA LÓGICA DE COLISÃO ---

            // --- NOVO: Lógica de Coleta de Notas de Dinheiro ---
            if (protagonista_estado == PROT_NORMAL) {
                for (int i = 0; i < MAX_MONEY_NOTES; i++) {
                    if (money_notes[i].ativa) {
                        float money_note_hb_x = money_notes[i].x - deslocamento_x; // Posição de tela da nota
                        float money_note_hb_y = money_notes[i].y;
                        float money_note_hb_w = money_notes[i].hitbox_width;
                        float money_note_hb_h = money_notes[i].hitbox_height;

                        if (check_collision(prot_hb_x, prot_hb_y, prot_hb_w, current_prot_hitbox_height,
                                            money_note_hb_x, money_note_hb_y, money_note_hb_w, money_note_hb_h)) {
                            player_money += money_notes[i].value;
                            money_notes[i].ativa = false; // Desativa a nota após a coleta
                            fprintf(stderr, "DEBUG: Coletou R$%d! Total: R$%d\n", money_notes[i].value, player_money);
                        }
                    }
                }
            }
            // --- FIM NOVO ---

            al_clear_to_color(al_map_rgb(0, 0, 0)); // Limpa a tela

            // --- Lógica de Desenho do Background para Tiling Infinito ---
            // CORRIGIDO: Cálculo do offset para tiling do background.
            // O fmod garante que o offset_x_bg esteja sempre dentro dos limites da largura do background.
            int offset_x_bg = (int)fmod(deslocamento_x, bg_width);
            if (offset_x_bg < 0) offset_x_bg += bg_width; // Garante offset positivo

            // Desenha a primeira parte do background
            al_draw_scaled_bitmap(background,
                                 offset_x_bg, 0, // Fonte X na imagem (começa do offset)
                                 bg_width - offset_x_bg, bg_height, // Largura da parte a ser desenhada
                                 0, 0, // Destino X, Y na tela
                                 (float)(bg_width - offset_x_bg), (float)ALTURA, 0); // Largura, Altura na tela (escalado)

            // Desenha a segunda parte do background (para tiling) se necessário
            if ((bg_width - offset_x_bg) < LARGURA) {
                al_draw_scaled_bitmap(background,
                                     0, 0, // Fonte X na imagem (começa do zero)
                                     LARGURA - (bg_width - offset_x_bg), bg_height, // Largura da parte a ser desenhada
                                     (float)(bg_width - offset_x_bg), 0, // Destino X, Y na tela
                                     (float)(LARGURA - (bg_width - offset_x_bg)), (float)ALTURA, 0); // Largura, Altura na tela (escalado)
            }
            // --- FIM DA LÓGICA DE DESENHO DO BACKGROUND ---


            // --- Desenha Obstáculos ---
            for (int i = 0; i < MAX_OBSTACULOS; i++) {
                if (obstaculos[i].ativa) {
                    float draw_x = obstaculos[i].x - deslocamento_x; // Posição de tela
                    // Desenha usando o campo sprite_bitmap
                    al_draw_scaled_bitmap(obstaculos[i].sprite_bitmap,
                                          0, 0,
                                          al_get_bitmap_width(obstaculos[i].sprite_bitmap), al_get_bitmap_height(obstaculos[i].sprite_bitmap),
                                          draw_x, obstaculos[i].y, // O Y do obstáculo é o topo da sua hitbox, que alinha com a parte inferior visual
                                          obstaculos[i].width, obstaculos[i].height,
                                          0);
                }
            }
            // --- FIM NOVO ---

            // --- Desenha NPC ---
            if (traficante.ativa) {
                float draw_x = traficante.x - deslocamento_x; // Posição de tela
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
            // --- FIM NOVO ---

            // --- DESENHA ANIMAÇÃO DO PERSONAGEM ---
            if (protagonista_estado == PROT_MORRENDO) {
                al_draw_scaled_bitmap(sprite_personagem_morte,
                                      personagem_frame_morte * frame_largura_personagem_morte, 0,
                                      frame_largura_personagem_morte, frame_altura,
                                      personagem_x, personagem_y,
                                      frame_largura_personagem_morte * escala_personagens, frame_altura * escala_personagens, 0);
            } else if (protagonist_visible) { // Só desenha se o protagonista estiver visível (para efeito de piscar)
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
            // --- FIM DESENHO ANIMAÇÃO DO PERSONAGEM ---

            // --- Desenha Garrafas ---
            for (int i = 0; i < MAX_GARRAFAS; i++) {
                if (garrafas[i].ativa) {
                    al_draw_scaled_rotated_bitmap(sprite_garrafa,
                                                  garrafa_largura_original / 2.0, garrafa_altura_original / 2.0,
                                                  garrafas[i].x - deslocamento_x, garrafas[i].y, // Posição de tela da garrafa
                                                  escala_garrafa, escala_garrafa,
                                                  garrafas[i].angulo,
                                                  0);
                }
            }

            // --- Desenha Inimigos ---
            for (int i = 0; i < MAX_INIMIGOS; i++) {
                if (inimigos[i].ativa) {
                    ALLEGRO_BITMAP *current_enemy_sprite = NULL;
                    int current_frame_largura_inimigo = 0;

                    // Atribuição do sprite correto e largura do frame para o estado atual
                    if (inimigos[i].estado == INIMIGO_ANDANDO) {
                        current_enemy_sprite = sprite_inimigo_andando;
                        current_frame_largura_inimigo = frame_largura_inimigo_andando;
                    } else if (inimigos[i].estado == INIMIGO_MORRENDO) {
                        current_enemy_sprite = sprite_inimigo_morte;
                        current_frame_largura_inimigo = frame_largura_inimigo_morte;
                    } else if (inimigos[i].estado == INIMIGO_ATACANDO) {
                        current_enemy_sprite = sprite_inimigo_ataque;
                        current_frame_largura_inimigo = frame_largura_inimigo_ataque;
                    } else { // INIMIGO_PARADO
                        current_enemy_sprite = sprite_inimigo_parado;
                        current_frame_largura_inimigo = frame_largura_inimigo_parado;
                    }

                    al_draw_scaled_bitmap(current_enemy_sprite,
                                          inimigos[i].frame_atual * current_frame_largura_inimigo, 0,
                                          current_frame_largura_inimigo, inimigo_altura_sprite,
                                          inimigos[i].x - deslocamento_x, inimigos[i].y, // Posição de tela do inimigo
                                          current_frame_largura_inimigo * escala_personagens, inimigo_altura_sprite * escala_personagens,
                                          ALLEGRO_FLIP_HORIZONTAL); // Inverte horizontalmente para facear o protagonista
                }
            }

            // --- Desenha Notas de Dinheiro ---
            for (int i = 0; i < MAX_MONEY_NOTES; i++) {
                if (money_notes[i].ativa) {
                    al_draw_scaled_bitmap(money_notes[i].sprite_bitmap,
                                          0, 0,
                                          al_get_bitmap_width(money_notes[i].sprite_bitmap), al_get_bitmap_height(money_notes[i].sprite_bitmap),
                                          money_notes[i].x - deslocamento_x, money_notes[i].y, // Posição de tela
                                          money_notes[i].hitbox_width, money_notes[i].hitbox_height,
                                          0);
                }
            }
            // --- FIM NOVO ---

            // --- Desenha Barra de Vida do Protagonista ---
            float health_bar_width = (float)protagonist_health / (MAX_PROTAGONIST_HEALTH + NPC_HEAL_AMOUNT) * 200; // Largura máxima ajustada
            if (health_bar_width < 0) health_bar_width = 0;
            // Altera a cor para indicar "over-heal" se protagonist_health > MAX_PROTAGONIST_HEALTH
            ALLEGRO_COLOR health_color = al_map_rgb(0, 255, 0); // Verde
            if (protagonist_health > MAX_PROTAGONIST_HEALTH) {
                health_color = al_map_rgb(0, 255, 255); // Ciano para over-heal
            }
            al_draw_filled_rectangle(10, 10, 10 + health_bar_width, 30, health_color);
            al_draw_rectangle(10, 10, 10 + 200, 30, al_map_rgb(255, 255, 255), 2);
            al_draw_textf(fonte, al_map_rgb(255, 255, 255), 220, 15, 0, "HP: %d", protagonist_health);
            // --- FIM NOVO ---

            // --- NOVO: Desenha Dinheiro do Jogador ---
            al_draw_textf(fonte, al_map_rgb(255, 255, 0), LARGURA - 10, 10, ALLEGRO_ALIGN_RIGHT, "R$ %d", player_money);
            // --- FIM NOVO ---

            al_flip_display(); // Atualiza a tela
        }
    }

    // --- Limpeza de Recursos (Liberação de Memória) ---
    // --- Destrói Ativos de Áudio ---
    if (instancia_som_verdebalaraio) {
        al_stop_sample_instance(instancia_som_verdebalaraio);
        al_destroy_sample_instance(instancia_som_verdebalaraio);
    }
    if (som_verdebalaraio) al_destroy_sample(som_verdebalaraio);
    // --- FIM NOVO ---

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
    al_destroy_bitmap(sprite_placa_radar); // Destrói novo sprite de obstáculo
    al_destroy_bitmap(sprite_2reais); // Destrói notas de dinheiro
    al_destroy_bitmap(sprite_5reais); // Destrói notas de dinheiro
    al_destroy_bitmap(sprite_10reais); // Destrói notas de dinheiro
    al_destroy_timer(timer);

    if (protagonista_estado == PROT_MORRENDO && personagem_morte_animacao_finalizada) {
        should_restart = game_over_screen(janela, fila, fonte);
    }
    
    return should_restart; // Retorna se o jogo deve ser reiniciado
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