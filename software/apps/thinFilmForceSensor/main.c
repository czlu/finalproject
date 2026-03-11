#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrfx_pwm.h"
#include "microbit_v2.h"
#include "fsr.h"
#include "lcd.h" // <-- Include your new screen driver

#define SPEAKER_PIN EDGE_P1

static const nrfx_pwm_t PWM_INST = NRFX_PWM_INSTANCE(0);
static bool speaker_on = false;

#define TONE_COUNTERTOP 1136
static nrf_pwm_values_common_t duty_val[1] = { TONE_COUNTERTOP / 2 };
static nrf_pwm_sequence_t pwm_seq = {
    .values.p_common = duty_val,
    .length = 1,
    .repeats = 0,
    .end_delay = 0,
};

static void pwm_init(void) {
    nrfx_pwm_config_t config = {
        .output_pins = { SPEAKER_PIN, NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED, NRFX_PWM_PIN_NOT_USED },
        .irq_priority = 7,
        .base_clock = NRF_PWM_CLK_500kHz,
        .count_mode = NRF_PWM_MODE_UP,
        .top_value = TONE_COUNTERTOP,
        .load_mode = NRF_PWM_LOAD_COMMON,
        .step_mode = NRF_PWM_STEP_AUTO,
    };
    nrfx_pwm_init(&PWM_INST, &config, NULL);
}

int main(void) {
    printf("Pressure Alarm & Visuals Starting...\n");
    
    // Initialize Hardware
    fsr_init();
    pwm_init();
    lcd_init(); // <-- Boot the screen
    
    // Start with a black screen
    lcd_fill_screen(COLOR_BLACK);

    uint32_t led_rows[5] = {LED_ROW1, LED_ROW2, LED_ROW3, LED_ROW4, LED_ROW5};
    uint32_t led_cols[5] = {LED_COL1, LED_COL2, LED_COL3, LED_COL4, LED_COL5};

    for (int i = 0; i < 5; i++) {
        nrf_gpio_cfg_output(led_rows[i]);
        nrf_gpio_cfg_output(led_cols[i]);
        nrf_gpio_pin_clear(led_rows[i]);
        nrf_gpio_pin_set(led_cols[i]);   
    }

    while (1) {
        uint16_t weight = fsr_read_raw();
        
        if (weight >= 1600) {
            if (!speaker_on) {
                printf("Force: %u ---> ALARM ON!\n", weight);
                
                nrfx_pwm_simple_playback(&PWM_INST, &pwm_seq, 1, NRFX_PWM_FLAG_LOOP);
                speaker_on = true;
                
                // Blast the screen RED
                lcd_fill_screen(COLOR_RED);
                
                for (int i = 0; i < 5; i++) {
                    nrf_gpio_pin_set(led_rows[i]);
                    nrf_gpio_pin_clear(led_cols[i]);
                }
            }
        } else {
            if (speaker_on) {
                printf("Force: %u ---> ALARM OFF\n", weight);
                
                nrfx_pwm_stop(&PWM_INST, true);
                speaker_on = false;
                
                // Clear the screen to BLACK
                lcd_fill_screen(COLOR_BLACK);
                
                for (int i = 0; i < 5; i++) {
                    nrf_gpio_pin_clear(led_rows[i]);
                    nrf_gpio_pin_set(led_cols[i]);
                }
            }
        }
        
        nrf_delay_ms(100);
    }
}