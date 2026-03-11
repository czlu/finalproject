#ifndef FSR_H
#define FSR_H

#include <stdint.h>

// Initialize the ADC for the micro:bit P0 (AIN4 on nRF52833)
void fsr_init(void);

// Read the raw 12-bit value (0-4095)
uint16_t fsr_read_raw(void);

#endif