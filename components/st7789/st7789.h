#ifndef ST7789_H
#define ST7789_H

#include "esp_err.h"
#include "pinout.h"

/**
 * @file st7789.h
 * @brief ST7789 TFT Display Driver for ESP32
 * 
 * High-performance driver for ST7789-based 240x240 TFT displays.
 * Optimized for displays without CS pin using SPI communication.
 * 
 * GPIO pin assignments are defined in pinout.h for centralized management.
 */ 

/**
 * @brief Initialize the ST7789 display driver
 * 
 * Configures GPIO pins, initializes SPI communication, performs hardware reset,
 * and sends the complete initialization sequence to the ST7789 controller.
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t st7789_init(void);

/**
 * @brief Draw a single pixel at specified coordinates
 * 
 * @param x X coordinate (0-239)
 * @param y Y coordinate (0-239) 
 * @param color 16-bit RGB565 color value
 */
void st7789_draw_pixel(uint16_t x, uint16_t y, uint16_t color);

/**
 * @brief Fill a rectangular area with specified color
 * 
 * @param x X coordinate of top-left corner
 * @param y Y coordinate of top-left corner
 * @param w Width of rectangle
 * @param h Height of rectangle
 * @param color 16-bit RGB565 color value
 */
void st7789_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * @brief Draw a single character at specified position
 * 
 * @param x X coordinate for character placement
 * @param y Y coordinate for character placement  
 * @param c Character to draw (printable ASCII 32-126)
 * @param color 16-bit RGB565 foreground color
 * @param bg_color 16-bit RGB565 background color
 */
void st7789_draw_char(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg_color);

/**
 * @brief Draw a text string at specified position
 * 
 * Supports newline (\n) and carriage return (\r) characters.
 * Automatically wraps text to next line if it exceeds display width.
 * 
 * @param x X coordinate for text start position
 * @param y Y coordinate for text start position
 * @param str Null-terminated string to draw
 * @param color 16-bit RGB565 foreground color
 * @param bg_color 16-bit RGB565 background color
 */
void st7789_draw_string(uint16_t x, uint16_t y, const char* str, uint16_t color, uint16_t bg_color);

/**
 * @brief Clear entire screen with specified color
 * 
 * @param color 16-bit RGB565 color value
 */
void st7789_clear_screen(uint16_t color);

/**
 * @brief Draw a single large character (16x16) at specified position
 * 
 * Supports numbers (0-9), period (.), percent (%), colon (:), and selected uppercase letters (A,C,D,E,H,I,M,N,P,R,S,T,U,Y).
 * Perfect for displaying sensor readings and labels.
 * 
 * @param x X coordinate for character placement
 * @param y Y coordinate for character placement  
 * @param c Character to draw (supported characters only)
 * @param color 16-bit RGB565 foreground color
 * @param bg_color 16-bit RGB565 background color
 */
void st7789_draw_large_char(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg_color);

/**
 * @brief Draw a text string with large font (16x16)
 * 
 * Supports newline (\n) and carriage return (\r) characters.
 * Automatically wraps text to next line if it exceeds display width.
 * Character set includes numbers, period, percent, colon, and selected uppercase letters.
 * 
 * @param x X coordinate for text start position
 * @param y Y coordinate for text start position
 * @param str Null-terminated string to draw (supported characters only)
 * @param color 16-bit RGB565 foreground color
 * @param bg_color 16-bit RGB565 background color
 */
void st7789_draw_large_string(uint16_t x, uint16_t y, const char* str, uint16_t color, uint16_t bg_color);

/**
 * @brief Run display functionality test
 * 
 * Executes a comprehensive test sequence including full-screen color fills
 * and multi-color patterns to verify display functionality and color accuracy.
 */
void st7789_test(void);

/**
 * @brief Test large font functionality with sensor-style display
 * 
 * Demonstrates the 16x16 large font capabilities by showing sample
 * sensor readings and testing the complete character set.
 */
void st7789_large_font_test(void);

// Common RGB565 color definitions for convenience
#define ST7789_BLACK   0x0000  // Black
#define ST7789_WHITE   0xFFFF  // White  
#define ST7789_RED     0xF800  // Red
#define ST7789_GREEN   0x07E0  // Green
#define ST7789_BLUE    0x001F  // Blue
#define ST7789_YELLOW  0xFFE0  // Yellow
#define ST7789_CYAN    0x07FF  // Cyan
#define ST7789_MAGENTA 0xF81F  // Magenta

#endif // ST7789_H