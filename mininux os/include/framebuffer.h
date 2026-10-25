#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>

#define FRAMEBUFFER_COLOR_BLACK    0x00000000u
#define FRAMEBUFFER_COLOR_WHITE    0x00FFFFFFu
#define FRAMEBUFFER_COLOR_GREEN    0x0000FF00u
#define FRAMEBUFFER_COLOR_RED      0x00FF0000u
#define FRAMEBUFFER_COLOR_CYAN     0x0000FFFFu

typedef struct {
    uintptr_t address;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t bpp;
    uint8_t type;
    uint8_t red_position;
    uint8_t green_position;
    uint8_t blue_position;
} framebuffer_info_t;

void framebuffer_init_from_multiboot(uint32_t multiboot_info_ptr);
uint32_t framebuffer_get_width(void);
uint32_t framebuffer_get_height(void);
void framebuffer_clear(uint32_t color);
void framebuffer_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void framebuffer_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);
void framebuffer_draw_string(uint32_t x, uint32_t y, const char *text, uint32_t color);
void framebuffer_draw_char(uint32_t x, uint32_t y, char ch, uint32_t color);

#endif
