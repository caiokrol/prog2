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

#define LARGURA 1536
#define ALTURA 864

#define NUM_OPCOES 3

const int VIDEO_LARGURA = 1920;
const int VIDEO_ALTURA = 1080;

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
    if (!background) {
        al_show_native_message_box(janela, "Erro", "Imagem", "Erro ao carregar background.png", NULL, 0);
        return;
    }

    ALLEGRO_BITMAP *sprite_parado = al_load_bitmap("personagem.png");
    if (!sprite_parado) {
        al_show_native_message_box(janela, "Erro", "Imagem", "Erro ao carregar personagem.png", NULL, 0);
        al_destroy_bitmap(background);
        return;
    }

    ALLEGRO_BITMAP *sprite_andando = al_load_bitmap("personagem_andando.png");
    if (!sprite_andando) {
        al_show_native_message_box(janela, "Erro", "Imagem", "Erro ao carregar personagem_andando.png", NULL, 0);
        al_destroy_bitmap(background);
        al_destroy_bitmap(sprite_parado);
        return;
    }

    ALLEGRO_BITMAP *sprite_correndo = al_load_bitmap("personagem_correndo.png");
    if (!sprite_correndo) {
        al_show_native_message_box(janela, "Erro", "Imagem", "Erro ao carregar personagem_correndo.png", NULL, 0);
        al_destroy_bitmap(background);
        al_destroy_bitmap(sprite_parado);
        al_destroy_bitmap(sprite_andando);
        return;
    }

    ALLEGRO_BITMAP *sprite_pulando = al_load_bitmap("personagem_pulando.png");
    if (!sprite_pulando) {
        al_show_native_message_box(janela, "Erro", "Imagem", "Erro ao carregar personagem_pulando.png", NULL, 0);
        al_destroy_bitmap(background);
        al_destroy_bitmap(sprite_parado);
        al_destroy_bitmap(sprite_andando);
        al_destroy_bitmap(sprite_correndo);
        return;
    }

    int bg_width = al_get_bitmap_width(background);
    int bg_height = al_get_bitmap_height(background);

    // Frames do sprite parado
    int frame_total_parado = 6;
    int frame_largura_parado = 768 / frame_total_parado; // 128
    int frame_altura_parado = 128;
    int frame_atual_parado = 0;
    float tempo_por_frame_parado = 1.0 / 5.0;
    float tempo_acumulado_parado = 0;

    // Frames do sprite andando
    int frame_total_andando = 8;
    int frame_largura_andando = 1024 / frame_total_andando; // 128
    int frame_altura_andando = 128;
    int frame_atual_andando = 0;
    float tempo_por_frame_andando = 1.0 / 10.0;
    float tempo_acumulado_andando = 0;

    // Frames do sprite correndo
    int frame_total_correndo = 8;
    int frame_largura_correndo = 1024 / frame_total_correndo; // 128
    int frame_altura_correndo = 128;
    int frame_atual_correndo = 0;
    float tempo_por_frame_correndo = 1.0 / 10.0;
    float tempo_acumulado_correndo = 0;

    // Frames do sprite pulando
    int frame_total_pulando = 16;
    int frame_largura_pulando = 2048 / frame_total_pulando; // 128
    int frame_altura_pulando = 128;
    int frame_atual_pulando = 0;
    float tempo_por_frame_pulando = 1.0 / 20.0;
    float tempo_acumulado_pulando = 0;

    float personagem_x = LARGURA / 2.0 - (frame_largura_correndo * escala_personagens) / 2.0;
    float personagem_y_base = (ALTURA - 300) - (frame_altura_correndo * escala_personagens) / 2.0;
    float personagem_y = personagem_y_base;

    bool pulando = false;
    float vel_y = 0.0;
    float gravidade = 0.5;

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
            } else if (ev.keyboard.keycode == ALLEGRO_KEY_UP && !pulando) {
                pulando = true;
                vel_y = -10.0;  // impulso do pulo
                frame_atual_pulando = 0;
                tempo_acumulado_pulando = 0;
            }
        } else if (ev.type == ALLEGRO_EVENT_TIMER) {
            ALLEGRO_KEYBOARD_STATE estado;
            al_get_keyboard_state(&estado);

            bool andando = false;
            bool correndo = false;

            if (pulando) {
                // Atualiza posição vertical com gravidade
                personagem_y += vel_y;
                vel_y += gravidade;

                if (personagem_y >= personagem_y_base) {
                    personagem_y = personagem_y_base;
                    pulando = false;
                    vel_y = 0;
                }

                tempo_acumulado_pulando += 1.0 / 60.0;
                if (tempo_acumulado_pulando >= tempo_por_frame_pulando) {
                    tempo_acumulado_pulando -= tempo_por_frame_pulando;
                    frame_atual_pulando = (frame_atual_pulando + 1) % frame_total_pulando;
                }
            } else {
                // Verifica velocidade com Shift
                if (al_key_down(&estado, ALLEGRO_KEY_LSHIFT) || al_key_down(&estado, ALLEGRO_KEY_RSHIFT)) {
                    velocidade = velocidade_correr;
                } else {
                    velocidade = velocidade_andar;
                }

                if (al_key_down(&estado, ALLEGRO_KEY_RIGHT)) {
                    deslocamento_x += velocidade;
                    if (deslocamento_x >= bg_width) {
                        deslocamento_x -= bg_width;
                    }
                    andando = true;
                    if (velocidade == velocidade_correr) {
                        correndo = true;
                    }
                }
                if (al_key_down(&estado, ALLEGRO_KEY_LEFT)) {
                    deslocamento_x -= velocidade;
                    if (deslocamento_x < 0) {
                        deslocamento_x += bg_width;
                    }
                    andando = true;
                    if (velocidade == velocidade_correr) {
                        correndo = true;
                    }
                }

                // Atualiza animações dependendo do estado
                if (andando) {
                    if (correndo) {
                        tempo_acumulado_correndo += 1.0 / 60.0;
                        if (tempo_acumulado_correndo >= tempo_por_frame_correndo) {
                            tempo_acumulado_correndo -= tempo_por_frame_correndo;
                            frame_atual_correndo = (frame_atual_correndo + 1) % frame_total_correndo;
                        }
                        // Resetar outros
                        frame_atual_andando = 0;
                        tempo_acumulado_andando = 0;
                        frame_atual_parado = 0;
                        tempo_acumulado_parado = 0;
                    } else {
                        tempo_acumulado_andando += 1.0 / 60.0;
                        if (tempo_acumulado_andando >= tempo_por_frame_andando) {
                            tempo_acumulado_andando -= tempo_por_frame_andando;
                            frame_atual_andando = (frame_atual_andando + 1) % frame_total_andando;
                        }
                        // Resetar outros
                        frame_atual_correndo = 0;
                        tempo_acumulado_correndo = 0;
                        frame_atual_parado = 0;
                        tempo_acumulado_parado = 0;
                    }
                } else {
                    tempo_acumulado_parado += 1.0 / 60.0;
                    if (tempo_acumulado_parado >= tempo_por_frame_parado) {
                        tempo_acumulado_parado -= tempo_por_frame_parado;
                        frame_atual_parado = (frame_atual_parado + 1) % frame_total_parado;
                    }
                    // Resetar outros
                    frame_atual_correndo = 0;
                    tempo_acumulado_correndo = 0;
                    frame_atual_andando = 0;
                    tempo_acumulado_andando = 0;
                }
            }

            al_clear_to_color(al_map_rgb(0, 0, 0));

            int parte1_largura = bg_width - (int)deslocamento_x;
            if (parte1_largura > LARGURA) parte1_largura = LARGURA;

            int parte2_largura = LARGURA - parte1_largura;

            al_draw_scaled_bitmap(background,
                                  (int)deslocamento_x, 0,
                                  parte1_largura, bg_height,
                                  0, 0,
                                  parte1_largura,
                                  ALTURA,
                                  0);

            if (parte2_largura > 0) {
                al_draw_scaled_bitmap(background,
                                      0, 0,
                                      parte2_largura, bg_height,
                                      parte1_largura, 0,
                                      parte2_largura,
                                      ALTURA,
                                      0);
            }

            // Desenha o personagem conforme o estado
            if (pulando) {
                al_draw_scaled_bitmap(sprite_pulando,
                                      frame_atual_pulando * frame_largura_pulando, 0,
                                      frame_largura_pulando, frame_altura_pulando,
                                      personagem_x, personagem_y,
                                      frame_largura_pulando * escala_personagens,
                                      frame_altura_pulando * escala_personagens,
                                      0);
            } else if (andando) {
                if (correndo) {
                    al_draw_scaled_bitmap(sprite_correndo,
                                          frame_atual_correndo * frame_largura_correndo, 0,
                                          frame_largura_correndo, frame_altura_correndo,
                                          personagem_x, personagem_y,
                                          frame_largura_correndo * escala_personagens,
                                          frame_altura_correndo * escala_personagens,
                                          0);
                } else {
                    al_draw_scaled_bitmap(sprite_andando,
                                          frame_atual_andando * frame_largura_andando, 0,
                                          frame_largura_andando, frame_altura_andando,
                                          personagem_x, personagem_y,
                                          frame_largura_andando * escala_personagens,
                                          frame_altura_andando * escala_personagens,
                                          0);
                }
            } else {
                al_draw_scaled_bitmap(sprite_parado,
                                      frame_atual_parado * frame_largura_parado, 0,
                                      frame_largura_parado, frame_altura_parado,
                                      personagem_x, personagem_y,
                                      frame_largura_parado * escala_personagens,
                                      frame_altura_parado * escala_personagens,
                                      0);
            }

            al_flip_display();
        }
    }

    al_destroy_bitmap(background);
    al_destroy_bitmap(sprite_parado);
    al_destroy_bitmap(sprite_andando);
    al_destroy_bitmap(sprite_correndo);
    al_destroy_bitmap(sprite_pulando);
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
