#include "nrf.h"
#include "fsr.h"

void fsr_init(void) {
    NRF_SAADC->ENABLE = 1;
    NRF_SAADC->RESOLUTION = 2;

    // Gain 1/6 (bit 8), Internal 0.6V Ref (bit 12), AcqTime 10us (bit 16), Single-Ended (bit 20)
    NRF_SAADC->CH[0].CONFIG = (0 << 8) | (0 << 12) | (1 << 16) | (0 << 20);
    NRF_SAADC->CH[0].PSELP = 1; // 1 = AIN0
    NRF_SAADC->CH[0].PSELN = 0; // 0 = Not Connected
}

uint16_t fsr_read_raw(void) {
    static volatile int16_t val = 0;

    NRF_SAADC->RESULT.PTR = (uint32_t)&val;
    NRF_SAADC->RESULT.MAXCNT = 1;

    // trigger start
    NRF_SAADC->EVENTS_STARTED = 0;
    NRF_SAADC->TASKS_START = 1;
    while (NRF_SAADC->EVENTS_STARTED == 0);

    // trigger sample
    NRF_SAADC->EVENTS_END = 0;
    NRF_SAADC->TASKS_SAMPLE = 1;
    while (NRF_SAADC->EVENTS_END == 0);

    // trigger stop
    NRF_SAADC->EVENTS_STOPPED = 0;
    NRF_SAADC->TASKS_STOP = 1;
    while (NRF_SAADC->EVENTS_STOPPED == 0);

    return (val < 0) ? 0 : (uint16_t)val;
}