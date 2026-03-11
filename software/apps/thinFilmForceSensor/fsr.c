#include "nrf.h"
#include "fsr.h"

void fsr_init(void) {
    // 1. Turn on the power to the SAADC hardware
    NRF_SAADC->ENABLE = 1;

    // 2. Set the resolution to 12-bit (Value '2' = 12-bit mode)
    NRF_SAADC->RESOLUTION = 2;

    // 3. Configure Channel 0 directly using bit-shifting
    // Gain 1/6 (bit 8), Internal 0.6V Ref (bit 12), AcqTime 10us (bit 16), Single-Ended (bit 20)
    NRF_SAADC->CH[0].CONFIG = (0 << 8) | (0 << 12) | (1 << 16) | (0 << 20);

    // 4. Connect Analog Input 0 (AIN0) to Channel 0's positive pin
    NRF_SAADC->CH[0].PSELP = 1; // 1 = AIN0
    NRF_SAADC->CH[0].PSELN = 0; // 0 = Not Connected
}

uint16_t fsr_read_raw(void) {
    // 'static' keeps it in standard RAM (safe for DMA)
    // 'volatile' tells the compiler: "The hardware changes this, don't ignore it!"
    static volatile int16_t val = 0;

    // 1. Tell the hardware exactly where in memory to put the reading
    NRF_SAADC->RESULT.PTR = (uint32_t)&val;
    NRF_SAADC->RESULT.MAXCNT = 1;

    // 2. Trigger the START task and wait for the hardware to acknowledge
    NRF_SAADC->EVENTS_STARTED = 0;
    NRF_SAADC->TASKS_START = 1;
    while (NRF_SAADC->EVENTS_STARTED == 0);

    // 3. Trigger the SAMPLE task to take the reading and wait for it to finish
    NRF_SAADC->EVENTS_END = 0;
    NRF_SAADC->TASKS_SAMPLE = 1;
    while (NRF_SAADC->EVENTS_END == 0);

    // 4. Trigger the STOP task so we don't leave the hardware hanging
    NRF_SAADC->EVENTS_STOPPED = 0;
    NRF_SAADC->TASKS_STOP = 1;
    while (NRF_SAADC->EVENTS_STOPPED == 0);

    // Return the value
    return (val < 0) ? 0 : (uint16_t)val;
}