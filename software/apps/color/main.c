// Color sensor app
//
// Reads from AS7262 6-channel visible light sensor over I2C
// and prints the detected color to the terminal.

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf_delay.h"
#include "nrf_twi_mngr.h"
#include "app_timer.h"

#include "microbit_v2.h"
#include "as7262.h"

// I2C manager instance
NRF_TWI_MNGR_DEF(twi_mngr_instance, 1, 0);

// Timer for periodic readings
APP_TIMER_DEF(color_timer);

static const char* last_color = "";

void read_color_callback(void* _unused) {
  (void)_unused;

  as7262_color_t color = as7262_read_color();
  const char* current = as7262_color_name(color);

  printf("V:%.1f B:%.1f G:%.1f Y:%.1f O:%.1f R:%.1f => %s\n",
    color.violet, color.blue, color.green,
    color.yellow, color.orange, color.red, current);
}

int main(void) {
  printf("Board started!\n");

  // Initialize I2C on the external QWIIC pins
  // Wiring: SCL -> pin 19, SDA -> pin 20
  nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
  i2c_config.scl = EDGE_P19;
  i2c_config.sda = EDGE_P20;
  i2c_config.frequency = NRF_DRV_TWI_FREQ_100K;
  i2c_config.interrupt_priority = 0;
  nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);

  // Initialize the AS7262 color sensor
  as7262_init(&twi_mngr_instance);

  // Set up a timer to read color every ~1 second
  app_timer_init();
  app_timer_create(&color_timer, APP_TIMER_MODE_REPEATED, read_color_callback);
  app_timer_start(color_timer, 32768, NULL);

  while (1) {
    nrf_delay_ms(1000);
  }
}
