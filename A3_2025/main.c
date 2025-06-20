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
#include <time.h>

#define LARGURA 1536
#define ALTURA 864

#define NUM_OPCOES 3

const int VIDEO_LARGURA = 1920;
const int VIDEO_ALTURA = 1080;

// --- Structure to hold bottle properties ---
typedef struct {
    float x, y;
    float vel_x;
    float angulo;
    bool ativa;
} Garrafa;


// --- Define maximum number of bottles ---
#define MAX_GARRAFAS 5 // You can adjust this number


// --- Enemy States Enum ---
typedef enum {
    INIMIGO_PARADO,
    INIMIGO_ANDANDO,
    INIMIGO_ATACANDO, // NEW: State for attacking
    INIMIGO_MORRENDO
} EnemyState;

typedef enum {
    PROT_NORMAL,    // Estado normal (parado, andando, pulando, agachando, atacando, arremessando)
    PROT_MORRENDO    // Estado de morte do protagonista
} ProtagonistState;

// Structure to hold enemy properties
typedef struct {
    float x, y;    // Posição do inimigo na TELA
    float vel_x;    // Velocidade horizontal RELATIVA À TELA
    int frame_atual;
    float acc_animacao;
    bool ativa;
    EnemyState estado;
    bool animacao_morte_finalizada; // NEW: Flag to know if death animation completed
    float hitbox_offset_x;
    float hitbox_offset_y;
    float hitbox_width;
    float hitbox_height;
    bool inimigo_pode_dar_dano;
} Inimigo;

// --- Define maximum number of enemies ---
#define MAX_INIMIGOS 3 // You can adjust this number

// --- Protagonist Health Definitions ---
#define MAX_PROTAGONIST_HEALTH 100
#define PROTAGONIST_DAMAGE_PER_HIT 20 // Amount of health lost per hit
// --- Invulnerability Definitions ---
#define PROTAGONIST_INVULNERABILITY_DURATION 1.0 // 1 second
#define PROTAGONIST_BLINK_INTERVAL 0.1 // Blink every 0.1 seconds

// --- NPC Structure ---
typedef struct {
    float x, y;
    int frame_atual;
    float acc_animacao;
    bool ativa; // Always true for a permanent NPC
    int largura_sprite;
    int altura_sprite;
} NPC;

// --- NPC Definitions ---
#define NPC_TRAFICANTE_FRAME_COUNT 7
#define NPC_TRAFICANTE_TPF (1.0 / 8.0) // Animation speed for the NPC
#define NPC_HEAL_AMOUNT 20 // Amount of HP restored
#define NPC_INTERACTION_DISTANCE 100.0 // Distance to interact with the NPC
#define NPC_HEAL_COST 20 // Cost to buy from NPC

// --- Audio Fade Definitions ---
#define AUDIO_MAX_DISTANCE 600.0 // Max distance in pixels for sound to be audible
#define AUDIO_MIN_DISTANCE 50.0  // Min distance in pixels for full volume

// --- UPDATED: Obstacle Structure to include sprite bitmap ---
typedef struct {
    float x, y; // World coordinates
    float width, height; // Hitbox dimensions
    bool ativa;
    ALLEGRO_BITMAP *sprite_bitmap; // Pointer to the actual bitmap
    bool only_crouch_pass; // NEW: Flag for obstacles that can only be passed while crouching
} Obstaculo;

// --- UPDATED: Define maximum number of obstacles ---
#define MAX_OBSTACULOS 4 // Increased to 4 for the new obstacle

// --- Money Note Structure ---
typedef struct {
    float x, y;
    int value; // 2, 5, or 10
    ALLEGRO_BITMAP *sprite_bitmap;
    bool ativa;
    float hitbox_width;
    float hitbox_height;
} MoneyNote;

// --- Money Note Definitions ---
#define MAX_MONEY_NOTES 5
#define MONEY_NOTE_SPAWN_INTERVAL 5.0 // Time in seconds between spawns
#define MONEY_NOTE_LIFETIME 10.0 // Time in seconds before a note despawns
#define MONEY_NOTE_SCALE 0.3 // Scale for money note sprites

void esperar_tecla(ALLEGRO_EVENT_QUEUE *fila) {
    ALLEGRO_EVENT ev;
    while (1) {
        al_wait_for_event(fila, &ev);
        if (ev.type == ALLEGRO_EVENT_KEY_DOWN) break;
    }
}

// Função para desenhar o vídeo redimensionado e centralizado mantendo a proporção
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

bool check_collision(float x1, float y1, float w1, float h1,float x2, float y2, float w2, float h2) {

    return x1 < x2 + w2 &&

           x1 + w1 > x2 &&

           y1 < y2 + h2 &&

           y1 + h1 > y2;

}

