#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "ssd1306.h"

// ---------------------------------------------------------------------------
// Pinos da BitDogLab
// ---------------------------------------------------------------------------
#define BTN_A       5   // A  → +1 rep / incrementa config
#define BTN_B       6   // B  → conclui serie (pausa) / decrementa config
#define BTN_JOY_SW  22  // Joystick SW → RESET novo usuario (segurar 2s)

// ---------------------------------------------------------------------------
// Limites
// ---------------------------------------------------------------------------
#define MAX_SERIES    10
#define MIN_SERIES    1
#define MAX_REPS      50
#define MIN_REPS      1

// Debounce generoso para evitar duplo clique
#define DEBOUNCE_MS   200

// Hold do joystick para reset (2 segundos)
#define RESET_HOLD_MS 2000

// ---------------------------------------------------------------------------
// Estados
// ---------------------------------------------------------------------------
typedef enum {
    STATE_CONFIG_SERIES,  // Configurando numero de series
    STATE_CONFIG_REPS,    // Configurando reps por serie
    STATE_COUNTING,       // Contando reps na serie atual
    STATE_SERIE_PAUSE,    // Pausa entre series — mostra resumo
    STATE_WORKOUT_DONE,   // Todas as series concluidas
    STATE_RESET_HOLD,     // Segurando joystick para resetar
} AppState;

// ---------------------------------------------------------------------------
// Dados do treino
// ---------------------------------------------------------------------------
typedef struct {
    int target_series;
    int target_reps;
    int current_serie;
    int current_rep;
    int reps_per_serie[MAX_SERIES];
} Workout;

// ---------------------------------------------------------------------------
// Globais
// ---------------------------------------------------------------------------
static AppState state   = STATE_CONFIG_SERIES;
static Workout  workout = {0};
static char     buf[32];

static int cfg_series = 3;
static int cfg_reps   = 12;

// Controle de hold do joystick
static uint32_t joy_hold_start     = 0;
static bool     joy_holding        = false;
static AppState state_before_reset = STATE_COUNTING;

// ---------------------------------------------------------------------------
// Debounce: detecta borda de descida com tempo minimo entre acionamentos
// ---------------------------------------------------------------------------
static uint32_t last_a  = 0;
static uint32_t last_b  = 0;

