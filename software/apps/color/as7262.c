// AS7262 6-Channel Visible Light / Color Sensor driver
//
// Communicates over I2C using the AS7262 virtual register interface.
// The AS7262 does not allow direct register access — instead you read/write
// virtual registers through a status/write/read handshake on physical registers.

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "as7262.h"
#include "nrf_delay.h"
#include "nrf_twi_mngr.h"

static const nrf_twi_mngr_t* i2c_manager = NULL;

// read one physical register
static uint8_t i2c_read_byte(uint8_t reg) {
  uint8_t rx_buf = 0;
  nrf_twi_mngr_transfer_t const xfer[] = {
    NRF_TWI_MNGR_WRITE(AS7262_ADDRESS, &reg, 1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ(AS7262_ADDRESS, &rx_buf, 1, 0),
  };
  ret_code_t result = nrf_twi_mngr_perform(i2c_manager, NULL, xfer, 2, NULL);
  if (result != NRF_SUCCESS) {
    printf("I2C read failed! Error: %lX\n", result);
  }
  return rx_buf;
}

// write one physical register
static void i2c_write_byte(uint8_t reg, uint8_t data) {
  uint8_t msg[2] = {reg, data};
  nrf_twi_mngr_transfer_t xfer = NRF_TWI_MNGR_WRITE(AS7262_ADDRESS, msg, 2, 0);
  ret_code_t result = nrf_twi_mngr_perform(i2c_manager, NULL, &xfer, 1, NULL);
  if (result != NRF_SUCCESS) {
    printf("I2C write failed! Error: %lX\n", result);
  }
}

// wait until TX_VALID bit is clear (device ready to accept write)
static void wait_for_tx_ready(void) {
  for (int i = 0; i < 200; i++) {
    uint8_t status = i2c_read_byte(AS7262_STATUS_REG);
    if ((status & AS7262_TX_VALID) == 0) return;
    nrf_delay_ms(5);
  }
  printf("AS7262: TX ready timeout\n");
}

// wait until RX_VALID bit is set (data available to read)
static void wait_for_rx_ready(void) {
  for (int i = 0; i < 200; i++) {
    uint8_t status = i2c_read_byte(AS7262_STATUS_REG);
    if (status & AS7262_RX_VALID) return;
    nrf_delay_ms(5);
  }
  printf("AS7262: RX ready timeout\n");
}

// read virtual register
static uint8_t virtual_reg_read(uint8_t vreg) {
  wait_for_tx_ready();
  i2c_write_byte(AS7262_WRITE_REG, vreg);
  wait_for_rx_ready();
  return i2c_read_byte(AS7262_READ_REG);
}

// write virtual register
static void virtual_reg_write(uint8_t vreg, uint8_t data) {
  wait_for_tx_ready();
  i2c_write_byte(AS7262_WRITE_REG, vreg | 0x80);
  wait_for_tx_ready();
  i2c_write_byte(AS7262_WRITE_REG, data);
}

// Read a calibrated 32-bit float from 4 consecutive virtual registers
static float read_calibrated_float(uint8_t reg_start) {
  uint32_t raw = 0;
  raw  = (uint32_t)virtual_reg_read(reg_start + 0) << 24;
  raw |= (uint32_t)virtual_reg_read(reg_start + 1) << 16;
  raw |= (uint32_t)virtual_reg_read(reg_start + 2) << 8;
  raw |= (uint32_t)virtual_reg_read(reg_start + 3);

  float result;
  memcpy(&result, &raw, sizeof(float));
  return result;
}

void as7262_init(const nrf_twi_mngr_t* i2c) {
  i2c_manager = i2c;

  nrf_delay_ms(100); 
  uint8_t hw_ver = virtual_reg_read(AS7262_HW_VERSION);
  printf("AS7262 HW version: 0x%02X\n", hw_ver);

  // Set integration time: 50 * 2.8ms = 140ms
  virtual_reg_write(AS7262_INT_T, 50);

  // Set gain to 16x and mode to continuous all 6 channels
  // CONTROL_SETUP register: [7:6]=gain, [5:4]=unused, [3:2]=mode, [1]=DATA_RDY, [0]=RST
  uint8_t control = (AS7262_GAIN_16X << 4) | (AS7262_MODE_2 << 2);
  virtual_reg_write(AS7262_CONTROL_SETUP, control);

  // Enable LED driver current (for illumination), low current
  // LED_CONTROL: [3]=LED_EN, [2:0]=LED_CURRENT (000 = 12.5mA)
  virtual_reg_write(AS7262_LED_CONTROL, 0x08);

  nrf_delay_ms(200);
  printf("AS7262 initialized\n");
}

as7262_color_t as7262_read_color(void) {
  for (int i = 0; i < 100; i++) {
    uint8_t control = virtual_reg_read(AS7262_CONTROL_SETUP);
    if (control & AS7262_DATA_RDY) break;
    nrf_delay_ms(10);
  }

  as7262_color_t color;
  color.violet = read_calibrated_float(AS7262_V_CAL);
  color.blue   = read_calibrated_float(AS7262_B_CAL);
  color.green  = read_calibrated_float(AS7262_G_CAL);
  color.yellow = read_calibrated_float(AS7262_Y_CAL);
  color.orange = read_calibrated_float(AS7262_O_CAL);
  color.red    = read_calibrated_float(AS7262_R_CAL);

  return color;
}

const char* as7262_color_name(as7262_color_t c) {
    float total = c.violet + c.blue + c.green + c.yellow + c.orange + c.red;

    // total brightness
    if (total < 800.0f) {
        return "Nothing";
    }

    // Eggplant is only topping where Violet completely overpowers Orange and Red
    if (c.violet > c.orange && c.violet > c.red) {
        return "Eggplant";
    }

    if (c.green > c.orange) {
        return "Veggies";
    }

    if (c.red > (c.green * 1.5f)) {
        return "Pepperoni";
    }

    return "Cheese";
}
