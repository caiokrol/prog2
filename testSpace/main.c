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

#define SCREEN_W 800
#define SCREEN_H 400
#define GRAVITY 1
#define JUMP_FORCE -15

typedef struct {
    float x, y;
    float vy;
    int w, h;
    int on_ground;
    int crouching;
    ALLEGRO_BITMAP *sprite;
} Player;

int main() {
    al_init();
    al_init_image_addon();
    al_init_primitives_addon();
    al_install_keyboard();

    ALLEGRO_DISPLAY *disp = al_create_display(SCREEN_W, SCREEN_H);
    ALLEGRO_EVENT_QUEUE *queue = al_create_event_queue();
    ALLEGRO_TIMER *timer = al_create_timer(1.0 / 60);
    al_register_event_source(queue, al_get_display_event_source(disp));
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_timer_event_source(timer));

    ALLEGRO_BITMAP *background = al_load_bitmap("background.png");
    Player player = {
        .x = 100, .y = 300,
        .vy = 0,
        .w = 48, .h = 48,
        .on_ground = 1,
        .crouching = 0,
        .sprite = al_load_bitmap("player.png")
    };

    int redraw = 1;
    int keys[ALLEGRO_KEY_MAX] = {0};

    al_start_timer(timer);

    while (1) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
            break;

        else if (ev.type == ALLEGRO_EVENT_KEY_DOWN)
            keys[ev.keyboard.keycode] = 1;

        else if (ev.type == ALLEGRO_EVENT_KEY_UP)
            keys[ev.keyboard.keycode] = 0;

        else if (ev.type == ALLEGRO_EVENT_TIMER) {
            // Movimento lateral
            if (keys[ALLEGRO_KEY_RIGHT] || keys[ALLEGRO_KEY_D]) player.x += 4;
            if (keys[ALLEGRO_KEY_LEFT]  || keys[ALLEGRO_KEY_A]) player.x -= 4;

            // Pulo
            if ((keys[ALLEGRO_KEY_UP] || keys[ALLEGRO_KEY_W] || keys[ALLEGRO_KEY_SPACE]) && player.on_ground) {
                player.vy = JUMP_FORCE;
                player.on_ground = 0;
            }

            // Gravidade
            player.y += player.vy;
            player.vy += GRAVITY;

            // Chão
            if (player.y >= 300) {
                player.y = 300;
                player.vy = 0;
                player.on_ground = 1;
            }

            // Agachar
            player.crouching = keys[ALLEGRO_KEY_DOWN] || keys[ALLEGRO_KEY_S];

            redraw = 1;
        }

        if (redraw && al_is_event_queue_empty(queue)) {
            redraw = 0;
            al_draw_bitmap(background, 0, 0, 0);

            if (player.crouching) {
                al_draw_scaled_bitmap(player.sprite, 0, 0, player.w, player.h / 2,
                                      player.x, player.y + player.h / 2, player.w, player.h / 2, 0);
            } else {
                al_draw_bitmap(player.sprite, player.x, player.y, 0);
            }

            al_flip_display();
            al_clear_to_color(al_map_rgb(0, 0, 0));
        }
    }

    al_destroy_bitmap(player.sprite);
    al_destroy_bitmap(background);
    al_destroy_display(disp);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);
    return 0;
}

