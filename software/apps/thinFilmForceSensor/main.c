#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "microbit_v2.h"
#include "fsr.h"

int main(void) {
    printf("FSR Demo Starting...\n");
    fsr_init();

    // Arrays holding the pin definitions from microbit_v2.h
    uint32_t led_rows[5] = {LED_ROW1, LED_ROW2, LED_ROW3, LED_ROW4, LED_ROW5};
    uint32_t led_cols[5] = {LED_COL1, LED_COL2, LED_COL3, LED_COL4, LED_COL5};

    // Initialize all LED pins as outputs
    for (int i = 0; i < 5; i++) {
        nrf_gpio_cfg_output(led_rows[i]);
        nrf_gpio_cfg_output(led_cols[i]);
        
        // Start with all LEDs OFF 
        // (Rows LOW means no power, Cols HIGH means no ground)
        nrf_gpio_pin_clear(led_rows[i]); 
        nrf_gpio_pin_set(led_cols[i]);   
    }

    while (1) {
        uint16_t weight = fsr_read_raw();
        
        if (weight >= 1600) {
            printf("Force Reading: %u ---> PRESSED!\n", weight);
            
            // Turn ON all LEDs
            for (int i = 0; i < 5; i++) {
                nrf_gpio_pin_set(led_rows[i]);     // Power the rows (HIGH)
                nrf_gpio_pin_clear(led_cols[i]);   // Sink the columns to ground (LOW)
            }
        } else {
            printf("Force Reading: %u\n", weight);
            
            // Turn OFF all LEDs
            for (int i = 0; i < 5; i++) {
                nrf_gpio_pin_clear(led_rows[i]);   // Cut power to rows (LOW)
                nrf_gpio_pin_set(led_cols[i]);     // Remove ground from columns (HIGH)
            }
        }
        
        nrf_delay_ms(100);
    }
}