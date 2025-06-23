#include "protagonista.h"
#include <allegro5/allegro_primitives.h> // Para desenhar hitboxes (se necessário para depuração)
#include <stdio.h> // Para fprintf
#include <math.h> // Para fabs (se usado no futuro)

// Definições que o módulo precisa, transferidas de main.c ou refinadas.
// Idealmente, LARGURA e ALTURA viriam de um cabeçalho global como 'config.h'
// ou seriam passadas como parâmetros para inicialização se fossem dinâmicas.
#define GAME_SCREEN_WIDTH 1536
#define GAME_SCREEN_HEIGHT 864

// Definições de Jogo que o protagonista usa
#define MAX_PROTAGONIST_HEALTH 100
#define PROTAGONIST_INVULNERABILITY_DURATION 1.0
#define PROTAGONIST_BLINK_INTERVAL 0.1

// NOVO: Definindo MAX_GARRAFAS para uso no protagonista.c
#define MAX_GARRAFAS 5

// TPFs (Tempo Por Frame) para as animações
const float TPF_PARADO = 1.0 / 5.0;
const float TPF_ANDANDO = 1.0 / 10.0;
const float TPF_CORRENDO = 1.0 / 10.0;
const float TPF_PULANDO = 1.0 / 12.0;
const float TPF_AGACHADO = 1.0 / 8.0;
const float TPF_ESPECIAL = 1.0 / 15.0;
const float TPF_ATAQUE1 = 1.0 / 10.0;
const float TPF_ARREMESSANDO = 1.0 / 8.0;
const float TPF_MORTE = 1.0 / 8.0; // Adicionado, se aplicável na lógica de morte

// Definições de Pulo e Gravidade
#define GRAVIDADE 0.5
#define VEL_PULO_INICIAL -10.0

void protagonista_inicializar(Protagonista *prot, ALLEGRO_BITMAP *sprites[]) {
    // Sprites
    prot->sprite_parado = sprites[0];
    prot->sprite_andando = sprites[1];
    prot->sprite_correndo = sprites[2];
    prot->sprite_pulando = sprites[3];
    prot->sprite_agachado = sprites[4];
    prot->sprite_especial = sprites[5];
    prot->sprite_ataque1 = sprites[6];
    prot->sprite_arremessando = sprites[7];
    prot->sprite_morte = sprites[8];

    // Contagem de frames
    // ATENÇÃO: Essas linhas **já atribuem** aos membros da struct, então não precisam do 'prot->' no lado esquerdo aqui.
    // O erro estava no seu código anterior que tinha 'prot->frame_total_parado = prot->frame_total_parado', que é redundante
    // ou tentava acessar 'frame_total_parado' do lado direito sem ser um membro da struct.
    prot->frame_total_parado = 6;
    prot->frame_total_andando = 8;
    prot->frame_total_correndo = 8;
    prot->frame_total_pulando = 16;
    prot->frame_total_agachado = 8;
    prot->frame_total_especial = 13;
    prot->frame_total_ataque1 = 5;
    prot->frame_total_arremessando = 4; // <--- Linha 52 que causava erro no código anterior, corrigida para ser apenas atribuição.
    prot->frame_total_morte = 4;

    // Dimensões dos frames
    prot->frame_h = 128; // Altura padrão para todos os frames
    prot->frame_w_parado = al_get_bitmap_width(prot->sprite_parado) / prot->frame_total_parado;
    prot->frame_w_andando = al_get_bitmap_width(prot->sprite_andando) / prot->frame_total_andando;
    prot->frame_w_correndo = al_get_bitmap_width(prot->sprite_correndo) / prot->frame_total_correndo;
    prot->frame_w_pulando = al_get_bitmap_width(prot->sprite_pulando) / prot->frame_total_pulando;
    prot->frame_w_agachado = al_get_bitmap_width(prot->sprite_agachado) / prot->frame_total_agachado;
    prot->frame_w_especial = al_get_bitmap_width(prot->sprite_especial) / prot->frame_total_especial;
    prot->frame_w_ataque1 = al_get_bitmap_width(prot->sprite_ataque1) / prot->frame_total_ataque1;
    // ATENÇÃO: Aqui também o erro estava na referência ao lado direito.
    prot->frame_w_arremessando = al_get_bitmap_width(prot->sprite_arremessando) / prot->frame_total_arremessando;
    prot->frame_w_morte = al_get_bitmap_width(prot->sprite_morte) / prot->frame_total_morte;

    // Posição e Estado
    prot->escala_sprites = 3.0; // Agora um membro da struct
    prot->x = GAME_SCREEN_WIDTH / 2.0 - (prot->frame_w_parado * prot->escala_sprites) / 2.0;
    prot->y_base = GAME_SCREEN_HEIGHT - 300; // Define a base do chão
    prot->y = prot->y_base;
    prot->y_chao_atual = prot->y_base; // Chão atual começa na base
    prot->vel_y = 0;
    prot->vel_x_pulo = 0;
    prot->pulando = false;
    prot->agachando = false;
    prot->atacando = false;
    prot->arremessando = false;
    prot->especial_ativo = false;
    prot->especial_finalizado = false;
    prot->animacao_agachar_finalizada = false;
    prot->estado = PROT_NORMAL;
    prot->vida = MAX_PROTAGONIST_HEALTH;
    prot->dinheiro = 0; // Dinheiro inicial, pode ser passado como parâmetro ou gerenciado externamente
    prot->invulneravel = false;
    prot->visivel = true;
    prot->timer_invulnerabilidade = 0;
    prot->timer_piscar = 0;
    prot->frame_atual = 0;
    prot->acc_animacao = 0;
    prot->animacao_morte_completa = false; // Inicializa a nova flag
    prot->velocidade_atual = 0; // Inicializa velocidade atual

    // Inicialização das Propriedades da Hitbox (agora membros da struct)
    prot->prot_hitbox_offset_x = 40.0 * prot->escala_sprites;
    prot->prot_hitbox_offset_y = 5.0 * prot->escala_sprites;
    prot->prot_hitbox_width = (prot->frame_w_parado - 80) * prot->escala_sprites;
    prot->prot_hitbox_height = (prot->frame_h - 10) * prot->escala_sprites;
    prot->prot_crouch_hitbox_height = (prot->frame_h - 50) * prot->escala_sprites;

    // Inicialização das Propriedades da Hitbox de ATAQUE (agora membros da struct)
    prot->prot_attack_hitbox_offset_x = 60.0 * prot->escala_sprites;
    prot->prot_attack_hitbox_offset_y = 50.0 * prot->escala_sprites;
    prot->prot_attack_hitbox_width = 50 * prot->escala_sprites;
    prot->prot_attack_hitbox_height = 80.0 * prot->escala_sprites;

    fprintf(stderr, "DEBUG: Protagonista inicializado.\n");
}

