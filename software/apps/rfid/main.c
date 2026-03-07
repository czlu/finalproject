// RFID Reader app
//
// Read RFID card IDs using MFRC522 over SPI on micro:bit v2

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrfx_spim.h"

#include "microbit_v2.h"
#include "mfrc522.h"

// SPI instance (SPIM2 has its own IRQ, no conflict with TWI)
static const nrfx_spim_t SPIM_INST = NRFX_SPIM_INSTANCE(2);

// Pin assignments for MFRC522
// SPI pins: SCK=P13, MOSI=P15, MISO=P14
// CS (SDA) pin: P16, RST pin: P8
#define MFRC522_CS_PIN  EDGE_P0
#define MFRC522_RST_PIN EDGE_P16

static void spi_init(void) {
  nrfx_spim_config_t spim_config = NRFX_SPIM_DEFAULT_CONFIG;
  spim_config.sck_pin  = EDGE_P2;
  spim_config.mosi_pin = EDGE_P8;
  spim_config.miso_pin = EDGE_P1;
  spim_config.irq_priority = 0;
  spim_config.frequency = NRF_SPIM_FREQ_4M;
  spim_config.mode = NRF_SPIM_MODE_3;

  nrfx_spim_init(&SPIM_INST, &spim_config, NULL, NULL);
}

int main(void) {
  printf("Board started!\n");

  spi_init();
  mfrc522_init(&SPIM_INST, MFRC522_CS_PIN, MFRC522_RST_PIN);

  uint32_t idcard = 0;
  printf("%lu\n", (unsigned long)idcard);

  uint32_t count = 0;

  while (1) {
    idcard = mfrc522_get_id();

    if (idcard != 0) {
      printf("Card detected! ID: 0x%08lX\n", (unsigned long)idcard);
      nrf_delay_ms(1000);
    }

    count++;
    printf("%lu\n", (unsigned long)count);

    nrf_delay_ms(200);
  }
}
