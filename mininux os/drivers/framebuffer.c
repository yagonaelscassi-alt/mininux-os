#include "framebuffer.h"

#include <stdint.h>

#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER 8u
#define MULTIBOOT_FRAMEBUFFER_TYPE_RGB 1u

static framebuffer_info_t g_framebuffer = {0};

static uint32_t framebuffer_get_u32(const uint8_t *ptr)
{
    return (uint32_t)ptr[0] | ((uint32_t)ptr[1] << 8) | ((uint32_t)ptr[2] << 16) | ((uint32_t)ptr[3] << 24);
}

static uint64_t framebuffer_get_u64(const uint8_t *ptr)
{
    return (uint64_t)framebuffer_get_u32(ptr) |
           ((uint64_t)framebuffer_get_u32(ptr + 4) << 32);
}

static void framebuffer_parse_multiboot2(uint32_t multiboot_info_ptr)
{
    const uint8_t *base = (const uint8_t *)(uintptr_t)multiboot_info_ptr;
    const uint32_t total_size = framebuffer_get_u32(base);
    const uint8_t *ptr = base + 8u;
    uint32_t offset = 8u;

    while (offset <= total_size && total_size - offset >= 8u) {
        const uint32_t tag_type = framebuffer_get_u32(ptr);
        const uint32_t tag_size = framebuffer_get_u32(ptr + 4);

        if (tag_size < 8u || tag_size > total_size - offset) {
            return;
        }

        if (tag_type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
            if (tag_size < 38u) {
                return;
            }

            const uint64_t address = framebuffer_get_u64(ptr + 8);
            const uint32_t pitch = framebuffer_get_u32(ptr + 16);
            const uint32_t width = framebuffer_get_u32(ptr + 20);
            const uint32_t height = framebuffer_get_u32(ptr + 24);
            const uint8_t bpp = ptr[28];
            const uint8_t type = ptr[29];
            const uint8_t red_position = ptr[32];
            const uint8_t green_position = ptr[34];
            const uint8_t blue_position = ptr[36];

            if (type != MULTIBOOT_FRAMEBUFFER_TYPE_RGB || bpp != 32u ||
                address > UINTPTR_MAX || width == 0u ||
                width > UINT32_MAX / 4u || pitch < width * 4u ||
                height == 0u || red_position > 24u ||
                green_position > 24u || blue_position > 24u ||
                ptr[33] != 8u || ptr[35] != 8u || ptr[37] != 8u) {
                return;
            }

            g_framebuffer.address = address;
            g_framebuffer.pitch = pitch;
            g_framebuffer.width = width;
            g_framebuffer.height = height;
            g_framebuffer.bpp = bpp;
            g_framebuffer.type = type;
            g_framebuffer.red_position = red_position;
            g_framebuffer.green_position = green_position;
            g_framebuffer.blue_position = blue_position;
            return;
        }

        if (tag_type == 0u) {
            break;
        }

        const uint32_t aligned_size = (tag_size + 7u) & ~7u;
        if (aligned_size > total_size - offset) {
            return;
        }
        ptr += aligned_size;
        offset += aligned_size;
    }
}

void framebuffer_init_from_multiboot(uint32_t multiboot_info_ptr)
{
    if (multiboot_info_ptr == 0u) {
        return;
    }

    framebuffer_parse_multiboot2(multiboot_info_ptr);
}

uint32_t framebuffer_get_width(void)
{
    return g_framebuffer.width;
}

uint32_t framebuffer_get_height(void)
{
    return g_framebuffer.height;
}

void framebuffer_clear(uint32_t color)
{
    if (g_framebuffer.address == 0u) {
        return;
    }

    uint32_t x;
    uint32_t y;
    for (y = 0; y < g_framebuffer.height; ++y) {
        for (x = 0; x < g_framebuffer.width; ++x) {
            framebuffer_put_pixel(x, y, color);
        }
    }
}

void framebuffer_put_pixel(uint32_t x, uint32_t y, uint32_t color)
{
    if (g_framebuffer.address == 0u) {
        return;
    }

    if (x >= g_framebuffer.width || y >= g_framebuffer.height) {
        return;
    }

    const uint32_t red = (color >> 16) & 0xFFu;
    const uint32_t green = (color >> 8) & 0xFFu;
    const uint32_t blue = color & 0xFFu;
    const uint32_t pixel = (red << g_framebuffer.red_position) |
                           (green << g_framebuffer.green_position) |
                           (blue << g_framebuffer.blue_position);
    volatile uint32_t *framebuffer = (volatile uint32_t *)g_framebuffer.address;
    const uint32_t offset = (y * g_framebuffer.pitch / 4u) + x;
    framebuffer[offset] = pixel;
}