void protagonista_processar_evento(Protagonista *prot, ALLEGRO_EVENT *ev, Garrafa garrafas[], float deslocamento_x) {
    if (prot->estado != PROT_NORMAL) return;

    if (ev->type == ALLEGRO_EVENT_KEY_DOWN) {
        if (ev->keyboard.keycode == ALLEGRO_KEY_UP && !prot->pulando && !prot->agachando && !prot->atacando && !prot->arremessando) {
            prot->pulando = true;
            prot->vel_y = VEL_PULO_INICIAL; // Usando a constante VEL_PULO_INICIAL
            prot->frame_atual = 0;
            prot->acc_animacao = 0;
            prot->especial_ativo = false;
            prot->especial_finalizado = false;
            // A lógica de vel_x_pulo é tratada no _atualizar com o estado do teclado
            prot->y_chao_atual = prot->y_base; // Garante que a gravidade o traga de volta ao chão original
        }
        else if (ev->keyboard.keycode == ALLEGRO_KEY_DOWN && !prot->pulando && !prot->atacando && !prot->arremessando) {
            prot->agachando = true;
            prot->frame_atual = 0;
            prot->acc_animacao = 0;
            prot->especial_ativo = false;
            prot->especial_finalizado = false;
            prot->animacao_agachar_finalizada = false;
        }
        else if (ev->keyboard.keycode == ALLEGRO_KEY_Z && !prot->pulando && !prot->agachando && !prot->especial_ativo && !prot->atacando && !prot->arremessando) {
             prot->especial_ativo = true;
             prot->especial_finalizado = false;
             prot->frame_atual = 0;
             prot->acc_animacao = 0;
        }
        else if (ev->keyboard.keycode == ALLEGRO_KEY_SPACE && prot->especial_ativo && prot->especial_finalizado && !prot->atacando && !prot->arremessando) {
             prot->atacando = true;
             prot->frame_atual = 0;
             prot->acc_animacao = 0;
        }
        else if (ev->keyboard.keycode == ALLEGRO_KEY_Q && !prot->agachando && !prot->especial_ativo && !prot->atacando && !prot->arremessando) {
             prot->arremessando = true;
             prot->frame_atual = 0;
             prot->acc_animacao = 0;

             // Lógica para criar a garrafa ao arremessar
             for (int i = 0; i < MAX_GARRAFAS; i++) { // <-- Linha 148 que causava erro no código anterior
                 if (!garrafas[i].ativa) {
                     garrafas[i].x = prot->x + (prot->frame_w_arremessando * prot->escala_sprites) / 2.0 + deslocamento_x; // <-- Linha 150 que causava erro no código anterior
                     garrafas[i].y = prot->y + (prot->frame_h * prot->escala_sprites) / 2.0;
                     garrafas[i].vel_x = 15.0; // Velocidade da garrafa
                     garrafas[i].angulo = 0.0;
                     garrafas[i].ativa = true;
                     break;
                 }
             }
        }
    }
}

