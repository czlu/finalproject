#include "nrf_gpio.h"
#include "nrfx_spim.h"
#include "nrf_delay.h"
#include "microbit_v2.h"
#include "app_error.h"
#include "lcd.h"

// Your Wiring Pins
#define LCD_CS   EDGE_P16
#define LCD_DC   EDGE_P2
#define LCD_RST  EDGE_P8

// We use SPI Instance 1 (Since PWM uses 0)
static const nrfx_spim_t spi = NRFX_SPIM_INSTANCE(2);

// Helper to send a 1-byte Command
static void spi_write_cmd(uint8_t cmd) {
    nrf_gpio_pin_clear(LCD_DC); // DC LOW = Command
    nrf_gpio_pin_clear(LCD_CS);
    nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_TX(&cmd, 1);
    nrfx_spim_xfer(&spi, &xfer, 0);
    nrf_gpio_pin_set(LCD_CS);
}

// Helper to send a 1-byte Data payload
static void spi_write_data(uint8_t data) {
    nrf_gpio_pin_set(LCD_DC); // DC HIGH = Data
    nrf_gpio_pin_clear(LCD_CS);
    nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_TX(&data, 1);
    nrfx_spim_xfer(&spi, &xfer, 0);
    nrf_gpio_pin_set(LCD_CS);
}

void lcd_init(void) {
    // 1. Setup Control Pins
    nrf_gpio_cfg_output(LCD_CS);
    nrf_gpio_cfg_output(LCD_DC);
    nrf_gpio_cfg_output(LCD_RST);
    nrf_gpio_pin_set(LCD_CS); // Deselect screen initially

    // 2. Setup High-Speed SPI (8 MHz)
    nrfx_spim_config_t spi_config = NRFX_SPIM_DEFAULT_CONFIG;
    spi_config.sck_pin  = EDGE_P13;
    spi_config.mosi_pin = EDGE_P15;
    spi_config.miso_pin = NRFX_SPIM_PIN_NOT_USED; // We only send data, no reading
    spi_config.frequency = NRF_SPIM_FREQ_8M;
    spi_config.mode = NRF_SPIM_MODE_0;
    APP_ERROR_CHECK(nrfx_spim_init(&spi, &spi_config, NULL, NULL));

    // 3. Hardware Reset
    nrf_gpio_pin_clear(LCD_RST);
    nrf_delay_ms(10);
    nrf_gpio_pin_set(LCD_RST);
    nrf_delay_ms(120);

    // 4. ILI9341 Wake-Up Sequence
    spi_write_cmd(0x01); // Software Reset
    nrf_delay_ms(150);

    spi_write_cmd(0x11); // Sleep Out
    nrf_delay_ms(500);

    spi_write_cmd(0x3A); // Pixel Format Set
    spi_write_data(0x55); // 16-bit RGB565 color format

    spi_write_cmd(0x36); // Memory Access Control
    spi_write_data(0x48); // Standard rotation

    spi_write_cmd(0x29); // Display ON
    nrf_delay_ms(100);
}

// Helper to define which box of pixels we are drawing in
static void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    spi_write_cmd(0x2A); // Column Address
    spi_write_data(x0 >> 8); spi_write_data(x0 & 0xFF);
    spi_write_data(x1 >> 8); spi_write_data(x1 & 0xFF);

    spi_write_cmd(0x2B); // Page Address
    spi_write_data(y0 >> 8); spi_write_data(y0 & 0xFF);
    spi_write_data(y1 >> 8); spi_write_data(y1 & 0xFF);

    spi_write_cmd(0x2C); // Memory Write (Prepare to receive pixels)
}

void lcd_fill_screen(uint16_t color) {
    lcd_set_window(0, 0, 239, 319); // Target the whole screen
    
    nrf_gpio_pin_set(LCD_DC);
    nrf_gpio_pin_clear(LCD_CS);

    // Buffer 16 pixels at a time so the micro:bit DMA is fast
    uint8_t buf[32]; 
    for(int i = 0; i < 16; i++) {
        buf[i*2]     = color >> 8;
        buf[i*2 + 1] = color & 0xFF;
    }

    // Blast the buffer to the screen to fill 76,800 pixels
    for (uint32_t i = 0; i < (240 * 320) / 16; i++) {
        nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_TX(buf, 32);
        nrfx_spim_xfer(&spi, &xfer, 0);
    }
    
    nrf_gpio_pin_set(LCD_CS);
}