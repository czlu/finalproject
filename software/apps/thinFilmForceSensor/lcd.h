#ifndef LCD_H
#define LCD_H

#include <stdint.h>

// Standard 16-bit RGB565 Colors
#define COLOR_BLACK   0x0000
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_WHITE   0xFFFF

// Setup the SPI pins and boot the screen
void lcd_init(void);

// Fill the entire 240x320 screen with one color
void lcd_fill_screen(uint16_t color);

#endif