void protagonista_atualizar(Protagonista *prot, ALLEGRO_KEYBOARD_STATE *teclado, float *deslocamento_x, float velocidade_andar, float velocidade_correr) {
    if (prot->estado == PROT_MORRENDO) {
        prot->acc_animacao += 1.0 / 60.0;
        if (prot->acc_animacao >= TPF_MORTE) {
            prot->acc_animacao -= TPF_MORTE;
            if (prot->frame_atual < prot->frame_total_morte - 1) {
                prot->frame_atual++;
            } else {
                prot->animacao_morte_completa = true; // ATENÇÃO: Usando a nova flag
            }
        }
        return; // Não processa outras lógicas se estiver morrendo
    }

    // Lógica de invulnerabilidade e piscar
    if (prot->invulneravel) {
        prot->timer_invulnerabilidade -= 1.0 / 60.0;
        prot->timer_piscar += 1.0 / 60.0;
        if (prot->timer_piscar >= PROTAGONIST_BLINK_INTERVAL) {
            prot->timer_piscar = 0.0;
            prot->visivel = !prot->visivel;
        }
        if (prot->timer_invulnerabilidade <= 0) {
            prot->invulneravel = false;
            prot->visivel = true;
        }
    }

    bool andando_h = false; // "andando_h" para horizontal, para não confundir com o 'andando' de `velocidade_atual`
    prot->velocidade_atual = (al_key_down(teclado, ALLEGRO_KEY_LSHIFT) || al_key_down(teclado, ALLEGRO_KEY_RSHIFT)) ? velocidade_correr : velocidade_andar;

    // Só permite movimento horizontal se não estiver em certas animações de ataque/arremesso/agachar
    if (!prot->atacando && !prot->arremessando && !prot->agachando && (!prot->especial_ativo || prot->especial_finalizado)) {
        if (al_key_down(teclado, ALLEGRO_KEY_RIGHT)) {
            *deslocamento_x += prot->velocidade_atual;
            andando_h = true;
        }
        if (al_key_down(teclado, ALLEGRO_KEY_LEFT)) {
            *deslocamento_x -= prot->velocidade_atual;
            andando_h = true;
        }
    }

    // --- LÓGICA DE ESTADOS E ANIMAÇÕES ---
    prot->acc_animacao += 1.0 / 60.0; // Acumula tempo para animação

    if (prot->pulando) {
        prot->y += prot->vel_y;
        prot->vel_y += GRAVIDADE; // Usando a constante GRAVIDADE

        // Movimento horizontal durante o pulo
        if (al_key_down(teclado, ALLEGRO_KEY_RIGHT)) {
            prot->vel_x_pulo = prot->velocidade_atual;
        } else if (al_key_down(teclado, ALLEGRO_KEY_LEFT)) {
            prot->vel_x_pulo = -prot->velocidade_atual;
        } else {
            prot->vel_x_pulo = 0; // Se nenhuma tecla direcional, para o movimento horizontal
        }
        *deslocamento_x += prot->vel_x_pulo;


        if (prot->y >= prot->y_chao_atual && prot->vel_y >= 0) {
            prot->y = prot->y_chao_atual;
            prot->pulando = false;
            prot->vel_y = 0;
            prot->vel_x_pulo = 0; // Zera vel horizontal de pulo ao tocar o chão
        }
        // Animação de pulo
        if (prot->acc_animacao >= TPF_PULANDO) {
            prot->acc_animacao -= TPF_PULANDO;
            if (prot->frame_atual < prot->frame_total_pulando - 1) prot->frame_atual++;
            else prot->frame_atual = prot->frame_total_pulando - 1; // Mantém no último frame de pulo
        }
    }
    else if (prot->agachando) {
        if (prot->acc_animacao >= TPF_AGACHADO) {
            prot->acc_animacao -= TPF_AGACHADO;
            if (prot->frame_atual < prot->frame_total_agachado - 1) prot->frame_atual++;
            else prot->animacao_agachar_finalizada = true;
        }
        // Se soltou a tecla de agachar E a animação de agachar terminou
        if (!al_key_down(teclado, ALLEGRO_KEY_DOWN) && prot->animacao_agachar_finalizada) {
            prot->agachando = false;
            prot->animacao_agachar_finalizada = false;
            prot->frame_atual = 0; // Volta para o frame inicial (parado)
            prot->acc_animacao = 0;
        }
        // Garante que outras animações sejam resetadas/desativadas
        prot->atacando = false;
        prot->arremessando = false;
        prot->especial_ativo = false; prot->especial_finalizado = false;
    }
    else if (prot->atacando) {
        if (prot->acc_animacao >= TPF_ATAQUE1) {
            prot->acc_animacao -= TPF_ATAQUE1;
            if (prot->frame_atual < prot->frame_total_ataque1 - 1) prot->frame_atual++;
            else {
                prot->atacando = false;
                prot->frame_atual = 0; // Volta para frame inicial (parado/andando)
                prot->acc_animacao = 0;
            }
        }
        // Garante que outras animações sejam resetadas/desativadas
        prot->agachando = false;
        prot->arremessando = false;
        prot->especial_ativo = false; prot->especial_finalizado = false;
    }
    else if (prot->arremessando) {
        if (prot->acc_animacao >= TPF_ARREMESSANDO) { // <-- Linha 272 que causava erro no código anterior
            prot->acc_animacao -= TPF_ARREMESSANDO;
            if (prot->frame_atual < prot->frame_total_arremessando - 1) {
                 prot->frame_atual++;
            } else {
                prot->arremessando = false;
                prot->frame_atual = 0; // Volta para frame inicial (parado/andando)
                prot->acc_animacao = 0;
            }
        }
        // Garante que outras animações sejam resetadas/desativadas
        prot->agachando = false;
        prot->atacando = false;
        prot->especial_ativo = false; prot->especial_finalizado = false;
    }
    else if (prot->especial_ativo) {
         if (prot->acc_animacao >= TPF_ESPECIAL) {
            prot->acc_animacao -= TPF_ESPECIAL;
            if (prot->frame_atual < prot->frame_total_especial - 1) prot->frame_atual++;
            else prot->especial_finalizado = true; // Animação de especial completa, mas ainda em estado "especial_ativo"
        }
        // Garante que outras animações sejam resetadas/desativadas
        prot->agachando = false;
        prot->atacando = false;
        prot->arremessando = false;
    }
    else { // Estado normal: parado, andando ou correndo
        // Garante que o personagem volte ao chão se não estiver pulando mas estiver acima dele
        if (prot->y < prot->y_chao_atual) {
            prot->y += GRAVIDADE * 8; // Queda mais rápida se não estiver pulando ativamente
            if (prot->y > prot->y_chao_atual) {
                prot->y = prot->y_chao_atual;
                prot->vel_y = 0;
            }
        }

        if(andando_h) { // Verifica se há movimento horizontal
            if(prot->velocidade_atual == velocidade_correr) { // Está correndo
                if(prot->acc_animacao >= TPF_CORRENDO) {
                    prot->acc_animacao = 0;
                    prot->frame_atual = (prot->frame_atual + 1) % prot->frame_total_correndo;
                }
            } else { // Está andando
                if(prot->acc_animacao >= TPF_ANDANDO) {
                    prot->acc_animacao = 0;
                    prot->frame_atual = (prot->frame_atual + 1) % prot->frame_total_andando;
                }
            }
        } else { // Parado
             if(prot->acc_animacao >= TPF_PARADO) {
                 prot->acc_animacao = 0;
                 prot->frame_atual = (prot->frame_atual + 1) % prot->frame_total_parado;
             }
        }
        // Garante que frames de outras animações sejam resetados se não estiverem ativos
        if (!andando_h && !prot->pulando && !prot->agachando && !prot->atacando && !prot->arremessando && !prot->especial_ativo) {
            // Se está parado e não em nenhuma outra animação, garante que o frame atual esteja dentro do range de parado
            if (prot->frame_atual >= prot->frame_total_parado) prot->frame_atual = 0;
        }
    }
}