void framebuffer_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color)
{
    uint32_t iy;
    uint32_t ix;

    for (iy = 0; iy < height; ++iy) {
        for (ix = 0; ix < width; ++ix) {
            framebuffer_put_pixel(x + ix, y + iy, color);
        }
    }
}

void framebuffer_draw_char(uint32_t x, uint32_t y, char ch, uint32_t color)
{
    static const uint8_t font8x8[128][8] = {
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
    };

    static const uint8_t font5x7[45][7] = {
        {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
        {14,17,16,16,16,17,14}, {30,17,17,17,17,17,30},
        {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
        {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17},
        {14,4,4,4,4,4,14}, {7,2,2,2,18,18,12},
        {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17},
        {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
        {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
        {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},
        {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
        {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31},
        {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14},
        {14,17,1,2,4,8,31}, {30,1,1,14,1,1,30},
        {2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
        {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8},
        {14,17,17,14,17,17,14}, {14,17,17,15,1,1,14},
        {1,2,2,4,8,8,16}, {0,0,0,0,0,0,31},
        {0,4,4,0,4,4,0}, {0,0,0,31,0,0,0},
        {0,0,0,0,0,12,12}, {0,4,4,31,4,4,0},
        {0,21,14,31,14,21,0}, {10,31,10,10,31,10,0},
        {14,17,23,21,23,16,14}
    };

    unsigned char character = (unsigned char)ch;
    const uint8_t *glyph = font8x8[character];
    uint32_t glyph_width = 8u;
    uint32_t glyph_height = 8u;
    uint32_t bit_offset = 7u;
    if (character >= 'a' && character <= 'z') {
        character = (unsigned char)(character - 'a' + 'A');
    }
    if (character >= 'A' && character <= 'Z') {
        glyph = font5x7[character - 'A'];
        glyph_width = 5u;
        glyph_height = 7u;
        bit_offset = 4u;
    } else if (character >= '0' && character <= '9') {
        glyph = font5x7[26u + character - '0'];
        glyph_width = 5u;
        glyph_height = 7u;
        bit_offset = 4u;
    } else if (character == '/') {
        glyph = font5x7[36];
        glyph_width = 5u;
        glyph_height = 7u;
        bit_offset = 4u;
    } else if (character == '_') {
        glyph = font5x7[37];
        glyph_width = 5u;
        glyph_height = 7u;
        bit_offset = 4u;
    } else if (character == ':') {
        glyph = font5x7[38];
        glyph_width = 5u;
        glyph_height = 7u;
        bit_offset = 4u;
    } else if (character == '-') {
        glyph = font5x7[39];
        glyph_width = 5u;
        glyph_height = 7u;
        bit_offset = 4u;
    } else if (character == '.') {
        glyph = font5x7[40];
        glyph_width = 5u;
        glyph_height = 7u;
        bit_offset = 4u;
    } else if (character == '+') {
        glyph = font5x7[41];
        glyph_width = 5u;
        glyph_height = 7u;
        bit_offset = 4u;
    } else if (character == '*') {
        glyph = font5x7[42];
        glyph_width = 5u;
        glyph_height = 7u;
        bit_offset = 4u;
    } else if (character == '#') {
        glyph = font5x7[43];
        glyph_width = 5u;
        glyph_height = 7u;
        bit_offset = 4u;
    } else if (character == '@') {
        glyph = font5x7[44];
        glyph_width = 5u;
        glyph_height = 7u;
        bit_offset = 4u;
    }
    uint32_t row;
    uint32_t col;

    for (row = 0; row < glyph_height; ++row) {
        for (col = 0; col < glyph_width; ++col) {
            if ((glyph[row] >> (bit_offset - col)) & 1u) {
                framebuffer_put_pixel(x + col, y + row, color);
            }
        }
    }
}

void framebuffer_draw_string(uint32_t x, uint32_t y, const char *text, uint32_t color)
{
    uint32_t cursor_x = x;
    const char *current = text;

    while (*current != '\0') {
        if (*current == '\n') {
            y += 10u;
            cursor_x = x;
        } else {
            framebuffer_draw_char(cursor_x, y, *current, color);
            cursor_x += 8u;
        }
        ++current;
    }
}
