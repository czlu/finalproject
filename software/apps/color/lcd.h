#ifndef LCD_H
#define LCD_H

#include <stdint.h>

// Standard 16-bit RGB565 Colors
#define COLOR_BLACK   0x0000
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_WHITE   0xFFFF
#define COLOR_YELLOW  0xFFE0
#define COLOR_ORANGE  0xFD20
#define COLOR_GRAY    0x7BEF
#define COLOR_DARK    0x2104

// Setup the SPI pins and boot the screen
void lcd_init(void);

// Fill the entire 240x320 screen with one color
void lcd_fill_screen(uint16_t color);

// Fill a rectangle with a color
void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

// Draw a single character (8x16 font) at pixel position
void lcd_draw_char(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg);

// Draw a string at pixel position
void lcd_draw_string(uint16_t x, uint16_t y, const char* str, uint16_t fg, uint16_t bg);

// Draw a string with 2x scaled font (16x32)
void lcd_draw_string_2x(uint16_t x, uint16_t y, const char* str, uint16_t fg, uint16_t bg);

// Draw a filled rectangle
void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

#endif