void protagonista_desenhar(Protagonista *prot) {
    if (!prot->visivel) return; // Não desenha se estiver invulnerável e invisível

    ALLEGRO_BITMAP *sprite_atual = NULL;
    int frame_w_atual = 0;
    float draw_y_offset = 0; // Offset para ajustar a posição Y do desenho

    // Seleciona o sprite e a largura do frame com base no estado
    if (prot->estado == PROT_MORRENDO) {
        sprite_atual = prot->sprite_morte;
        frame_w_atual = prot->frame_w_morte;
    } else if (prot->pulando) {
        sprite_atual = prot->sprite_pulando;
        frame_w_atual = prot->frame_w_pulando;
    } else if (prot->agachando) {
        sprite_atual = prot->sprite_agachado;
        frame_w_atual = prot->frame_w_agachado;
        // Ajusta a posição Y para o agachamento:
        // A diferença entre a altura normal da hitbox e a altura agachada é o quanto ele "abaixa".
        // Isso precisa ser refletido no desenho.
        draw_y_offset = (prot->prot_hitbox_height - prot->prot_crouch_hitbox_height);
    } else if (prot->atacando) {
        sprite_atual = prot->sprite_ataque1;
        frame_w_atual = prot->frame_w_ataque1;
    } else if (prot->especial_ativo) {
        sprite_atual = prot->sprite_especial;
        frame_w_atual = prot->frame_w_especial;
    } else if (prot->arremessando) {
        sprite_atual = prot->sprite_arremessando; // <-- Linha 360 que causava erro no código anterior
        frame_w_atual = prot->frame_w_arremessando;
    } else { // Parado, andando ou correndo
        // Verifica a velocidade_atual para decidir entre andando e correndo
        if (prot->velocidade_atual > 0) { // Se está se movendo horizontalmente
            if (prot->velocidade_atual > 3.0) { // Assumindo 3.0 é velocidade_andar, 6.0 é velocidade_correr
                sprite_atual = prot->sprite_correndo;
                frame_w_atual = prot->frame_w_correndo;
            } else {
                sprite_atual = prot->sprite_andando;
                frame_w_atual = prot->frame_w_andando;
            }
        } else { // Parado
            sprite_atual = prot->sprite_parado;
            frame_w_atual = prot->frame_w_parado;
        }
    }

    if(sprite_atual) {
        al_draw_scaled_bitmap(sprite_atual,
                              prot->frame_atual * frame_w_atual, 0,
                              frame_w_atual, prot->frame_h,
                              prot->x, prot->y + draw_y_offset, // Aplica o offset de desenho
                              frame_w_atual * prot->escala_sprites, prot->frame_h * prot->escala_sprites,
                              0);
    }

    // --- DEBUG: Desenha Hitboxes (DESCOMENTE PARA VISUALIZAR) ---
    /*
    ALLEGRO_COLOR hitbox_color = al_map_rgb(255, 0, 0); // Vermelho para hitbox principal
    ALLEGRO_COLOR attack_hitbox_color = al_map_rgb(0, 255, 255); // Ciano para hitbox de ataque

    // Hitbox Principal
    float current_prot_hitbox_height = prot->agachando ? prot->prot_crouch_hitbox_height : prot->prot_hitbox_height;
    float prot_hb_y = prot->y + prot->prot_hitbox_offset_y;
    if(prot->agachando) {
        prot_hb_y = prot->y + (prot->prot_hitbox_height - prot->prot_crouch_hitbox_height) + prot->prot_hitbox_offset_y;
    }
    al_draw_rectangle(prot->x + prot->prot_hitbox_offset_x,
                      prot_hb_y,
                      prot->x + prot->prot_hitbox_offset_x + prot->prot_hitbox_width,
                      prot_hb_y + current_prot_hitbox_height,
                      hitbox_color, 1);

    // Hitbox de Ataque (apenas quando atacando)
    if (prot->atacando) {
        al_draw_rectangle(prot->x + prot->prot_attack_hitbox_offset_x,
                          prot->y + prot->prot_attack_hitbox_offset_y,
                          prot->x + prot->prot_attack_hitbox_offset_x + prot->prot_attack_hitbox_width,
                          prot->y + prot->prot_attack_hitbox_offset_y + prot->prot_attack_hitbox_height,
                          attack_hitbox_color, 1);
    }
    */
}