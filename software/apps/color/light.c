// LSM303AGR driver for Microbit_v2
//
// Initializes sensor and communicates over I2C
// Capable of reading temperature, acceleration, and magnetic field strength

#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "light.h"
#include "nrf_delay.h"

static const nrf_twi_mngr_t* i2c_manager = NULL;

static void bh1750_write_command(uint8_t cmd) {

  nrf_twi_mngr_transfer_t const write_transfer[] = {
    NRF_TWI_MNGR_WRITE(BH1750_ADDRESS, &cmd, 1, 0)
  };

  ret_code_t result = nrf_twi_mngr_perform(i2c_manager, NULL, write_transfer, 1, NULL);

  if (result != NRF_SUCCESS) {
    printf("BH1750 Command Write failed! Error: %lX\n", result);
  }

}

void bh1750_init(const nrf_twi_mngr_t* i2c) {
  i2c_manager = i2c;

  bh1750_write_command(BH1750_POWER_ON);
  nrf_delay_ms(10);

  bh1750_write_command(BH1750_CONT_HIRES_MODE);

  nrf_delay_ms(180);
}

float bh1750_read_lux(void) {
  uint8_t rx_buf[2] = {0, 0};

  nrf_twi_mngr_transfer_t const read_transfer[] = {
    NRF_TWI_MNGR_READ(BH1750_ADDRESS, rx_buf, 2, 0)
  };

  ret_code_t result = nrf_twi_mngr_perform(i2c_manager, NULL, read_transfer, 1, NULL);
  if (result != NRF_SUCCESS) {
    printf("BH1750 Read failed! Error: %lX\n", result);
    return 0.0f;
  }

  uint16_t raw_lux = (rx_buf[0] << 8) | rx_buf[1];

  return (float)raw_lux / 1.2f;
}