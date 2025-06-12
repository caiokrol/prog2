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

#define LARGURA 1536
#define ALTURA 864

#define NUM_OPCOES 3

const int VIDEO_LARGURA = 1920;
const int VIDEO_ALTURA = 1080;

// --- NEW: Structure to hold bottle properties ---
typedef struct {
    float x, y;
    float vel_x;
    float angulo;
    bool ativa;
} Garrafa;

// --- NEW: Define maximum number of bottles ---
#define MAX_GARRAFAS 5 // You can adjust this number

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

    dx = (LARGURA - dw) / 2;
    dy = (ALTURA - dh) / 2;

    al_draw_scaled_bitmap(frame,
                         0, 0,
                         VIDEO_LARGURA, VIDEO_ALTURA,
                         dx, dy,
                         dw, dh,
                         0);
}

void jogo(ALLEGRO_DISPLAY *janela, ALLEGRO_EVENT_QUEUE *fila, ALLEGRO_FONT *fonte) {
    float deslocamento_x = 0.0;
    float velocidade_andar = 3.0;
    float velocidade_correr = 6.0;
    float velocidade = velocidade_andar;
    float escala_personagens = 3.0;

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

    if (!background || !sprite_parado || !sprite_andando || !sprite_correndo || !sprite_pulando ||
        !sprite_agachado || !sprite_especial || !sprite_ataque1 || !sprite_arremesso || !sprite_garrafa) {
        fprintf(stderr, "Failed to load one or more bitmaps! Make sure all sprite files exist.\n");
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
        return;
    }

    int bg_width = al_get_bitmap_width(background);
    int bg_height = al_get_bitmap_height(background);

    int frame_total_parado = 6, frame_total_andando = 8, frame_total_correndo = 8, frame_total_pulando = 16, frame_total_agachado = 8;
    int frame_total_especial = 13;
    int frame_total_ataque1 = 5;
    int frame_total_arremesso = 4;

    int frame_largura_parado = 768 / frame_total_parado;
    int frame_largura_andando = 1024 / frame_total_andando;
    int frame_largura_correndo = 1024 / frame_total_correndo;
    int frame_largura_pulando = 2048 / frame_total_pulando;
    int frame_largura_agachado = 1024 / frame_total_agachado;
    int frame_largura_especial = 1664 / frame_total_especial;
    int frame_largura_ataque1 = 640 / frame_total_ataque1;
    int frame_largura_arremesso = 512 / frame_total_arremesso;
    int frame_altura = 128;

    int garrafa_largura_original = al_get_bitmap_width(sprite_garrafa);
    int garrafa_altura_original = al_get_bitmap_height(sprite_garrafa);

    float escala_garrafa = 1.0;

    int frame_parado = 0, frame_andando = 0, frame_correndo = 0, frame_pulando = 0, frame_agachado = 0, frame_especial = 0, frame_ataque1 = 0;
    int frame_arremesso = 0;

    float tpf_parado = 1.0 / 5.0, tpf_andando = 1.0 / 10.0, tpf_correndo = 1.0 / 10.0, tpf_pulando = 1.0 / 12, tpf_agachado = 1.0 / 8.0;
    float tpf_especial = 1.0 / 15.0;
    float tpf_ataque1 = 1.0 / 10.0;
    float tpf_arremesso = 1.0 / 8.0;

    float acc_parado = 0, acc_andando = 0, acc_correndo = 0, acc_pulando = 0, acc_agachado = 0, acc_especial = 0, acc_ataque1 = 0;
    float acc_arremesso = 0;

    float personagem_x = LARGURA / 2.0 - (frame_largura_correndo * escala_personagens) / 2.0;
    float personagem_y_base = (ALTURA - 300) - (frame_altura * escala_personagens) / 2.0;
    float personagem_y = personagem_y_base;

    bool pulando = false;
    bool agachando = false;
    bool especial_ativo = false;
    bool especial_finalizado = false;
    bool atacando = false;
    bool arremessando = false;

    float vel_y = 0.0;
    float gravidade = 0.5;
    float vel_x_pulo = 0.0;

    // --- NEW: Array of bottles and angular speed ---
    Garrafa garrafas[MAX_GARRAFAS];
    float garrafa_velocidade_angular = 0.2;

    // Initialize all bottles as inactive
    for (int i = 0; i < MAX_GARRAFAS; i++) {
        garrafas[i].ativa = false;
    }
    // --- END NEW ---

    ALLEGRO_TIMER *timer = al_create_timer(1.0 / 60.0);
    al_start_timer(timer);

    al_register_event_source(fila, al_get_display_event_source(janela));
    al_register_event_source(fila, al_get_keyboard_event_source());
    al_register_event_source(fila, al_get_timer_event_source(timer));

    bool jogando = true;

    while (jogando) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(fila, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            jogando = false;
        } else if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                jogando = false;
            }
            // Jumping (only if not jumping, crouching, attacking, or throwing)
            else if (ev.keyboard.keycode == ALLEGRO_KEY_UP && !pulando && !agachando && !atacando && !arremessando) {
                pulando = true;
                vel_y = -10.0;
                frame_pulando = 0;
                acc_pulando = 0;
                especial_ativo = false;
                especial_finalizado = false;

                ALLEGRO_KEYBOARD_STATE estado;
                al_get_keyboard_state(&estado);
                if (al_key_down(&estado, ALLEGRO_KEY_RIGHT)) {
                    vel_x_pulo = velocidade;
                } else if (al_key_down(&estado, ALLEGRO_KEY_LEFT)) {
                    vel_x_pulo = -velocidade;
                } else {
                    vel_x_pulo = 0;
                }
            }
            // Crouching (only if not jumping, attacking, or throwing)
            else if (ev.keyboard.keycode == ALLEGRO_KEY_DOWN && !pulando && !atacando && !arremessando) {
                agachando = true;
                frame_agachado = 0;
                acc_agachado = 0;
                especial_ativo = false;
                especial_finalizado = false;
            }
            // Special Attack (only if not jumping, crouching, active special, attacking, or throwing)
            else if (ev.keyboard.keycode == ALLEGRO_KEY_Z && !pulando && !agachando && !especial_ativo && !atacando && !arremessando) {
                especial_ativo = true;
                especial_finalizado = false;
                frame_especial = 0;
                acc_especial = 0;
            }
            // Regular Attack (only if special is active and finalized, and not attacking, or throwing)
            else if (ev.keyboard.keycode == ALLEGRO_KEY_SPACE && especial_ativo && especial_finalizado && !atacando && !arremessando) {
                atacando = true;
                frame_ataque1 = 0;
                acc_ataque1 = 0;
            }
            // Throw Action (now also during jump, but not if already throwing)
            else if (ev.keyboard.keycode == ALLEGRO_KEY_Q && !agachando && !especial_ativo && !atacando && !arremessando) {
                arremessando = true;
                frame_arremesso = 0;
                acc_arremesso = 0;
                // 'garrafa_ativa' (from single-bottle logic) is completely replaced by individual bottle 'ativa' flags.
            }
        }
        
        else if (ev.type == ALLEGRO_EVENT_TIMER) {
            ALLEGRO_KEYBOARD_STATE estado;
            al_get_keyboard_state(&estado);
            bool andando = false, correndo = false;

            // Horizontal Movement: Stays the same, no changes related to bottle.
            // It correctly prevents movement during attack, special, or character's throwing animation.
            if (!atacando && (!especial_ativo || especial_finalizado) && !arremessando) {
                velocidade = (al_key_down(&estado, ALLEGRO_KEY_LSHIFT) || al_key_down(&estado, ALLEGRO_KEY_RSHIFT)) ? velocidade_correr : velocidade_andar;
                if (!pulando) { // Normal ground movement
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
                // Allows horizontal control during jump, as per previous logic.
                velocidade = (al_key_down(&estado, ALLEGRO_KEY_LSHIFT) || al_key_down(&estado, ALLEGRO_KEY_RSHIFT)) ? velocidade_correr : velocidade_andar;
            }

            // Character State Updates
            // Resetting other animations:
            // The key here is to ONLY reset the arremessando flag and its related counters
            // when the character moves into a new, distinct state (like landing from a jump, or crouching).
            // This ensures that if the character is mid-air throwing, and then lands, the throwing animation state is reset.
            if (pulando) {
                personagem_y += vel_y;
                vel_y += gravidade;
                deslocamento_x += vel_x_pulo; 
                
                if (deslocamento_x >= bg_width) deslocamento_x -= bg_width;
                if (deslocamento_x < 0) deslocamento_x += bg_width;
                
                if (personagem_y >= personagem_y_base) { // Landed from jump
                    personagem_y = personagem_y_base;
                    pulando = false;
                    vel_y = 0;
                    vel_x_pulo = 0;
                    
                    // Reset character animations on landing (standard behavior)
                    acc_pulando = 0; frame_pulando = 0;
                    frame_parado = 0; acc_parado = 0;
                    frame_andando = 0; acc_andando = 0;
                    frame_correndo = 0; acc_correndo = 0;
                    frame_especial = 0; acc_especial = 0;
                    frame_agachado = 0; acc_agachado = 0;
                    atacando = false; acc_ataque1 = 0; frame_ataque1 = 0;
                    
                    // Reset character throwing animation on landing, but NOT the bottle state.
                    // This ensures the character stops "throwing" if they land mid-animation,
                    // but already launched bottles continue.
                    arremessando = false; acc_arremesso = 0; frame_arremesso = 0;
                }
                // Animate jump even if mid-throw
                acc_pulando += 1.0 / 60.0;
                if (acc_pulando >= tpf_pulando) {
                    acc_pulando -= tpf_pulando;
                    if (frame_pulando < frame_total_pulando - 1)
                        frame_pulando++;
                }
            } else if (agachando) {
                acc_agachado += 1.0 / 60.0;
                if (acc_agachado >= tpf_agachado) {
                    acc_agachado -= tpf_agachado;
                    if (frame_agachado < frame_total_agachado - 1) {
                        frame_agachado++;
                    } else {
                        agachando = false; 
                    }
                }
                // Reset other animations on crouching
                frame_parado = 0; acc_parado = 0;
                frame_andando = 0; acc_andando = 0; 
                frame_correndo = 0; acc_correndo = 0;
                frame_especial = 0; acc_especial = 0;
                atacando = false; acc_ataque1 = 0; frame_ataque1 = 0;
                arremessando = false; acc_arremesso = 0; frame_arremesso = 0;
            } else if (atacando) {
                acc_ataque1 += 1.0 / 60.0;
                if (acc_ataque1 >= tpf_ataque1) {
                    acc_ataque1 -= tpf_ataque1;
                    if (frame_ataque1 < frame_total_ataque1 - 1) {
                        frame_ataque1++;
                    } else {
                        atacando = false;
                        frame_ataque1 = 0;
                    }
                }
                // Reset other animations on attack
                frame_parado = 0; acc_parado = 0;
                frame_andando = 0; acc_andando = 0;
                frame_correndo = 0; acc_correndo = 0;
                frame_pulando = 0; acc_pulando = 0;
                frame_agachado = 0; acc_agachado = 0;
                arremessando = false; acc_arremesso = 0; frame_arremesso = 0;
            }
            else if (especial_ativo) {
                acc_especial += 1.0 / 60.0;
                if (acc_especial >= tpf_especial) {
                    acc_especial -= tpf_especial;
                    if (frame_especial < frame_total_especial - 1) {
                        frame_especial++;
                    } else {
                        especial_finalizado = true;
                    }
                }
                // Reset other animations on special
                frame_parado = 0; acc_parado = 0;
                frame_andando = 0; acc_andando = 0;
                frame_correndo = 0; acc_correndo = 0;
                frame_pulando = 0; acc_pulando = 0;
                frame_agachado = 0; acc_agachado = 0;
                atacando = false; acc_ataque1 = 0; frame_ataque1 = 0;
                arremessando = false; acc_arremesso = 0; frame_arremesso = 0;
            }
            // Throwing Animation Update
            else if (arremessando) {
                acc_arremesso += 1.0 / 60.0;
                if (acc_arremesso >= tpf_arremesso) {
                    acc_arremesso -= tpf_arremesso;
                    if (frame_arremesso < frame_total_arremesso - 1) {
                        frame_arremesso++;
                    } else {
                        arremessando = false; // Throwing animation finished

                        // --- NEW: Find an inactive bottle to launch ---
                        for (int i = 0; i < MAX_GARRAFAS; i++) {
                            if (!garrafas[i].ativa) {
                                garrafas[i].x = personagem_x + (frame_largura_arremesso * escala_personagens) / 2.0;
                                garrafas[i].y = personagem_y + (frame_altura * escala_personagens) / 2.0;
                                garrafas[i].vel_x = 15.0; // Same speed as before
                                garrafas[i].ativa = true;
                                garrafas[i].angulo = 0.0;
                                break; // Launch only one bottle per throw animation
                            }
                        }
                        // --- END NEW ---
                        
                        frame_arremesso = 0; // Reset frame for next throw
                    }
                }
                // When character is throwing, ensure other animations are reset
                // This block applies whether the character is on the ground or jumping
                frame_parado = 0; acc_parado = 0;
                frame_andando = 0; acc_andando = 0;
                frame_correndo = 0; acc_correndo = 0;
                // No need to reset frame_pulando/acc_pulando here, as 'pulando' state handles it.
                frame_agachado = 0; acc_agachado = 0;
                atacando = false; acc_ataque1 = 0; frame_ataque1 = 0;
            }
            // Standing, Walking, or Running Logic
            else { // Character is in a "normal" state (idle, walking, running)
                if (andando) {
                    if (correndo) {
                        acc_correndo += 1.0 / 60.0;
                        if (acc_correndo >= tpf_correndo) {
                            acc_correndo -= tpf_correndo;
                            frame_correndo = (frame_correndo + 1) % frame_total_correndo;
                        }
                        frame_andando = 0; acc_andando = 0; frame_parado = 0; acc_parado = 0;
                    } else {
                        acc_andando += 1.0 / 60.0;
                        if (acc_andando >= tpf_andando) {
                            acc_andando -= tpf_andando;
                            frame_andando = (frame_andando + 1) % frame_total_andando;
                        }
                        frame_correndo = 0; acc_correndo = 0; frame_parado = 0; acc_parado = 0;
                    }
                } else { // Character is idle
                    acc_parado += 1.0 / 60.0;
                    if (acc_parado >= tpf_parado) {
                        acc_parado -= tpf_parado;
                        frame_parado = (frame_parado + 1) % frame_total_parado;
                    }
                    frame_correndo = 0; acc_correndo = 0; frame_andando = 0; acc_andando = 0;
                }
                // Ensure no character action-specific flags like arremessando are active here
                // if they are not actively performing that action.
                // This is generally handled by the specific state blocks (pulando, agachando etc.)
            }

            // --- NEW: Update and draw ALL active bottles ---
            for (int i = 0; i < MAX_GARRAFAS; i++) {
                if (garrafas[i].ativa) {
                    garrafas[i].x += garrafas[i].vel_x;
                    garrafas[i].angulo += garrafa_velocidade_angular;

                    if (garrafas[i].angulo > ALLEGRO_PI * 2) {
                        garrafas[i].angulo -= ALLEGRO_PI * 2;
                    }

                    // Deactivate bottle if it goes off-screen
                    if (garrafas[i].x > LARGURA + (garrafa_largura_original * escala_garrafa)) {
                        garrafas[i].ativa = false;
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

            // --- DRAW CHARACTER ANIMATION (UNCHANGED) ---
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
            // --- END DRAW CHARACTER ANIMATION ---

            // --- NEW: Draw ALL active bottles ---
            for (int i = 0; i < MAX_GARRAFAS; i++) {
                if (garrafas[i].ativa) {
                    al_draw_scaled_rotated_bitmap(sprite_garrafa,
                                                  garrafa_largura_original / 2.0, garrafa_altura_original / 2.0,
                                                  garrafas[i].x, garrafas[i].y,
                                                  escala_garrafa, escala_garrafa,
                                                  garrafas[i].angulo,
                                                  0);
                }
            }
            // --- END NEW ---

            al_flip_display();
        }
    }

    // --- CLEANUP RESOURCES ---
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
