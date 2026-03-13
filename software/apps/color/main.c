// Pizza Builder on LCD
// Color sensor (AS7262) detects toppings, FSR detects plate pressure,
// LCD (ILI9341) shows the pizza game UI.

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

// I2C manager
NRF_TWI_MNGR_DEF(twi_mngr_instance, 1, 0);

// Timer
APP_TIMER_DEF(game_timer);

// Pizza definitions
typedef struct {
    const char* name;
    const char* toppings[3]; // max 3 toppings
    uint8_t count;
} pizza_t;

static const pizza_t pizzas[] = {
    {"Pepperoni",  {"Cheese", "Pepperoni", NULL},      2},
    {"Veggie",     {"Cheese", "Veggies", NULL},         2},
    {"Everything", {"Cheese", "Pepperoni", "Veggies"},  3},
};
#define NUM_PIZZAS 3

// Game state
static uint8_t current_pizza = 0;
static uint8_t step = 0;
static bool done = false;
static uint8_t score = 0;
static const char* last_scan = "Nothing";
static bool needs_redraw = true;
static char message[40] = "Scan toppings!";
static uint16_t message_color = COLOR_WHITE;

// Get color for a topping name
static uint16_t topping_color(const char* name) {
    if (strcmp(name, "Cheese") == 0)    return COLOR_YELLOW;
    if (strcmp(name, "Pepperoni") == 0) return COLOR_RED;
    if (strcmp(name, "Veggies") == 0)   return COLOR_GREEN;
    return COLOR_GRAY;
}

static void pick_new_pizza(void) {
    current_pizza = rand() % NUM_PIZZAS;
    step = 0;
    done = false;
    last_scan = "Nothing";
    strcpy(message, "Scan toppings!");
    message_color = COLOR_WHITE;
    needs_redraw = true;
}

static void draw_game(void) {
    const pizza_t* p = &pizzas[current_pizza];

    // Background
    lcd_fill_screen(COLOR_DARK);

    // Title
    lcd_draw_string_2x(30, 10, "PIZZA BUILDER", COLOR_ORANGE, COLOR_DARK);

    // Pizza name
    char title[30];
    snprintf(title, sizeof(title), "Order: %s", p->name);
    lcd_draw_string_2x(10, 40, title, COLOR_WHITE, COLOR_DARK);

    // Topping list with status
    for (uint8_t i = 0; i < p->count; i++) {
        uint16_t y = 75 + i * 35;
        char line[30];
        uint16_t fg;

        if (i < step) {
            // Completed
            snprintf(line, sizeof(line), "[OK] %s", p->toppings[i]);
            fg = topping_color(p->toppings[i]);
        } else if (i == step && !done) {
            // Current step
            snprintf(line, sizeof(line), "-> %s", p->toppings[i]);
            fg = COLOR_WHITE;
        } else {
            // Future step
            snprintf(line, sizeof(line), "   %s", p->toppings[i]);
            fg = COLOR_GRAY;
        }

        lcd_draw_string_2x(15, y, line, fg, COLOR_DARK);
    }

    // Message area
    lcd_draw_string_2x(10, 190, message, message_color, COLOR_DARK);

    // Score
    char score_str[20];
    snprintf(score_str, sizeof(score_str), "Score: %d", score);
    lcd_draw_string_2x(10, 230, score_str, COLOR_WHITE, COLOR_DARK);

    // Current scan
    char scan_str[30];
    snprintf(scan_str, sizeof(scan_str), "Sensor: %s", last_scan);
    lcd_draw_string(10, 270, scan_str, COLOR_GRAY, COLOR_DARK);

    // Force sensor reading
    uint16_t force = fsr_read_raw();
    char force_str[30];
    snprintf(force_str, sizeof(force_str), "Force: %u", force);
    lcd_draw_string(10, 285, force_str, COLOR_GRAY, COLOR_DARK);

    // Button hint
    lcd_draw_string(10, 305, "BTN_A = New Pizza", COLOR_GRAY, COLOR_DARK);
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
                done = true;
                score++;
                snprintf(message, sizeof(message), "%s done!", p->name);
                message_color = COLOR_GREEN;
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

    // Read color sensor
    as7262_color_t color = as7262_read_color();
    const char* current = as7262_color_name(color);

    // Print to serial too (for debugging / GUI)
    printf("V:%.1f B:%.1f G:%.1f Y:%.1f O:%.1f R:%.1f => %s\n",
        color.violet, color.blue, color.green,
        color.yellow, color.orange, color.red, current);

    handle_scan(current);

    // Check Button A for new pizza
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

    // I2C for color sensor
    nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
    i2c_config.scl = EDGE_P19;
    i2c_config.sda = EDGE_P20;
    i2c_config.frequency = NRF_DRV_TWI_FREQ_100K;
    i2c_config.interrupt_priority = 0;
    nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);
    as7262_init(&twi_mngr_instance);

    // FSR (ADC)
    fsr_init();

    // LCD (SPI)
    lcd_init();

    // Button A input
    nrf_gpio_cfg_input(BTN_A, NRF_GPIO_PIN_PULLUP);

    // Seed random from first ADC reading
    srand(fsr_read_raw());

    // Start game
    pick_new_pizza();
    draw_game();
    needs_redraw = false;

    // Timer: tick every ~1 second
    app_timer_init();
    app_timer_create(&game_timer, APP_TIMER_MODE_REPEATED, game_tick);
    app_timer_start(game_timer, 32768, NULL);

    while (1) {
        nrf_delay_ms(100);
    }
}
