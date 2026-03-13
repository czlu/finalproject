// Pizza Builder on LCD
// Color sensor (AS7262) detects toppings, FSR detects plate pressure,
// LCD (ILI9341) shows the pizza game UI, and BH1750 detects the oven.

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "nrf_delay.h"
#include "nrf_twi_mngr.h"
#include "nrf_gpio.h"
#include "app_timer.h"

#include "microbit_v2.h"
#include "as7262.h"
#include "fsr.h"
#include "lcd.h"
#include "light.h" 

NRF_TWI_MNGR_DEF(twi_mngr_instance, 1, 0);
APP_TIMER_DEF(game_timer);

typedef struct {
    const char* name;
    const char* toppings[3];
    uint8_t count;
} pizza_t;

static const pizza_t pizzas[] = {
    {"Pepperoni",  {"Cheese", "Pepperoni", NULL},      2},
    {"Veggie",     {"Cheese", "Veggies", NULL},        2},
    {"Everything", {"Cheese", "Pepperoni", "Veggies"}, 3},
    {"Eggplant",   {"Cheese", "Eggplant", NULL},       2}, 
};
#define NUM_PIZZAS 4

static uint8_t current_pizza = 0;
static uint8_t step = 0;
static bool done = false;
static bool baking = false;  
static bool pizza_ready = false; 
static int16_t score = 0;
static const char* last_scan = "Nothing";
static bool needs_redraw = true;
static char message[40] = "Knead the dough!";
static uint16_t message_color = COLOR_WHITE;

static uint8_t dough_presses_needed = 0;
static bool dough_ready = false;
static bool fsr_was_pressed = false;
static uint8_t bake_time = 0;
static const char* candidate_scan = "Nothing";
static uint8_t match_count = 0;

static uint16_t topping_color(const char* name) {
    if (strcmp(name, "Cheese") == 0)    return COLOR_YELLOW;
    if (strcmp(name, "Pepperoni") == 0) return COLOR_RED;
    if (strcmp(name, "Veggies") == 0)   return COLOR_GREEN;
    if (strcmp(name, "Eggplant") == 0)  return 0x780F; 
    return COLOR_GRAY;
}

static void pick_new_pizza(void) {
    //reset
    current_pizza = rand() % NUM_PIZZAS;
    step = 0;
    done = false;
    baking = false;
    pizza_ready = false;
    last_scan = "Nothing";
    bake_time = 0;
    
    dough_presses_needed = 4 + (rand() % 3);
    dough_ready = false;
    
    strcpy(message, "Knead the dough!");
    message_color = COLOR_WHITE;
    needs_redraw = true;
}

static void draw_game(void) {
    const pizza_t* p = &pizzas[current_pizza];

    lcd_fill_screen(COLOR_BLACK);
    lcd_draw_string_2x(10, 10, "STEPHEN'S PIZZERIA", COLOR_ORANGE, COLOR_BLACK);

    char title[30];
    snprintf(title, sizeof(title), "Order: %s", p->name);
    lcd_draw_string_2x(10, 40, title, COLOR_WHITE, COLOR_BLACK);

    for (uint8_t i = 0; i < p->count; i++) {
        uint16_t y = 75 + i * 35;
        char line[30];
        uint16_t fg;

        if (i < step) {
            snprintf(line, sizeof(line), "[OK] %s", p->toppings[i]);
            fg = topping_color(p->toppings[i]);
        } else if (i == step && !done && dough_ready) {
            snprintf(line, sizeof(line), "-> %s", p->toppings[i]);
            fg = COLOR_WHITE;
        } else {
            snprintf(line, sizeof(line), "   %s", p->toppings[i]);
            fg = COLOR_GRAY;
        }

        lcd_draw_string_2x(15, y, line, fg, COLOR_BLACK);
    }

    uint16_t box_color = dough_ready ? COLOR_GREEN : COLOR_RED;
    lcd_fill_rect(170, 75, 40, 40, box_color);
    
    if (!dough_ready) {
        char presses_str[10];
        snprintf(presses_str, sizeof(presses_str), "%d", dough_presses_needed);
        lcd_draw_string_2x(185, 85, presses_str, COLOR_WHITE, COLOR_RED);
    }

    lcd_draw_string_2x(10, 190, message, message_color, COLOR_BLACK);

    char score_str[20];
    snprintf(score_str, sizeof(score_str), "Score: %d", score);
    lcd_draw_string_2x(10, 230, score_str, COLOR_WHITE, COLOR_BLACK);
}

