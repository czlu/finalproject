// AS7262 6-Channel Visible Light / Color Sensor driver

#pragma once

#include "nrf_twi_mngr.h"

// I2C address
static const uint8_t AS7262_ADDRESS = 0x49;

// Virtual register addresses (accessed via status/write/read registers)
typedef enum {
  AS7262_HW_VERSION     = 0x00,
  AS7262_FW_VERSION     = 0x02,
  AS7262_CONTROL_SETUP  = 0x04,
  AS7262_INT_T          = 0x05,
  AS7262_DEVICE_TEMP    = 0x06,
  AS7262_LED_CONTROL    = 0x07,

  // Raw data registers (16-bit each, high byte first)
  AS7262_V_HIGH = 0x08,
  AS7262_V_LOW  = 0x09,
  AS7262_B_HIGH = 0x0A,
  AS7262_B_LOW  = 0x0B,
  AS7262_G_HIGH = 0x0C,
  AS7262_G_LOW  = 0x0D,
  AS7262_Y_HIGH = 0x0E,
  AS7262_Y_LOW  = 0x0F,
  AS7262_O_HIGH = 0x10,
  AS7262_O_LOW  = 0x11,
  AS7262_R_HIGH = 0x12,
  AS7262_R_LOW  = 0x13,

  // Calibrated data registers (32-bit IEEE 754 float each)
  AS7262_V_CAL = 0x14,
  AS7262_B_CAL = 0x18,
  AS7262_G_CAL = 0x1C,
  AS7262_Y_CAL = 0x20,
  AS7262_O_CAL = 0x24,
  AS7262_R_CAL = 0x28,
} as7262_reg_t;

// Physical I2C registers for virtual register access
typedef enum {
  AS7262_STATUS_REG = 0x00,
  AS7262_WRITE_REG  = 0x01,
  AS7262_READ_REG   = 0x02,
} as7262_i2c_reg_t;

// Status register bits
#define AS7262_TX_VALID  0x02
#define AS7262_RX_VALID  0x01

// Control setup bits
#define AS7262_DATA_RDY  0x02

// Gain settings
typedef enum {
  AS7262_GAIN_1X  = 0b00,
  AS7262_GAIN_3_7X = 0b01,
  AS7262_GAIN_16X = 0b10,
  AS7262_GAIN_64X = 0b11,
} as7262_gain_t;

// Measurement modes
typedef enum {
  AS7262_MODE_0 = 0b00, // Continuous VBGY
  AS7262_MODE_1 = 0b01, // Continuous GYOR
  AS7262_MODE_2 = 0b10, // Continuous all 6 channels
  AS7262_MODE_3 = 0b11, // One-shot all 6 channels
} as7262_mode_t;

// Color measurement result
typedef struct {
  float violet;
  float blue;
  float green;
  float yellow;
  float orange;
  float red;
} as7262_color_t;

// Initialize the AS7262 sensor
// i2c - pointer to already initialized and enabled twi manager instance
void as7262_init(const nrf_twi_mngr_t* i2c);

// Read calibrated color values from all 6 channels
as7262_color_t as7262_read_color(void);

// Read device temperature in degrees C
uint8_t as7262_read_temperature(void);

// Determine the dominant color name from a reading
const char* as7262_color_name(as7262_color_t color);