static bool btn_pressed(uint gpio, uint32_t *last_ts) {
    if (!gpio_get(gpio)) {  // LOW = pressionado (pull-up)
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - *last_ts >= DEBOUNCE_MS) {
            *last_ts = now;
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Helper: centraliza string no display (128px)
// ---------------------------------------------------------------------------
static int cx(const char *s, int sc) {
    int w = (int)strlen(s) * 6 * sc;
    int x = (128 - w) / 2;
    return x < 0 ? 0 : x;
}

// ---------------------------------------------------------------------------
// TELA: configurar numero de series
// ---------------------------------------------------------------------------
static void render_config_series(void) {
    ssd1306_clear();

    ssd1306_draw_string(cx("GYM COUNTER", 1), 0, "GYM COUNTER", 1);
    ssd1306_draw_line(0, 10, 127, 10);

    ssd1306_draw_string(cx("Numero de Series", 1), 14, "Numero de Series", 1);

    snprintf(buf, sizeof(buf), "%d", cfg_series);
    ssd1306_draw_string(cx(buf, 3), 24, buf, 3);

    ssd1306_draw_line(0, 53, 127, 53);
    ssd1306_draw_string(cx("A:+  B:-  Joy:ok", 1), 56, "A:+  B:-  Joy:ok", 1);

    ssd1306_show();
}

// ---------------------------------------------------------------------------
// TELA: configurar reps por serie
// ---------------------------------------------------------------------------
static void render_config_reps(void) {
    ssd1306_clear();

    ssd1306_draw_string(cx("GYM COUNTER", 1), 0, "GYM COUNTER", 1);
    ssd1306_draw_line(0, 10, 127, 10);

    ssd1306_draw_string(cx("Reps por Serie", 1), 14, "Reps por Serie", 1);

    snprintf(buf, sizeof(buf), "%d", cfg_reps);
    ssd1306_draw_string(cx(buf, 3), 24, buf, 3);

    ssd1306_draw_line(0, 53, 127, 53);
    ssd1306_draw_string(cx("A:+  B:-  Joy:iniciar", 1), 56, "A:+  B:-  Joy:iniciar", 1);

    ssd1306_show();
}

// ---------------------------------------------------------------------------
// TELA: contagem de reps
// ---------------------------------------------------------------------------
static void render_counting(void) {
    ssd1306_clear();

    snprintf(buf, sizeof(buf), "SERIE %d / %d",
             workout.current_serie, workout.target_series);
    ssd1306_draw_string(cx(buf, 1), 0, buf, 1);
    ssd1306_draw_line(0, 10, 127, 10);

    // Numero grande central
    snprintf(buf, sizeof(buf), "%d", workout.current_rep);
    ssd1306_draw_string(cx(buf, 3), 14, buf, 3);

    // Barra de progresso das reps
    ssd1306_draw_rect(10, 42, 108, 7, false);
    if (workout.target_reps > 0) {
        int fill = workout.current_rep * 108 / workout.target_reps;
        if (fill > 108) fill = 108;
        if (fill > 0) ssd1306_draw_rect(10, 42, fill, 7, true);
    }

    snprintf(buf, sizeof(buf), "meta: %d reps", workout.target_reps);
    ssd1306_draw_string(cx(buf, 1), 51, buf, 1);

    ssd1306_show();
}

// ---------------------------------------------------------------------------
// TELA: pausa entre series
// ---------------------------------------------------------------------------
static void render_serie_pause(void) {
    ssd1306_clear();

    snprintf(buf, sizeof(buf), "SERIE %d FEITA!", workout.current_serie);
    ssd1306_draw_string(cx(buf, 1), 0, buf, 1);
    ssd1306_draw_line(0, 10, 127, 10);

    // Reps feitas nesta serie
    int feitas = workout.reps_per_serie[workout.current_serie - 1];
    snprintf(buf, sizeof(buf), "%d reps", feitas);
    ssd1306_draw_string(cx(buf, 2), 16, buf, 2);

    // Series restantes
    int restantes = workout.target_series - workout.current_serie;
    if (restantes > 0) {
        snprintf(buf, sizeof(buf), "Faltam %d serie(s)", restantes);
    } else {
        snprintf(buf, sizeof(buf), "Ultima serie!");
    }
    ssd1306_draw_string(cx(buf, 1), 38, buf, 1);

    ssd1306_draw_line(0, 49, 127, 49);

    // Instrucao clara do proximo passo
    if (restantes > 0) {
        ssd1306_draw_string(cx("B: proxima serie", 1), 53, "B: proxima serie", 1);
    } else {
        ssd1306_draw_string(cx("B: concluir treino", 1), 53, "B: concluir treino", 1);
    }

    ssd1306_show();
}

// ---------------------------------------------------------------------------
// TELA: treino completo
// ---------------------------------------------------------------------------
static void render_workout_done(void) {
    ssd1306_clear();

    ssd1306_draw_string(cx("TREINO FEITO!", 1), 0, "TREINO FEITO!", 1);
    ssd1306_draw_line(0, 10, 127, 10);

    int total = 0;
    for (int i = 0; i < workout.target_series; i++)
        total += workout.reps_per_serie[i];

    snprintf(buf, sizeof(buf), "%d", total);
    ssd1306_draw_string(cx(buf, 3), 13, buf, 3);
    ssd1306_draw_string(cx("reps no total", 1), 40, "reps no total", 1);

    ssd1306_draw_line(0, 49, 127, 49);
    ssd1306_draw_string(cx("Segure Joystick p/reset", 1), 53,
                        "Segure Joystick p/reset", 1);
    ssd1306_show();
}

// ---------------------------------------------------------------------------
// TELA: barra de confirmacao do reset
// ---------------------------------------------------------------------------
static void render_reset_hold(uint32_t held_ms) {
    ssd1306_clear();
    ssd1306_draw_string(cx("NOVO USUARIO", 1), 2, "NOVO USUARIO", 1);
    ssd1306_draw_line(0, 12, 127, 12);

    ssd1306_draw_rect(10, 24, 108, 12, false);
    int fill = (int)((uint64_t)held_ms * 108 / RESET_HOLD_MS);
    if (fill > 108) fill = 108;
    if (fill > 0) ssd1306_draw_rect(10, 24, fill, 12, true);

    ssd1306_draw_string(cx("Segure para confirmar", 1), 42,
                        "Segure para confirmar", 1);
    ssd1306_draw_string(cx("Solte para cancelar", 1), 53,
                        "Solte para cancelar", 1);
    ssd1306_show();
}

// ---------------------------------------------------------------------------
// Reset: volta para config do zero
// ---------------------------------------------------------------------------
static void do_reset(void) {
    joy_holding = false;
    cfg_series  = 3;
    cfg_reps    = 12;
    memset(&workout, 0, sizeof(workout));
    state = STATE_CONFIG_SERIES;
}

// ---------------------------------------------------------------------------
// Gerencia hold do joystick SW — ativo em qualquer estado exceto config
// ---------------------------------------------------------------------------
static void handle_joy_sw(void) {
    bool sw_down = !gpio_get(BTN_JOY_SW);
    uint32_t now = to_ms_since_boot(get_absolute_time());

    if (sw_down) {
        if (!joy_holding) {
            joy_holding        = true;
            joy_hold_start     = now;
            state_before_reset = state;
        }
        uint32_t held = now - joy_hold_start;
        if (held >= RESET_HOLD_MS) {
            do_reset();
        } else {
            state = STATE_RESET_HOLD;
        }
    } else {
        if (joy_holding && state == STATE_RESET_HOLD) {
            state = state_before_reset;
        }
        joy_holding = false;
    }
}

// ---------------------------------------------------------------------------
// Inicia o treino com as configuracoes atuais
// ---------------------------------------------------------------------------
static void start_workout(void) {
    memset(&workout, 0, sizeof(workout));
    workout.target_series = cfg_series;
    workout.target_reps   = cfg_reps;
    workout.current_serie = 1;
    workout.current_rep   = 0;
    state = STATE_COUNTING;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(void) {
    stdio_init_all();

    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A);

    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);

    gpio_init(BTN_JOY_SW);
    gpio_set_dir(BTN_JOY_SW, GPIO_IN);
    gpio_pull_up(BTN_JOY_SW);

    ssd1306_init();

    // Splash
    ssd1306_clear();
    ssd1306_draw_string(cx("GYM COUNTER", 2), 6,  "GYM COUNTER", 2);
    ssd1306_draw_string(cx("BitDogLab RP2040", 1), 34, "BitDogLab RP2040", 1);
    ssd1306_draw_string(cx("A:rep  B:serie  Joy:ok", 1), 50,
                        "A:rep  B:serie  Joy:ok", 1);
    ssd1306_show();
    sleep_ms(2000);

    while (true) {
        bool a = btn_pressed(BTN_A, &last_a);
        bool b = btn_pressed(BTN_B, &last_b);

        // Joystick SW: hold para reset em qualquer estado exceto config
        bool in_config = (state == STATE_CONFIG_SERIES || state == STATE_CONFIG_REPS);
        if (!in_config) {
            handle_joy_sw();
        }

        // Joystick SW clique simples na config: avanca tela
        if (in_config && !gpio_get(BTN_JOY_SW)) {
            uint32_t now = to_ms_since_boot(get_absolute_time());
            static uint32_t last_joy = 0;
            if (now - last_joy >= DEBOUNCE_MS) {
                last_joy = now;
                if (state == STATE_CONFIG_SERIES) {
                    state = STATE_CONFIG_REPS;
                } else {
                    start_workout();
                }
            }
        }

        switch (state) {

            // -- CONFIG SERIES --
            case STATE_CONFIG_SERIES:
                if (a) { cfg_series++; if (cfg_series > MAX_SERIES) cfg_series = MAX_SERIES; }
                if (b) { cfg_series--; if (cfg_series < MIN_SERIES) cfg_series = MIN_SERIES; }
                render_config_series();
                break;

            // -- CONFIG REPS --
            case STATE_CONFIG_REPS:
                if (a) { cfg_reps++; if (cfg_reps > MAX_REPS) cfg_reps = MAX_REPS; }
                if (b) { cfg_reps--; if (cfg_reps < MIN_REPS) cfg_reps = MIN_REPS; }
                render_config_reps();
                break;

            // -- CONTAGEM --
            case STATE_COUNTING:
                if (a) {
                    workout.current_rep++;
                    if (workout.current_rep > workout.target_reps)
                        workout.current_rep = workout.target_reps;
                }
                if (b) {
                    // B = conclui/pausa a serie atual
                    workout.reps_per_serie[workout.current_serie - 1] = workout.current_rep;
                    state = STATE_SERIE_PAUSE;
                }
                render_counting();
                break;

            // -- PAUSA ENTRE SERIES --
            case STATE_SERIE_PAUSE:
                if (b) {
                    int restantes = workout.target_series - workout.current_serie;
                    if (restantes > 0) {
                        // Ainda tem series: vai para a proxima
                        workout.current_serie++;
                        workout.current_rep = 0;
                        state = STATE_COUNTING;
                    } else {
                        // Era a ultima serie: treino completo
                        state = STATE_WORKOUT_DONE;
                    }
                }
                render_serie_pause();
                break;

            // -- TREINO COMPLETO --
            case STATE_WORKOUT_DONE:
                render_workout_done();
                break;

            // -- RESET EM PROGRESSO --
            case STATE_RESET_HOLD: {
                uint32_t held = to_ms_since_boot(get_absolute_time()) - joy_hold_start;
                render_reset_hold(held);
                break;
            }
        }

        sleep_ms(20);
    }

    return 0;
}