void jogo(ALLEGRO_DISPLAY *janela, ALLEGRO_EVENT_QUEUE *fila, ALLEGRO_FONT *fonte) {
    fprintf(stderr, "DEBUG: Início da função jogo.\n");

    float deslocamento_x = 0.0;
    float velocidade_andar = 3.0;
    float velocidade_correr = 6.0;
    float velocidade = velocidade_andar;
    float escala_personagens = 3.0;
    float escala_traficante = 1.7; // Specific scale for the Traficante NPC

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
    ALLEGRO_BITMAP *sprite_placa_radar = al_load_bitmap("placa_radar.png"); // NEW: Load new obstacle sprite

    // --- NEW: Load Money Note Sprites ---
    ALLEGRO_BITMAP *sprite_2reais = al_load_bitmap("2reais.png");
    ALLEGRO_BITMAP *sprite_5reais = al_load_bitmap("5reais.png");
    ALLEGRO_BITMAP *sprite_10reais = al_load_bitmap("10reais.png");
    // --- END NEW ---

    // --- Load Audio Sample ---
    ALLEGRO_SAMPLE *som_verdebalaraio = al_load_sample("verdebalaraio.ogg");
    ALLEGRO_SAMPLE_INSTANCE *instancia_som_verdebalaraio = NULL;

    if (!background || !sprite_parado || !sprite_andando || !sprite_correndo || !sprite_pulando ||
        !sprite_agachado || !sprite_especial || !sprite_ataque1 || !sprite_arremesso || !sprite_garrafa ||
        !sprite_inimigo_parado || !sprite_inimigo_andando || !sprite_inimigo_morte || !sprite_inimigo_ataque ||
        !sprite_personagem_morte || !sprite_traficante_parada || !som_verdebalaraio || !sprite_sacos_lixo ||
        !sprite_placa_radar || !sprite_2reais || !sprite_5reais || !sprite_10reais) { // Check all new assets including money notes
        fprintf(stderr, "ERRO CRÍTICO: Falha ao carregar um ou mais bitmaps/áudios! Verifique nomes/caminhos dos arquivos.\n");
        // Detailed error messages for debugging
        if (!background) fprintf(stderr, "Falha ao carregar background.png\n");
        if (!sprite_parado) fprintf(stderr, "Falha ao carregar personagem.png\n");
        if (!sprite_traficante_parada) fprintf(stderr, "Falha ao carregar traficante_parada.png\n");
        if (!som_verdebalaraio) fprintf(stderr, "Falha ao carregar verdebalaraio.ogg\n");
        if (!sprite_sacos_lixo) fprintf(stderr, "Falha ao carregar sacos_lixo.png\n");
        if (!sprite_placa_radar) fprintf(stderr, "Falha ao carregar placa_radar.png\n"); // NEW: Error for new obstacle sprite
        if (!sprite_2reais) fprintf(stderr, "Falha ao carregar 2reais.png\n"); // NEW: Error for new money note
        if (!sprite_5reais) fprintf(stderr, "Falha ao carregar 5reais.png\n"); // NEW: Error for new money note
        if (!sprite_10reais) fprintf(stderr, "Falha ao carregar 10reais.png\n"); // NEW: Error for new money note

        // Cleanup any loaded assets before returning
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
        if (sprite_placa_radar) al_destroy_bitmap(sprite_placa_radar); // NEW: Destroy new obstacle sprite
        if (sprite_2reais) al_destroy_bitmap(sprite_2reais); // NEW: Destroy new money note
        if (sprite_5reais) al_destroy_bitmap(sprite_5reais); // NEW: Destroy new money note
        if (sprite_10reais) al_destroy_bitmap(sprite_10reais); // NEW: Destroy new money note
        return;
    }
    fprintf(stderr, "DEBUG: Todos os bitmaps e áudios carregados com sucesso.\n");

    // --- Create and attach audio instance ---
    instancia_som_verdebalaraio = al_create_sample_instance(som_verdebalaraio);
    if (!instancia_som_verdebalaraio) {
        fprintf(stderr, "ERRO CRÍTICO: Falha ao criar instancia_som_verdebalaraio!\n");
        // Handle error, destroy loaded assets
        al_destroy_bitmap(background); al_destroy_bitmap(sprite_parado); al_destroy_bitmap(sprite_andando);
        al_destroy_bitmap(sprite_correndo); al_destroy_bitmap(sprite_pulando); al_destroy_bitmap(sprite_agachado);
        al_destroy_bitmap(sprite_especial); al_destroy_bitmap(sprite_ataque1); al_destroy_bitmap(sprite_arremesso);
        al_destroy_bitmap(sprite_garrafa); al_destroy_bitmap(sprite_inimigo_parado); al_destroy_bitmap(sprite_inimigo_andando);
        al_destroy_bitmap(sprite_inimigo_morte); al_destroy_bitmap(sprite_inimigo_ataque);
        al_destroy_bitmap(sprite_personagem_morte); al_destroy_bitmap(sprite_traficante_parada);
        al_destroy_sample(som_verdebalaraio);
        al_destroy_bitmap(sprite_sacos_lixo);
        al_destroy_bitmap(sprite_placa_radar); // NEW: Destroy new obstacle sprite
        al_destroy_bitmap(sprite_2reais); al_destroy_bitmap(sprite_5reais); al_destroy_bitmap(sprite_10reais); // NEW: Destroy money notes
        return;
    }
    al_set_sample_instance_playmode(instancia_som_verdebalaraio, ALLEGRO_PLAYMODE_LOOP);
    al_attach_sample_instance_to_mixer(instancia_som_verdebalaraio, al_get_default_mixer());
    al_set_sample_instance_gain(instancia_som_verdebalaraio, 0.0); // Start with zero volume
    al_play_sample_instance(instancia_som_verdebalaraio); // Start playing (silently)
    // --- END NEW ---

    int bg_width = al_get_bitmap_width(background);
    int bg_height = al_get_bitmap_height(background);

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
    int frame_altura = 128; // Character sprite height (and enemy sprite height)

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

    // --- Protagonist Hitbox Properties ---
    float prot_hitbox_offset_x = 40.0 * escala_personagens;
    float prot_hitbox_offset_y = 5.0 * escala_personagens;   // Adjusted for feet
    float prot_hitbox_width = (frame_largura_parado - 80) * escala_personagens;
    float prot_hitbox_height = (frame_altura - 10) * escala_personagens; // Adjusted for feet
    // --- End Protagonist Hitbox ---

    // --- Protagonist ATTACK Hitbox Properties ---
    float prot_attack_hitbox_offset_x = 60.0 * escala_personagens;
    float prot_attack_hitbox_offset_y = 50.0 * escala_personagens;
    float prot_attack_hitbox_width = 50 * escala_personagens;
    float prot_attack_hitbox_height = 80.0 * escala_personagens;
    // --- END Protagonist ATTACK Hitbox ---


    int garrafa_largura_original = al_get_bitmap_width(sprite_garrafa);
    int garrafa_altura_original = al_get_bitmap_height(sprite_garrafa);

    float escala_garrafa = 1.0;
    float garrafa_hitbox_scale = 0.8;
    float garrafa_hitbox_width = garrafa_largura_original * escala_garrafa * garrafa_hitbox_scale;
    float garrafa_hitbox_height = garrafa_altura_original * escala_garrafa * garrafa_hitbox_scale;

    // --- Obstacle Properties for different types ---
    // Sacos de lixo (existing)
    float escala_obstaculo_sacos = 0.1;
    int obstaculo_largura_original_sacos = al_get_bitmap_width(sprite_sacos_lixo);
    int obstaculo_altura_original_sacos = al_get_bitmap_height(sprite_sacos_lixo);
    float obstaculo_width_sacos = obstaculo_largura_original_sacos * escala_obstaculo_sacos;
    float obstaculo_height_sacos = obstaculo_altura_original_sacos * escala_obstaculo_sacos;

    // NOVO: Placa de Radar properties
    float escala_placa_radar = 0.3; // Adjust as needed for its size
    int placa_radar_largura_original = al_get_bitmap_width(sprite_placa_radar);
    int placa_radar_altura_original = al_get_bitmap_height(sprite_placa_radar);
    float placa_radar_width = placa_radar_largura_original * escala_placa_radar;
    float placa_radar_height = placa_radar_altura_original * escala_placa_radar;


    float personagem_x = LARGURA / 2.0 - (frame_largura_correndo * escala_personagens) / 2.0;
    float personagem_y_base = (ALTURA - 300) - (frame_altura * escala_personagens) / 2.0;
    float personagem_y = personagem_y_base;


    // For a "pass-through-crouching" obstacle, its hitbox height should be above the crouch height
    // and its Y position should align its bottom with the ground.
    // Assuming the main ground level is 'personagem_y_base + (frame_altura * escala_personagens)'
    float placa_radar_y_base = personagem_y_base + (frame_altura * escala_personagens) - placa_radar_height;

    // Protagonist Crouch Hitbox Height (Approximation)
    // You might want to define a specific hitbox height for crouching if the sprite changes significantly
    float prot_crouch_hitbox_height = (frame_altura - 50) * escala_personagens; // Example: 50 pixels shorter when crouching
    // --- END Obstacle Properties ---

    int frame_parado = 0, frame_andando = 0, frame_correndo = 0, frame_pulando = 0, frame_agachado = 0, frame_especial = 0, frame_ataque1 = 0;
    int frame_arremesso = 0;
    // --- Protagonist Death Animation Variables ---
    int personagem_frame_morte = 0;
    float personagem_acc_morte = 0;
    float tpf_personagem_morte = 1.0 / 8.0; // Speed of protagonist death animation
    bool personagem_morte_animacao_finalizada = false;
    // --- END NEW ---


    float tpf_parado = 1.0 / 5.0, tpf_andando = 1.0 / 10.0, tpf_correndo = 1.0 / 10.0, tpf_pulando = 1.0 / 12, tpf_agachado = 1.0 / 8.0;
    float tpf_especial = 1.0 / 15.0;
    float tpf_ataque1 = 1.0 / 10.0;
    float tpf_arremesso = 1.0 / 8.0;
    float tpf_inimigo_parado = 1.0 / 8.0;
    float tpf_inimigo_andando = 1.0 / 10.0;
    float tpf_inimigo_ataque = 1.0 / 8.0;
    float tpf_inimigo_morte = 1.0 / 8.0;

    float acc_parado = 0, acc_andando = 0, acc_correndo = 0, acc_pulando = 0, acc_agachado = 0, acc_especial = 0, acc_ataque1 = 0;
    float acc_arremesso = 0;

    
    ProtagonistState protagonista_estado = PROT_NORMAL;
    // --- Protagonist Health ---
    int protagonist_health = MAX_PROTAGONIST_HEALTH;
    // --- Invulnerability Variables ---
    bool protagonist_invulnerable = false;
    float protagonist_invulnerability_timer = 0.0;
    float protagonist_blink_timer = 0.0;
    bool protagonist_visible = true; // For blinking effect
    // --- END NEW ---

    // --- NEW: Player Money Variable ---
    int player_money = 2; // Starting money
    // --- END NEW ---

    // --- Initialize NPC ---
    NPC traficante;
    traficante.largura_sprite = al_get_bitmap_width(sprite_traficante_parada) / NPC_TRAFICANTE_FRAME_COUNT;
    traficante.altura_sprite = al_get_bitmap_height(sprite_traficante_parada);
    // Randomize Traficante X position, ensuring it's not too close to the start
    traficante.x = LARGURA + (float)(rand() % (bg_width - LARGURA / 2)); // Random X far right
    traficante.y = personagem_y_base - (traficante.altura_sprite * escala_traficante - frame_altura * escala_personagens); // Adjust Y based on scaled height
    traficante.ativa = true;
    traficante.frame_atual = 0;
    traficante.acc_animacao = 0;
    // --- END NEW ---

    bool pulando = false;
    bool agachando = false;
    bool especial_ativo = false;
    bool especial_finalizado = false;
    bool atacando = false;
    bool arremessando = false;
    bool crouch_animation_finished = false; // NOVO: Flag para controlar o fim da animação de agachar

    float vel_y = 0.0;
    float gravidade = 0.5; // Increased gravity for faster falls
    float vel_x_pulo = 0.0;

    // --- NEW: Current ground for protagonist (either base or obstacle top) ---
    float current_ground_y = personagem_y_base;
    // --- END NEW ---

    Garrafa garrafas[MAX_GARRAFAS];
    float garrafa_velocidade_angular = 0.2;

    for (int i = 0; i < MAX_GARRAFAS; i++) {
        garrafas[i].ativa = false;
    }

    Inimigo inimigos[MAX_INIMIGOS];
    float tempo_para_spawn_inimigo = 0.5;
    float min_intervalo_spawn_inimigo = 0.5;
    float max_intervalo_spawn_inimigo = 2.0;

    float inimigo_velocidade_parado_base = -0.5;
    float inimigo_velocidade_andando_base = -2.5;
    float inimigo_velocidade_ataque_base = 0.0;

    float inimigo_distancia_deteccao = 500.0;
    float inimigo_distancia_ataque = 180.0;

    float deslocamento_x_anterior = deslocamento_x;

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

    // --- Initialize obstacles (now with sprite_bitmap and only_crouch_pass) ---
    Obstaculo obstaculos[MAX_OBSTACULOS];
    srand(time(NULL)); // Seed the random number generator

    // Arrays to hold the types of obstacles and their properties for easier randomization
    struct {
        ALLEGRO_BITMAP *sprite;
        float scale;
        bool crouch_pass;
    } obstacle_types[] = {
        {sprite_sacos_lixo, escala_obstaculo_sacos, false},
        {sprite_placa_radar, escala_placa_radar, true}
    };
    int num_obstacle_types = sizeof(obstacle_types) / sizeof(obstacle_types[0]);

    // Minimum distance between obstacles (to prevent immediate overlap)
    float min_obstacle_distance = 200.0; // Adjust as needed

    for (int i = 0; i < MAX_OBSTACULOS; i++) {
        obstaculos[i].ativa = true; // All obstacles start active for placement

        // Randomly choose an obstacle type
        int type_index = rand() % num_obstacle_types;
        obstaculos[i].sprite_bitmap = obstacle_types[type_index].sprite;
        obstaculos[i].only_crouch_pass = obstacle_types[type_index].crouch_pass;

        float current_scale = obstacle_types[type_index].scale;
        int original_width = al_get_bitmap_width(obstaculos[i].sprite_bitmap);
        int original_height = al_get_bitmap_height(obstaculos[i].sprite_bitmap);

        obstaculos[i].width = original_width * current_scale;
        obstaculos[i].height = original_height * current_scale;

        // Randomize X position
        // Start placing obstacles after a certain distance from the player's initial position
        // and within the background width, ensuring space between them.
        float min_x = LARGURA + (i * 500); // Start further out for subsequent obstacles
        float max_x = LARGURA + (i * 500) + 1000; // Spread them out
        if (max_x > bg_width - obstaculos[i].width) { // Don't go off the map
             max_x = bg_width - obstaculos[i].width;
        }
        if (min_x > max_x) min_x = LARGURA + (i * 200); // Ensure min_x is always less than max_x, for smaller maps

        obstaculos[i].x = min_x + (float)rand() / RAND_MAX * (max_x - min_x);

        // Ensure vertical alignment with the ground
        obstaculos[i].y = personagem_y_base + (frame_altura * escala_personagens) - obstaculos[i].height;

        // Adjust Traficante position if it overlaps with an obstacle during initialization
        // This is a simple check; for more complex scenarios, you might need iterative placement.
        if (traficante.ativa) {
            float traficante_hb_x = traficante.x; // Simplified hitbox for placement check
            float traficante_hb_y = traficante.y;
            float traficante_hb_w = traficante.largura_sprite * escala_traficante;
            float traficante_hb_h = traficante.altura_sprite * escala_traficante;

            if (check_collision(obstaculos[i].x, obstaculos[i].y, obstaculos[i].width, obstaculos[i].height,
                                 traficante_hb_x, traficante_hb_y, traficante_hb_w, traficante_hb_h)) {
                // If collision, move the traficante slightly
                traficante.x += obstaculos[i].width + min_obstacle_distance;
                fprintf(stderr, "DEBUG: Traficante moved due to obstacle collision. New X: %.2f\n", traficante.x);
            }
        }
    }
    // --- END Obstacle Initialization ---

    // --- NEW: Initialize Money Notes ---
    MoneyNote money_notes[MAX_MONEY_NOTES];
    ALLEGRO_BITMAP *money_sprites[] = {sprite_2reais, sprite_5reais, sprite_10reais};
    int money_values[] = {2, 5, 10};
    int num_money_types = sizeof(money_sprites) / sizeof(money_sprites[0]);
    float time_to_spawn_money = MONEY_NOTE_SPAWN_INTERVAL;

    for (int i = 0; i < MAX_MONEY_NOTES; i++) {
        money_notes[i].ativa = false;
    }
    // --- END NEW ---


    ALLEGRO_TIMER *timer = al_create_timer(1.0 / 60.0);
    al_start_timer(timer);

    al_register_event_source(fila, al_get_display_event_source(janela));
    al_register_event_source(fila, al_get_keyboard_event_source());
    al_register_event_source(fila, al_get_timer_event_source(timer));

    bool jogando = true;
    // int frame_count = 0; // Not used

    while (jogando) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(fila, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            jogando = false;
        } else if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            // --- Protagonist cannot take actions if dying ---
            if (protagonista_estado == PROT_MORRENDO) {
                if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) jogando = false;
            }
            // --- End Protagonist cannot take actions if dying ---

            if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) { jogando = false; }
            else if (protagonista_estado == PROT_NORMAL) { // Only allow actions if protagonist is normal
                if (ev.keyboard.keycode == ALLEGRO_KEY_UP && !pulando && !agachando && !atacando && !arremessando) {
                    pulando = true; vel_y = -10.0; frame_pulando = 0; acc_pulando = 0; especial_ativo = false; especial_finalizado = false;
                    ALLEGRO_KEYBOARD_STATE estado; al_get_keyboard_state(&estado);
                    if (al_key_down(&estado, ALLEGRO_KEY_RIGHT)) vel_x_pulo = velocidade;
                    else if (al_key_down(&estado, ALLEGRO_KEY_LEFT)) vel_x_pulo = -velocidade;
                    else vel_x_pulo = 0;
                    // When jumping, immediately set ground to base, unless we land on something else
                    current_ground_y = personagem_y_base;
                }
                // When pressing DOWN, go into crouch state
                else if (ev.keyboard.keycode == ALLEGRO_KEY_DOWN && !pulando && !atacando && !arremessando) {
                    agachando = true; frame_agachado = 0; acc_agachado = 0; especial_ativo = false; especial_finalizado = false;
                    crouch_animation_finished = false; // Garante que a flag esteja falsa ao iniciar a animação
                }
                else if (ev.keyboard.keycode == ALLEGRO_KEY_Z && !pulando && !agachando && !especial_ativo && !atacando && !arremessando) { especial_ativo = true; especial_finalizado = false; frame_especial = 0; acc_especial = 0; }
                else if (ev.keyboard.keycode == ALLEGRO_KEY_SPACE && especial_ativo && especial_finalizado && !atacando && !arremessando) { atacando = true; frame_ataque1 = 0; acc_ataque1 = 0; }
                else if (ev.keyboard.keycode == ALLEGRO_KEY_Q && !agachando && !especial_ativo && !atacando && !arremessando) { arremessando = true; frame_arremesso = 0; acc_arremesso = 0; }
                // --- 'B' Key for NPC Interaction ---
                else if (ev.keyboard.keycode == ALLEGRO_KEY_B) {
                    // Calculate protagonist's hitbox center (world coordinates)
                    float prot_world_x_center = personagem_x + (frame_largura_parado * escala_personagens) / 2.0 + deslocamento_x;
                    float npc_world_x_center = traficante.x + (traficante.largura_sprite * escala_traficante) / 2.0;

                    float distance = fabs(prot_world_x_center - npc_world_x_center);

                    if (traficante.ativa && distance < NPC_INTERACTION_DISTANCE) {
                        if (player_money >= NPC_HEAL_COST) {
                            // NEW: Check if protagonist health is already at or above the new maximum
                            if (protagonist_health >= MAX_PROTAGONIST_HEALTH + NPC_HEAL_AMOUNT) { // Max 100 + 20 from one purchase
                                fprintf(stderr, "DEBUG: Vida já está no máximo (120 HP)! Não é possível comprar mais pó.\n");
                            } else {
                                player_money -= NPC_HEAL_COST;
                                protagonist_health += NPC_HEAL_AMOUNT;
                                // NEW: Cap health at 120 (MAX_PROTAGONIST_HEALTH + NPC_HEAL_AMOUNT)
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
                // --- END NEW ---
            }
        }
        // When key is released, set a flag that the key is no longer pressed
        else if (ev.type == ALLEGRO_EVENT_KEY_UP) {
            if (protagonista_estado == PROT_NORMAL) {
                if (ev.keyboard.keycode == ALLEGRO_KEY_DOWN) {
                    // 'agachando' is NOT set to false here directly.
                    // It will only become false in the timer loop once animation finishes AND key is released.
                }
            }
        }
        
        else if (ev.type == ALLEGRO_EVENT_TIMER) {
            // frame_count++; // Not actively used, can remove or use for other purposes
            float delta_deslocamento_x = deslocamento_x - deslocamento_x_anterior;
            deslocamento_x_anterior = deslocamento_x;

            ALLEGRO_KEYBOARD_STATE estado;
            al_get_keyboard_state(&estado);
            bool andando = false, correndo = false;

            // --- Protagonist Movement Logic ---
            float old_deslocamento_x_prot = deslocamento_x;
            float old_personagem_x = personagem_x; // Store old x for collision rollback
            float old_personagem_y = personagem_y; // Store old y for collision rollback

            if (protagonista_estado == PROT_NORMAL) {
                if (!atacando && (!especial_ativo || especial_finalizado) && !arremessando) {
                    velocidade = (al_key_down(&estado, ALLEGRO_KEY_LSHIFT) || al_key_down(&estado, ALLEGRO_KEY_RSHIFT)) ? velocidade_correr : velocidade_andar;
                    if (!pulando) {
                        if (al_key_down(&estado, ALLEGRO_KEY_RIGHT)) {
                            deslocamento_x += velocidade;
                            if (deslocamento_x >= bg_width) deslocamento_x -= bg_width;
                            andando = true;
                            if (velocidade == velocidade_correr) correndo = true;
                        }
                        if (al_key_down(&estado, ALLEGRO_KEY_LEFT)) {
                            deslocamento_x -= velocidade;
                            if (deslocamento_x < 0) deslocamento_x += bg_width;
                            andando = true;
                            if (velocidade == velocidade_correr) correndo = true;
                        }
                    }
                } else if (pulando) {
                    velocidade = (al_key_down(&estado, ALLEGRO_KEY_LSHIFT) || al_key_down(&estado, ALLEGRO_KEY_RSHIFT)) ? velocidade_correr : velocidade_andar;
                }
            } else {
                andando = false; correndo = false;
                pulando = false; agachando = false; atacando = false; arremessando = false;
                vel_y = 0; vel_x_pulo = 0;
                especial_ativo = false; especial_finalizado = false;
                frame_ataque1 = 0; acc_ataque1 = 0;
                
                frame_arremesso = 0; // Added this line to reset arremesso animation when dying
                acc_arremesso = 0; // Added this line to reset arremesso animation when dying
            }

            if (protagonista_estado == PROT_NORMAL) {
                // --- Invulnerability Timer Update ---
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
                // --- END NEW ---

                if (pulando) {
                    personagem_y += vel_y;
                    vel_y += gravidade;
                    deslocamento_x += vel_x_pulo;
                    if (deslocamento_x >= bg_width) deslocamento_x -= bg_width;
                    if (deslocamento_x < 0) deslocamento_x += bg_width;

                    // Check for landing on base ground
                    if (personagem_y >= personagem_y_base && vel_y >= 0) {
                        personagem_y = personagem_y_base;
                        pulando = false;
                        vel_y = 0;
                        vel_x_pulo = 0;
                        current_ground_y = personagem_y_base; // Landed on base ground
                    }

                    acc_pulando += 1.0 / 60.0; if (acc_pulando >= tpf_pulando) { acc_pulando -= tpf_pulando; if (frame_pulando < frame_total_pulando - 1) frame_pulando++; }
                } else if (agachando) {
                    acc_agachado += 1.0 / 60.0;
                    if (acc_agachado >= tpf_agachado) {
                        acc_agachado -= tpf_agachado;
                        if (frame_agachado < frame_total_agachado - 1) {
                            frame_agachado++;
                        } else {
                            // Animation finished.
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
                    acc_arremesso += 1.0 / 60.0; if (acc_arremesso >= tpf_arremesso) { acc_arremesso -= tpf_arremesso; if (frame_arremesso < frame_total_arremesso - 1) { frame_arremesso++; } else { arremessando = false; for (int i = 0; i < MAX_GARRAFAS; i++) { if (!garrafas[i].ativa) { garrafas[i].x = personagem_x + (frame_largura_arremesso * escala_personagens) / 2.0; garrafas[i].y = personagem_y + (frame_altura * escala_personagens) / 2.0; garrafas[i].vel_x = 15.0; garrafas[i].ativa = true; garrafas[i].angulo = 0.0; break; } } frame_arremesso = 0; } }
                    frame_parado = 0; acc_parado = 0; frame_andando = 0; acc_andando = 0; frame_correndo = 0; acc_correndo = 0; frame_agachado = 0; acc_agachado = 0; atacando = false; acc_ataque1 = 0; frame_ataque1 = 0;
                } else {
                    // Apply gravity even when not "jumping" to fall from obstacles
                    if (personagem_y < current_ground_y) {
                        personagem_y += gravidade * 8;
                        // Clamp to current_ground_y if it overshoots
                        if (personagem_y > current_ground_y) {
                            personagem_y = current_ground_y;
                            vel_y = 0;
                        }
                    } else if (personagem_y > current_ground_y) { // If somehow below, snap to ground
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
                    }
                }
            }

            // --- Update NPC Animation Timer ---
            if (traficante.ativa) {
                traficante.acc_animacao += 1.0 / 60.0;
                if (traficante.acc_animacao >= NPC_TRAFICANTE_TPF) {
                    traficante.acc_animacao -= NPC_TRAFICANTE_TPF;
                    traficante.frame_atual = (traficante.frame_atual + 1) % NPC_TRAFICANTE_FRAME_COUNT;
                }
            }
            // --- END NEW ---

            // --- Update NPC Audio Volume (Fade) ---
            if (traficante.ativa && instancia_som_verdebalaraio) {
                // Calculate protagonist's hitbox center (world coordinates)
                float prot_world_x_center = personagem_x + (frame_largura_parado * escala_personagens) / 2.0 + deslocamento_x;
                float npc_world_x_center = traficante.x + (traficante.largura_sprite * escala_traficante) / 2.0;

                float distance = fabs(prot_world_x_center - npc_world_x_center);
                float volume;

                if (distance <= AUDIO_MIN_DISTANCE) {
                    volume = 1.0; // Full volume
                } else if (distance >= AUDIO_MAX_DISTANCE) {
                    volume = 0.0; // Muted
                } else {
                    // Linear interpolation: volume decreases as distance increases
                    volume = 1.0 - ((distance - AUDIO_MIN_DISTANCE) / (AUDIO_MAX_DISTANCE - AUDIO_MIN_DISTANCE));
                    if (volume < 0.0) volume = 0.0; // Clamp to 0
                    if (volume > 1.0) volume = 1.0; // Clamp to 1
                }
                al_set_sample_instance_gain(instancia_som_verdebalaraio, volume);
            }
            // --- END NEW ---


            for (int i = 0; i < MAX_GARRAFAS; i++) {
                if (garrafas[i].ativa) { garrafas[i].x += garrafas[i].vel_x; garrafas[i].angulo += garrafa_velocidade_angular;
                    if (garrafas[i].angulo > ALLEGRO_PI * 2) { garrafas[i].angulo -= ALLEGRO_PI * 2; }
                    if (garrafas[i].x > LARGURA + (garrafa_largura_original * escala_garrafa)) { garrafas[i].ativa = false; } }
            }

            // --- Enemy Spawn Logic ---
            tempo_para_spawn_inimigo -= 1.0 / 60.0;
            if (tempo_para_spawn_inimigo <= 0) {
                for (int i = 0; i < MAX_INIMIGOS; i++) {
                    if (!inimigos[i].ativa) {
                        inimigos[i].x = LARGURA + (float)(rand() % 200 + 50);
                        inimigos[i].y = personagem_y_base;
                        inimigos[i].vel_x = inimigo_velocidade_parado_base;
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
            // --- End Enemy Spawn Logic ---

            // --- NEW: Money Note Spawn Logic ---
            time_to_spawn_money -= 1.0 / 60.0;
            if (time_to_spawn_money <= 0) {
                for (int i = 0; i < MAX_MONEY_NOTES; i++) {
                    if (!money_notes[i].ativa) {
                        int type_index = rand() % num_money_types;
                        money_notes[i].sprite_bitmap = money_sprites[type_index];
                        money_notes[i].value = money_values[type_index];
                        money_notes[i].hitbox_width = al_get_bitmap_width(money_notes[i].sprite_bitmap) * MONEY_NOTE_SCALE;
                        money_notes[i].hitbox_height = al_get_bitmap_height(money_notes[i].sprite_bitmap) * MONEY_NOTE_SCALE;
                        
                        money_notes[i].x = LARGURA + (float)(rand() % 400 + 100); // Spawn off-screen right
                        money_notes[i].y = personagem_y_base + (frame_altura * escala_personagens) - money_notes[i].hitbox_height; // On the ground
                        money_notes[i].ativa = true;
                        time_to_spawn_money = MONEY_NOTE_SPAWN_INTERVAL;
                        fprintf(stderr, "DEBUG: Nota de R$%d SPAWNADA em (%.2f, %.2f).\n", money_notes[i].value, money_notes[i].x, money_notes[i].y);
                        break;
                    }
                }
            }

            // --- Update Money Notes Position and Lifetime ---
            for (int i = 0; i < MAX_MONEY_NOTES; i++) {
                if (money_notes[i].ativa) {
                    money_notes[i].x -= delta_deslocamento_x; // Move with the background scroll
                    // Despawn if off-screen left
                    if (money_notes[i].x < -money_notes[i].hitbox_width) {
                        money_notes[i].ativa = false;
                        fprintf(stderr, "DEBUG: Nota de dinheiro desativada (fora da tela).\n");
                    }
                }
            }
            // --- END NEW ---

            // --- Update Enemies States, Movement, and Check Collisions ---
            // float protagonist_center_x = personagem_x + (frame_largura_parado * escala_personagens) / 2.0; // Not used

            // Calculate protagonist's hitbox for collision checks
            float prot_hb_x = personagem_x + prot_hitbox_offset_x;
            float prot_hb_y = personagem_y + prot_hitbox_offset_y;
            float prot_hb_w = prot_hitbox_width;
            // float prot_hb_h = prot_hitbox_height; // No longer directly used, use current_prot_hitbox_height

            // Adjust protagonist hitbox height if crouching for collision
            float current_prot_hitbox_height = prot_hitbox_height;
            if (agachando) {
                current_prot_hitbox_height = prot_crouch_hitbox_height;
                // Also adjust the Y position of the hitbox so it stays on the ground
                // This means the top of the hitbox moves down
                prot_hb_y = personagem_y + (prot_hitbox_height - prot_crouch_hitbox_height) + prot_hitbox_offset_y;
            } else {
                 prot_hb_y = personagem_y + prot_hitbox_offset_y; // Reset if not crouching
            }


            // Calculate protagonist's ATTACK hitbox for collision checks
            float prot_attack_hb_x = personagem_x + prot_attack_hitbox_offset_x;
            float prot_attack_hb_y = personagem_y + prot_attack_hitbox_offset_y;
            float prot_attack_hb_w = prot_attack_hitbox_width;
            float prot_attack_hb_h = prot_attack_hitbox_height;

            for (int i = 0; i < MAX_INIMIGOS; i++) {
                if (inimigos[i].ativa) {
                    float inimigo_x_antes_movimento = inimigos[i].x;

                    float inimigo_vx_total_no_frame = inimigos[i].vel_x + (-delta_deslocamento_x);
                    inimigos[i].x += inimigo_vx_total_no_frame;
                    
                    float inimigo_current_hb_x = inimigos[i].x + inimigos[i].hitbox_offset_x;
                    float inimigo_current_hb_y = inimigos[i].y + inimigos[i].hitbox_offset_y;
                    float inimigo_current_hb_w = inimigos[i].hitbox_width;
                    float inimigo_current_hb_h = inimigos[i].hitbox_height;


                    // --- Collision: Protagonist Attack vs Enemy ---
                    if (atacando && inimigos[i].estado != INIMIGO_MORRENDO) {
                        if (check_collision(prot_attack_hb_x, prot_attack_hb_y, prot_attack_hb_w, prot_attack_hb_h,
                                             inimigo_current_hb_x, inimigo_current_hb_y, inimigo_current_hb_w, inimigo_current_hb_h)) {
                            fprintf(stderr, "COLISÃO! Ataque Especial atingiu Inimigo %d! Iniciando animação de morte.\n", i);
                            inimigos[i].estado = INIMIGO_MORRENDO;
                            inimigos[i].vel_x = 0;
                            inimigos[i].frame_atual = 0;
                            inimigos[i].acc_animacao = 0;
                            inimigos[i].animacao_morte_finalizada = false;
                        }
                    }
                    // --- END Collision: Protagonist Attack vs Enemy ---


                    // --- Colisão com Protagonista: Lógica de Bloqueio e Ataque ---
                    if (inimigos[i].estado != INIMIGO_MORRENDO && protagonista_estado == PROT_NORMAL) {
                        // Use the adjusted protagonist hitbox for enemy collisions
                        if (check_collision(inimigo_current_hb_x, inimigo_current_hb_y, inimigo_current_hb_w, inimigo_current_hb_h,
                                             prot_hb_x, prot_hb_y, prot_hb_w, current_prot_hitbox_height)) {
                            
                            deslocamento_x = old_deslocamento_x_prot;
                            
                            delta_deslocamento_x = deslocamento_x - deslocamento_x_anterior;
                            deslocamento_x_anterior = deslocamento_x;

                            inimigos[i].x = inimigo_x_antes_movimento;
                            
                            float dist_to_protagonist_x = fabs( inimigo_current_hb_x - prot_hb_x);

                            if (dist_to_protagonist_x < inimigo_distancia_ataque && inimigos[i].estado != INIMIGO_ATACANDO) {
                                fprintf(stderr, "COLISÃO! Inimigo %d atingiu Protagonista! Iniciando ataque do INIMIGO.\n", i);
                                inimigos[i].estado = INIMIGO_ATACANDO;
                                inimigos[i].vel_x = inimigo_velocidade_ataque_base;
                                inimigos[i].frame_atual = 0;
                                inimigos[i].acc_animacao = 0;
                                inimigos[i].inimigo_pode_dar_dano = true;
                            }
                            
                            if (inimigos[i].estado == INIMIGO_ATACANDO && inimigos[i].inimigo_pode_dar_dano && !protagonist_invulnerable) {
                                protagonist_health -= PROTAGONIST_DAMAGE_PER_HIT;
                                inimigos[i].inimigo_pode_dar_dano = false;
                                fprintf(stderr, "PROTAGONISTA SOFREU DANO! Vida restante: %d\n", protagonist_health);

                                protagonist_invulnerable = true;
                                protagonist_invulnerability_timer = PROTAGONIST_INVULNERABILITY_DURATION;
                                protagonist_blink_timer = 0.0;
                                protagonist_visible = false;

                                if (protagonist_health <= 0) {
                                    protagonista_estado = PROT_MORRENDO;
                                    personagem_frame_morte = 0;
                                    personagem_acc_morte = 0;
                                    personagem_morte_animacao_finalizada = false;
                                    fprintf(stderr, "PROTAGONISTA MORREU! Iniciando animação de morte do protagonista.\n");
                                }
                            }
                        } else {
                            inimigos[i].inimigo_pode_dar_dano = true;
                        }
                    }
                    // --- FIM Colisão com Protagonista ---


                    // --- Lógica de Bloqueio (Não atravessar outros inimigos) ---
                    if (inimigos[i].estado != INIMIGO_MORRENDO) {
                        for (int k = 0; k < MAX_INIMIGOS; k++) {
                            if (i == k) continue;
                            if (inimigos[k].ativa && inimigos[k].estado != INIMIGO_MORRENDO) {
                                float other_inimigo_hb_x = inimigos[k].x + inimigos[k].hitbox_offset_x;
                                float other_inimigo_hb_y = inimigos[k].y + inimigos[k].hitbox_offset_y;
                                float other_inimigo_hb_w = inimigos[k].hitbox_width;
                                float other_inimigo_hb_h = inimigos[k].hitbox_height;

                                if (check_collision(inimigo_current_hb_x, inimigo_current_hb_y, inimigo_current_hb_w, inimigo_current_hb_h,
                                                     other_inimigo_hb_x, other_inimigo_hb_y, other_inimigo_hb_w, other_inimigo_hb_h)) {
                                    float overlap_amount = 0;
                                    if (inimigo_current_hb_x < other_inimigo_hb_x) {
                                        overlap_amount = (inimigo_current_hb_x + inimigo_current_hb_w) - other_inimigo_hb_x;
                                        inimigos[i].x -= overlap_amount;
                                    } else {
                                        overlap_amount = (other_inimigo_hb_x + other_inimigo_hb_w) - inimigo_current_hb_x;
                                        inimigos[i].x += overlap_amount;
                                    }
                                    inimigo_current_hb_x = inimigos[i].x + inimigos[i].hitbox_offset_x;
                                }
                            }
                        }
                    }
                    // --- FIM Lógica de Bloqueio entre Inimigos ---


                    // Calculate distance to protagonist (for detection to chase)
                    float dist_to_protagonist_x = fabs( inimigo_current_hb_x - prot_hb_x);

                    // --- State Transition Logic (Chase, Attack) ---
                    if (inimigos[i].estado != INIMIGO_MORRENDO) {
                        if (inimigos[i].estado == INIMIGO_ATACANDO) {
                            inimigos[i].vel_x = inimigo_velocidade_ataque_base;
                            if (dist_to_protagonist_x > inimigo_distancia_ataque) {
                                fprintf(stderr, "DEBUG: Inimigo %d saiu do alcance de ataque. Voltando para ANDANDO.\n", i);
                                inimigos[i].estado = INIMIGO_ANDANDO;
                                inimigos[i].vel_x = inimigo_velocidade_andando_base;
                                inimigos[i].frame_atual = 0;
                                inimigos[i].acc_animacao = 0;
                            }
                        }
                        else if (inimigos[i].estado == INIMIGO_ANDANDO) {
                            inimigos[i].vel_x = inimigo_velocidade_andando_base;
                            if (dist_to_protagonist_x >= inimigo_distancia_deteccao) {
                                fprintf(stderr, "DEBUG: Inimigo %d saiu do alcance de detecção. Voltando para PARADO.\n", i);
                                inimigos[i].estado = INIMIGO_PARADO;
                                inimigos[i].vel_x = inimigo_velocidade_parado_base;
                                inimigos[i].frame_atual = 0;
                                inimigos[i].acc_animacao = 0;
                            } else if (dist_to_protagonist_x <= inimigo_distancia_ataque) {
                                 fprintf(stderr, "DEBUG: Inimigo %d no alcance de ataque. Mudando para ATACANDO.\n", i);
                                 inimigos[i].estado = INIMIGO_ATACANDO;
                                 inimigos[i].vel_x = inimigo_velocidade_ataque_base;
                                 inimigos[i].frame_atual = 0;
                                 inimigos[i].acc_animacao = 0;
                            }
                        }
                        else if (inimigos[i].estado == INIMIGO_PARADO) {
                            inimigos[i].vel_x = inimigo_velocidade_parado_base;
                            if (dist_to_protagonist_x < inimigo_distancia_deteccao && dist_to_protagonist_x > inimigo_distancia_ataque) {
                                fprintf(stderr, "DEBUG: Inimigo %d detectou. Mudando para ANDANDO.\n", i);
                                inimigos[i].estado = INIMIGO_ANDANDO;
                                inimigos[i].vel_x = inimigo_velocidade_andando_base;
                                inimigos[i].frame_atual = 0;
                                inimigos[i].acc_animacao = 0;
                            } else if (dist_to_protagonist_x <= inimigo_distancia_ataque) {
                                 fprintf(stderr, "DEBUG: Inimigo %d no alcance de ataque. Mudando para ATACANDO (do PARADO).\n", i);
                                 inimigos[i].estado = INIMIGO_ATACANDO;
                                 inimigos[i].vel_x = inimigo_velocidade_ataque_base;
                                 inimigos[i].frame_atual = 0;
                                 inimigos[i].acc_animacao = 0;
                            }
                        }
                    }
                    // --- FIM Lógica de Transição de Estado do Inimigo ---
                    
                    // Update enemy animation based on state
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
                            if (inimigos[i].estado == INIMIGO_ATACANDO && inimigos[i].frame_atual == 0) {
                                inimigos[i].inimigo_pode_dar_dano = true;
                            }
                        } else {
                            if (inimigos[i].estado == INIMIGO_ATACANDO) {
                                inimigos[i].frame_atual = 0;
                                inimigos[i].inimigo_pode_dar_dano = true;
                            } else if (inimigos[i].estado == INIMIGO_MORRENDO) {
                                inimigos[i].animacao_morte_finalizada = true;
                            } else {
                                inimigos[i].frame_atual = 0;
                            }
                        }
                    }

                    // Collision detection (Garrafa vs Inimigo)
                    if (inimigos[i].estado != INIMIGO_MORRENDO) {
                        for (int j = 0; j < MAX_GARRAFAS; j++) {
                            if (garrafas[j].ativa) {
                                float garrafa_hb_x = garrafas[j].x - garrafa_hitbox_width / 2.0;
                                float garrafa_hb_y = garrafas[j].y - garrafa_hitbox_height / 2.0;

                                float inimigo_hb_x = inimigos[i].x + inimigos[i].hitbox_offset_x;
                                float inimigo_hb_y = inimigos[i].y + inimigos[i].hitbox_offset_y;
                                float inimigo_hb_w = inimigos[i].hitbox_width;
                                float inimigo_hb_h = inimigos[i].hitbox_height;

                                if (check_collision(garrafa_hb_x, garrafa_hb_y, garrafa_hitbox_width, garrafa_hitbox_height,
                                                     inimigo_hb_x, inimigo_hb_y, inimigo_hb_w, inimigo_hb_h)) {
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
                    
                    // Deactivate logic for enemies
                    bool should_deactivate = false;

                    bool is_offscreen_left = inimigos[i].x < -(inimigo_largura_sprite_max * escala_personagens);
                    bool is_offscreen_right = inimigos[i].x > LARGURA + (inimigo_largura_sprite_max * escala_personagens);

                    if (is_offscreen_left || is_offscreen_right) {
                        if (inimigos[i].estado == INIMIGO_MORRENDO && !inimigos[i].animacao_morte_finalizada) {
                            should_deactivate = false;
                        } else {
                            should_deactivate = true;
                        }
                    }

                    if (should_deactivate) {
                        fprintf(stderr, "DEBUG: Inimigo %d DESATIVADO (saiu da tela após morte ou normal).\n", i);
                        inimigos[i].ativa = false;
                        inimigos[i].estado = INIMIGO_PARADO;
                        inimigos[i].vel_x = inimigo_velocidade_parado_base;
                        inimigos[i].animacao_morte_finalizada = false;
                        inimigos[i].frame_atual = 0;
                        inimigos[i].acc_animacao = 0;
                        inimigos[i].inimigo_pode_dar_dano = true;
                    }
                }
            }

            // --- Protagonist-Obstacle Collision Logic ---
            float prot_current_x_screen = personagem_x; // Character X on screen
            float prot_current_y_screen = personagem_y; // Character Y on screen

            if (protagonista_estado == PROT_NORMAL) {
                // Determine protagonist's bounding box for collision before moving
                // Use the dynamically adjusted hitbox for collisions here
                float prot_left_before = prot_current_x_screen + prot_hitbox_offset_x;
                float prot_right_before = prot_left_before + prot_hitbox_width;
                float prot_top_before = prot_hb_y; // Use the adjusted prot_hb_y
                float prot_bottom_before = prot_hb_y + current_prot_hitbox_height; // Use the adjusted height

                // Predict protagonist's horizontal movement
                float predicted_prot_left = prot_left_before;
                float predicted_prot_right = prot_right_before;
                if (al_key_down(&estado, ALLEGRO_KEY_RIGHT) && !pulando && !atacando && !arremessando) {
                    predicted_prot_left += velocidade;
                    predicted_prot_right += velocidade;
                }
                if (al_key_down(&estado, ALLEGRO_KEY_LEFT) && !pulando && !atacando && !arremessando) {
                    predicted_prot_left -= velocidade;
                    predicted_prot_right -= velocidade;
                }
                
                // Track if protagonist is currently standing on an obstacle
                bool on_obstacle = false;

                for (int i = 0; i < MAX_OBSTACULOS; i++) {
                    if (obstaculos[i].ativa) {
                        // Calculate obstacle's screen coordinates
                        float obstacle_screen_x = obstaculos[i].x - deslocamento_x;
                        float obstacle_top = obstaculos[i].y;
                        float obstacle_bottom = obstaculos[i].y + obstaculos[i].height;
                        float obstacle_left = obstacle_screen_x;
                        float obstacle_right = obstacle_screen_x + obstaculos[i].width;

                        // Check horizontal collision (prevent passing through)
                        // If protagonist is moving right and would collide horizontally
                        if (al_key_down(&estado, ALLEGRO_KEY_RIGHT) && !pulando && !atacando && !arremessando &&
                            prot_right_before <= obstacle_left && predicted_prot_right > obstacle_left &&
                            // Crucial for the "only crouch pass" obstacle:
                            // If it's a "crouch-pass" obstacle AND protagonist is NOT crouching
                            // OR if it's a regular obstacle,
                            // AND there's vertical overlap (character isn't clearly above/below)
                            (obstaculos[i].only_crouch_pass ? !agachando : true) && // Block if it's a crouch-pass obs AND not crouching
                            !(prot_bottom_before <= obstacle_top || prot_top_before >= obstacle_bottom)) {
                            
                            deslocamento_x = old_deslocamento_x_prot; // Revert scroll
                        }
                        // If protagonist is moving left and would collide horizontally
                        else if (al_key_down(&estado, ALLEGRO_KEY_LEFT) && !pulando && !atacando && !arremessando &&
                                 prot_left_before >= obstacle_right && predicted_prot_left < obstacle_right &&
                                 // Crucial for the "only crouch pass" obstacle:
                                 (obstaculos[i].only_crouch_pass ? !agachando : true) && // Block if it's a crouch-pass obs AND not crouching
                                 !(prot_bottom_before <= obstacle_top || prot_top_before >= obstacle_bottom)) {
                            
                            deslocamento_x = old_deslocamento_x_prot; // Revert scroll
                        }

                        // Re-calculate protagonist's current screen position and hitbox after potential horizontal adjustment
                        // (These are needed for the vertical collision check below)
                        prot_left_before = personagem_x + prot_hitbox_offset_x;
                        prot_right_before = prot_left_before + prot_hitbox_width;
                        prot_top_before = prot_hb_y; // Use the adjusted prot_hb_y
                        prot_bottom_before = prot_hb_y + current_prot_hitbox_height; // Use the adjusted height

                        // Check for vertical collision (landing on obstacle)
                        if (pulando && vel_y > 0 &&
                            // Check vertical position relative to obstacle's top (considering character's new hitbox top)
                            (old_personagem_y + prot_hitbox_offset_y + prot_hitbox_height <= obstacle_top ||
                             // Special case: if it's a crouch-pass obstacle, can't land on top if too high
                             (obstaculos[i].only_crouch_pass && (obstacle_top < prot_top_before + current_prot_hitbox_height / 2))) && // Simplified check for "too high to land"
                            personagem_y + prot_hitbox_offset_y + prot_hitbox_height > obstacle_top &&
                            prot_right_before > obstacle_left && prot_left_before < obstacle_right) {
                            
                            personagem_y = obstacle_top - prot_hitbox_offset_y - prot_hitbox_height; // Snap to top of obstacle
                            pulando = false;
                            vel_y = 0;
                            vel_x_pulo = 0;
                            current_ground_y = personagem_y; // Protagonist's current Y is the new ground
                            on_obstacle = true; // Flag that protagonist is on an obstacle
                            fprintf(stderr, "DEBUG: Personagem pousou no obstáculo %d. Novo chão Y: %.2f\n", i, current_ground_y);
                            break; // Landed on one obstacle, no need to check others
                        }

                        // Check if the character is currently standing on this obstacle
                        if (check_collision(prot_left_before, prot_bottom_before - 5, prot_hitbox_width, 10, // Check a small area just below character
                                             obstacle_left, obstacle_top, obstaculos[i].width, obstaculos[i].height) &&
                                             prot_bottom_before >= obstacle_top - 5 && // Use adjusted protagonist bottom
                                             prot_bottom_before <= obstacle_top + 5) { // Use adjusted protagonist bottom
                            on_obstacle = true;
                            current_ground_y = obstacle_top - prot_hitbox_offset_y - prot_hitbox_height; // The obstacle's top is the current ground
                            pulando = false; // Ensure not considered jumping if on an obstacle
                            vel_y = 0; // Stop vertical movement
                        }
                    }
                }
                
                // If protagonist is not on any obstacle and not jumping, reset ground to base.
                // This is crucial for falling off obstacles.
                if (!on_obstacle && !pulando && personagem_y < personagem_y_base) {
                    current_ground_y = personagem_y_base;
                }
            }
            // --- END NEW ---

            // --- NEW: Money Note Collection Logic ---
            if (protagonista_estado == PROT_NORMAL) {
                for (int i = 0; i < MAX_MONEY_NOTES; i++) {
                    if (money_notes[i].ativa) {
                        float money_note_hb_x = money_notes[i].x;
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
            // --- END NEW ---

            al_clear_to_color(al_map_rgb(0, 0, 0));

            int parte1_largura = bg_width - (int)deslocamento_x;
            if (parte1_largura > LARGURA) parte1_largura = LARGURA;
            int parte2_largura = LARGURA - parte1_largura;

            al_draw_scaled_bitmap(background, (int)deslocamento_x, 0, parte1_largura, bg_height, 0, 0, parte1_largura, ALTURA, 0);
            if (parte2_largura > 0) al_draw_scaled_bitmap(background, 0, 0, parte2_largura, bg_height, parte1_largura, 0, parte2_largura, ALTURA, 0);

            // --- Draw Obstacles ---
            for (int i = 0; i < MAX_OBSTACULOS; i++) {
                if (obstaculos[i].ativa) {
                    float draw_x = obstaculos[i].x - deslocamento_x;
                    // Draw using the sprite_bitmap field
                    al_draw_scaled_bitmap(obstaculos[i].sprite_bitmap,
                                         0, 0,
                                         al_get_bitmap_width(obstaculos[i].sprite_bitmap), al_get_bitmap_height(obstaculos[i].sprite_bitmap),
                                         draw_x, obstaculos[i].y, // Obstacle's Y is its hitbox top, which aligns with its visual bottom
                                         obstaculos[i].width, obstaculos[i].height,
                                         0);
                    // DEBUG: Drawing removed
                    // al_draw_rectangle(draw_x, obstaculos[i].y,
                    //                   draw_x + obstaculos[i].width, obstaculos[i].y + obstaculos[i].height,
                    //                   al_map_rgb(255, 0, 0), 1); // Red color for obstacle hitbox
                }
            }
            // --- END NEW ---

            // --- Draw NPC ---
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
            // --- END NEW ---

            // --- DRAW CHARACTER ANIMATION ---
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
            // --- END DRAW CHARACTER ANIMATION ---


            // --- DEBUG: Draw Protagonist Hitbox ---
            // Drawing removed
            // al_draw_rectangle(prot_hb_x, prot_hb_y,
            //                   prot_hb_x + prot_hb_w, prot_hb_y + current_prot_hitbox_height,
            //                   al_map_rgb(255, 255, 0), 1);
            // --- END DEBUG ---

            // --- DEBUG: Draw Protagonist ATTACK Hitbox ---
            // Drawing removed
            // if (atacando) {
            //     float prot_attack_hb_x_debug = personagem_x + prot_attack_hitbox_offset_x;
            //     float prot_attack_hb_y_debug = personagem_y + prot_attack_hitbox_offset_y;
            //     al_draw_rectangle(prot_attack_hb_x_debug, prot_attack_hb_y_debug,
            //                       prot_attack_hb_x_debug + prot_attack_hb_w, prot_attack_hb_y_debug + prot_attack_hb_h,
            //                       al_map_rgb(0, 255, 0), 1);
            // }
            // --- END DEBUG ---


            for (int i = 0; i < MAX_GARRAFAS; i++) {
                if (garrafas[i].ativa) {
                    al_draw_scaled_rotated_bitmap(sprite_garrafa,
                                                     garrafa_largura_original / 2.0, garrafa_altura_original / 2.0,
                                                     garrafas[i].x, garrafas[i].y,
                                                     escala_garrafa, escala_garrafa,
                                                     garrafas[i].angulo,
                                                     0);
                    // DEBUG: Drawing removed
                    // float garrafa_hb_x_debug = garrafas[i].x - garrafa_hitbox_width / 2.0;
                    // float garrafa_hb_y_debug = garrafas[i].y - garrafa_hitbox_height / 2.0;
                    // al_draw_rectangle(garrafa_hb_x_debug, garrafa_hb_y_debug,
                    //                   garrafa_hb_x_debug + garrafa_hitbox_width, garrafa_hb_y_debug + garrafa_hitbox_height,
                    //                   al_map_rgb(255, 0, 255), 1);
                }
            }

            for (int i = 0; i < MAX_INIMIGOS; i++) {
                if (inimigos[i].ativa) {
                    ALLEGRO_BITMAP *current_enemy_sprite;
                    int current_frame_largura_inimigo;
                    
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
                                                     inimigos[i].x, inimigos[i].y,
                                                     current_frame_largura_inimigo * escala_personagens, inimigo_altura_sprite * escala_personagens,
                                                     ALLEGRO_FLIP_HORIZONTAL);

                    // DEBUG: Drawing removed
                    // float inimigo_hb_x_debug = inimigos[i].x + inimigos[i].hitbox_offset_x;
                    // float inimigo_hb_y_debug = inimigos[i].y + inimigos[i].hitbox_offset_y;
                    // al_draw_rectangle(inimigo_hb_x_debug, inimigo_hb_y_debug,
                    //                   inimigo_hb_x_debug + inimigos[i].hitbox_width, inimigo_hb_y_debug + inimigos[i].hitbox_height,
                    //                   al_map_rgb(0, 255, 255), 1);
                }
            }

            // --- Draw Money Notes ---
            for (int i = 0; i < MAX_MONEY_NOTES; i++) {
                if (money_notes[i].ativa) {
                    al_draw_scaled_bitmap(money_notes[i].sprite_bitmap,
                                         0, 0,
                                         al_get_bitmap_width(money_notes[i].sprite_bitmap), al_get_bitmap_height(money_notes[i].sprite_bitmap),
                                         money_notes[i].x, money_notes[i].y,
                                         money_notes[i].hitbox_width, money_notes[i].hitbox_height,
                                         0);
                    // DEBUG: Draw Money Note Hitbox
                    // al_draw_rectangle(money_notes[i].x, money_notes[i].y,
                    //                   money_notes[i].x + money_notes[i].hitbox_width, money_notes[i].y + money_notes[i].hitbox_height,
                    //                   al_map_rgb(255, 165, 0), 1); // Orange for money hitbox
                }
            }
            // --- END NEW ---

            // --- Draw Protagonist Health Bar ---
            float health_bar_width = (float)protagonist_health / (MAX_PROTAGONIST_HEALTH + NPC_HEAL_AMOUNT) * 200; // Adjusted max width
            if (health_bar_width < 0) health_bar_width = 0;
            // Change color to indicate over-heal if protagonist_health > MAX_PROTAGONIST_HEALTH
            ALLEGRO_COLOR health_color = al_map_rgb(0, 255, 0); // Green
            if (protagonist_health > MAX_PROTAGONIST_HEALTH) {
                health_color = al_map_rgb(0, 255, 255); // Cyan for over-heal
            }
            al_draw_filled_rectangle(10, 10, 10 + health_bar_width, 30, health_color);
            al_draw_rectangle(10, 10, 10 + 200, 30, al_map_rgb(255, 255, 255), 2);
            al_draw_textf(fonte, al_map_rgb(255, 255, 255), 220, 15, 0, "HP: %d", protagonist_health);
            // --- END NEW ---

            // --- NEW: Draw Player Money ---
            al_draw_textf(fonte, al_map_rgb(255, 255, 0), LARGURA - 10, 10, ALLEGRO_ALIGN_RIGHT, "R$ %d", player_money);
            // --- END NEW ---

            al_flip_display();
        }
    }

    // --- Destroy Audio Assets ---
    if (instancia_som_verdebalaraio) {
        al_stop_sample_instance(instancia_som_verdebalaraio);
        al_destroy_sample_instance(instancia_som_verdebalaraio);
    }
    if (som_verdebalaraio) al_destroy_sample(som_verdebalaraio);
    // --- END NEW ---

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
    al_destroy_bitmap(sprite_placa_radar); // NEW: Destroy new obstacle sprite
    al_destroy_bitmap(sprite_2reais); // NEW: Destroy money notes
    al_destroy_bitmap(sprite_5reais); // NEW: Destroy money notes
    al_destroy_bitmap(sprite_10reais); // NEW: Destroy money notes
    al_destroy_timer(timer);
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

    bool sair = false;
    while (!sair) {
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
                            sair = true;
                        } else if (opcao_selecionada == 0) { // Iniciar jogo
                            al_close_video(video); // Fecha o vídeo antes de iniciar o jogo
                            jogo(janela, fila, fonte);
                            al_start_video(video, al_get_default_mixer()); // Reinicia vídeo do menu
                        } else {
                            printf("Opção selecionada: %s\n", opcoes[opcao_selecionada]);
                        }
                         break;
                    case ALLEGRO_KEY_ESCAPE:
                        sair = true;
                        break;
                }
            } else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
                sair = true;
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