static void handle_scan(const char* ingredient) {
    if (strcmp(ingredient, "Nothing") == 0 || done) return;
    if (strcmp(ingredient, last_scan) == 0) return;

    last_scan = ingredient;
    const pizza_t* p = &pizzas[current_pizza];

    if (step < p->count) {
        if (strcmp(ingredient, p->toppings[step]) == 0) {
            step++;
            if (step >= p->count) {
                // ALL TOPPINGS ADDED! send to the oven.
                done = true;
                baking = true;
                snprintf(message, sizeof(message), "Put in Oven!");
                message_color = COLOR_ORANGE;
            } else {
                snprintf(message, sizeof(message), "Good! Next: %s", p->toppings[step]);
                message_color = COLOR_GREEN;
            }
        } else {
            snprintf(message, sizeof(message), "Wrong! Need %s", p->toppings[step]);
            message_color = COLOR_RED;
        }
        needs_redraw = true;
    }
}

void game_tick(void* _unused) {
    (void)_unused;

    // Always read and print sensor data for debugging
    as7262_color_t color = as7262_read_color();
    const char* current = as7262_color_name(color);
    uint16_t force = fsr_read_raw();
    float lux = bh1750_read_lux();

    printf("V:%.1f B:%.1f G:%.1f Y:%.1f O:%.1f R:%.1f => %s | FSR:%u | Lux:%.1f\n",
        color.violet, color.blue, color.green,
        color.yellow, color.orange, color.red, current, force, lux);

    // toppings
    if (dough_ready && !done) {
        if (strcmp(current, "Nothing") == 0) {
            candidate_scan = "Nothing";
            match_count = 0;
        } else if (strcmp(current, candidate_scan) == 0) {
            match_count++;
            if (match_count == 2) {
                handle_scan(current);
                match_count = 0; 
                candidate_scan = "Nothing"; 
            }
        } else {
            candidate_scan = current;
            match_count = 1;
        }
    } 
    // baking
    else if (baking) {
        // currently baking
        if (lux < 20.0f) {
            bake_time++; 
            
            if (bake_time < 10) {
                snprintf(message, sizeof(message), "Baking... %ds", bake_time);
                message_color = COLOR_YELLOW;
            } else if (bake_time >= 10 && bake_time < 17) {
                snprintf(message, sizeof(message), "DONE! Take out oven!");
                message_color = COLOR_GREEN;
            } else if (bake_time >= 17) {
                snprintf(message, sizeof(message), "BURNING! Take out oven!");
                message_color = COLOR_RED;
            }
            needs_redraw = true;
            
        } 
        // not baking
        else {
            // burnt
            if (bake_time >= 17) {
                baking = false;
                pizza_ready = true;
                score -= 2; 
                snprintf(message, sizeof(message), "Burnt to a crisp! -2");
                message_color = COLOR_RED;
                needs_redraw = true;
            } 
            // good
            else if (bake_time >= 10) {
                baking = false;
                pizza_ready = true;
                score++;
                snprintf(message, sizeof(message), "PIZZA PERFECT! +1");
                message_color = COLOR_GREEN;
                needs_redraw = true;
            } 
            // too early
            else if (bake_time > 0) {
                bake_time = 0;
                snprintf(message, sizeof(message), "Too early! Put back!");
                message_color = COLOR_ORANGE;
                needs_redraw = true;
            }
        }
    }

    if (!nrf_gpio_pin_read(BTN_A)) {
        pick_new_pizza();
    }

    if (needs_redraw) {
        draw_game();
        needs_redraw = false;
    }
}

int main(void) {
    printf("Pizza Builder starting...\n");

    nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
    i2c_config.scl = EDGE_P19;
    i2c_config.sda = EDGE_P20;
    i2c_config.frequency = NRF_DRV_TWI_FREQ_100K;
    i2c_config.interrupt_priority = 0;
    nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);
    as7262_init(&twi_mngr_instance);
    bh1750_init(&twi_mngr_instance);
    fsr_init();
    lcd_init();
    nrf_gpio_cfg_input(BTN_A, NRF_GPIO_PIN_PULLUP);
    srand(fsr_read_raw());

    pick_new_pizza();
    draw_game();
    needs_redraw = false;

    app_timer_init();
    app_timer_create(&game_timer, APP_TIMER_MODE_REPEATED, game_tick);
    app_timer_start(game_timer, 32768, NULL);

    while (1) {
        if (!dough_ready) {
            uint16_t force = fsr_read_raw();
            
            if (force > 1600 && !fsr_was_pressed) {
                fsr_was_pressed = true; 
                
                if (dough_presses_needed > 0) {
                    dough_presses_needed--;
                    needs_redraw = true;
                    
                    if (dough_presses_needed == 0) {
                        dough_ready = true;
                        strcpy(message, "Add toppings!");
                        message_color = COLOR_GREEN;
                    }
                }
            } 
            else if (force < 1000) {
                fsr_was_pressed = false;
            }
        }
        
        nrf_delay_ms(50); 
    }
}