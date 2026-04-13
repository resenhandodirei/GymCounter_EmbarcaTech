#ifndef SSD1306_H
#define SSD1306_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// ---------------------------------------------------------------------------
// Configuração de hardware (BitDogLab)
// ---------------------------------------------------------------------------
#define SSD1306_I2C_PORT    i2c1
#define SSD1306_I2C_ADDR    0x3C
#define SSD1306_SDA_PIN     14
#define SSD1306_SCL_PIN     15
#define SSD1306_WIDTH       128
#define SSD1306_HEIGHT      64

// ---------------------------------------------------------------------------
// Comandos SSD1306
// ---------------------------------------------------------------------------
#define SSD1306_SET_CONTRAST        0x81
#define SSD1306_DISPLAY_ALL_ON_RES  0xA4
#define SSD1306_NORMAL_DISPLAY      0xA6
#define SSD1306_DISPLAY_OFF         0xAE
#define SSD1306_DISPLAY_ON          0xAF
#define SSD1306_SET_DISPLAY_OFFSET  0xD3
#define SSD1306_SET_COM_PINS        0xDA
#define SSD1306_SET_VCOM_DETECT     0xDB
#define SSD1306_SET_DISPLAY_CLK     0xD5
#define SSD1306_SET_PRECHARGE       0xD9
#define SSD1306_SET_MUX             0xA8
#define SSD1306_SET_LOW_COLUMN      0x00
#define SSD1306_SET_HIGH_COLUMN     0x10
#define SSD1306_SET_START_LINE      0x40
#define SSD1306_MEMORY_MODE         0x20
#define SSD1306_COLUMN_ADDR         0x21
#define SSD1306_PAGE_ADDR           0x22
#define SSD1306_COM_SCAN_INC        0xC0
#define SSD1306_COM_SCAN_DEC        0xC8
#define SSD1306_SEG_REMAP           0xA0
#define SSD1306_CHARGE_PUMP         0x8D

// ---------------------------------------------------------------------------
// Funções públicas
// ---------------------------------------------------------------------------
void ssd1306_init(void);
void ssd1306_clear(void);
void ssd1306_show(void);
void ssd1306_draw_pixel(int x, int y, bool on);
void ssd1306_draw_char(int x, int y, char c, int scale);
void ssd1306_draw_string(int x, int y, const char *str, int scale);
void ssd1306_draw_rect(int x, int y, int w, int h, bool filled);
void ssd1306_draw_line(int x0, int y0, int x1, int y1);

#endif 