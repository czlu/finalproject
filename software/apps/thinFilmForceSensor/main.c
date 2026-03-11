#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "nrf_delay.h"
#include "fsr.h"

int main(void) {
    printf("FSR Demo Starting...\n");
    fsr_init();

    while (1) {
        uint16_t weight = fsr_read_raw();
        printf("Force Reading: %u\n", weight);
        nrf_delay_ms(1000);
    }
}