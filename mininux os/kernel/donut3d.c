#include "donut3d.h"

#include <stdint.h>

#include "framebuffer.h"

#define DONUT_WIDTH 48u
#define DONUT_HEIGHT 20u
#define ANGLE_COUNT 64u
#define FIXED_ONE 256

static const int16_t sin_table[ANGLE_COUNT] = {
    0, 25, 50, 74, 98, 121, 142, 162,
    181, 198, 213, 226, 237, 245, 251, 255,
    256, 255, 251, 245, 237, 226, 213, 198,
    181, 162, 142, 121, 98, 74, 50, 25,
    0, -25, -50, -74, -98, -121, -142, -162,
    -181, -198, -213, -226, -237, -245, -251, -255,
    -256, -255, -251, -245, -237, -226, -213, -198,
    -181, -162, -142, -121, -98, -74, -50, -25
};

static int32_t fixed_mul(int32_t left, int32_t right)
{
    return (left * right) / FIXED_ONE;
}

void donut3d_draw(uint32_t x, uint32_t y, uint32_t frame)
{
    static char pixels[DONUT_WIDTH * DONUT_HEIGHT];
    static uint16_t depth[DONUT_WIDTH * DONUT_HEIGHT];
    static const char shades[] = ".:-+*#@";
    const uint32_t angle_x = frame & (ANGLE_COUNT - 1u);
    const uint32_t angle_z = (frame * 2u) & (ANGLE_COUNT - 1u);
    const int32_t sin_x = sin_table[angle_x];
    const int32_t cos_x = sin_table[(angle_x + 16u) & (ANGLE_COUNT - 1u)];
    const int32_t sin_z = sin_table[angle_z];
    const int32_t cos_z = sin_table[(angle_z + 16u) & (ANGLE_COUNT - 1u)];
    uint32_t index;
    uint32_t theta;

    for (index = 0u; index < DONUT_WIDTH * DONUT_HEIGHT; ++index) {
        pixels[index] = ' ';
        depth[index] = UINT16_MAX;
    }

    for (theta = 0u; theta < ANGLE_COUNT; theta += 2u) {
        const int32_t sin_theta = sin_table[theta];
        const int32_t cos_theta = sin_table[(theta + 16u) & (ANGLE_COUNT - 1u)];
        uint32_t phi;

        for (phi = 0u; phi < ANGLE_COUNT; ++phi) {
            const int32_t cos_phi = sin_table[(phi + 16u) & (ANGLE_COUNT - 1u)];
            const int32_t sin_phi = sin_table[phi];
            const int32_t ring_radius = 512 + cos_phi;
            const int32_t point_x = fixed_mul(ring_radius, cos_theta);
            const int32_t point_y = fixed_mul(ring_radius, sin_theta);
            const int32_t rotated_x = fixed_mul(point_x, cos_z) -
                                      fixed_mul(point_y, sin_z);
            const int32_t rotated_y = fixed_mul(point_x, sin_z) +
                                      fixed_mul(point_y, cos_z);
            const int32_t rotated_y_x = fixed_mul(rotated_y, cos_x) -
                                        fixed_mul(sin_phi, sin_x);
            const int32_t rotated_z_x = fixed_mul(rotated_y, sin_x) +
                                        fixed_mul(sin_phi, cos_x);
            const int32_t distance = 1280 - rotated_z_x;
            int32_t screen_x;
            int32_t screen_y;
            const int32_t normal_x = fixed_mul(cos_phi, cos_theta);
            const int32_t normal_y = fixed_mul(cos_phi, sin_theta);
            const int32_t lit_x = fixed_mul(normal_x, cos_z) -
                                  fixed_mul(normal_y, sin_z);
            const int32_t lit_y = fixed_mul(normal_x, sin_z) +
                                  fixed_mul(normal_y, cos_z);
            const int32_t lit_y_x = fixed_mul(lit_y, cos_x) -
                                    fixed_mul(sin_phi, sin_x);
            const int32_t lit_z_x = fixed_mul(lit_y, sin_x) +
                                    fixed_mul(sin_phi, cos_x);
            const int32_t brightness = fixed_mul(lit_x, -102) +
                                       fixed_mul(lit_y_x, 179) +
                                       fixed_mul(lit_z_x, -154);
            uint32_t shade;
            uint32_t pixel_index;

            if (distance <= 0) {
                continue;
            }
            screen_x = (int32_t)(DONUT_WIDTH / 2u) +
                       (rotated_x * 18) / distance;
            screen_y = (int32_t)(DONUT_HEIGHT / 2u) -
                       (rotated_y_x * 8) / distance;

            if (brightness <= 0 || screen_x < 0 || screen_y < 0 ||
                screen_x >= (int32_t)DONUT_WIDTH ||
                screen_y >= (int32_t)DONUT_HEIGHT) {
                continue;
            }

            pixel_index = (uint32_t)screen_y * DONUT_WIDTH + (uint32_t)screen_x;
            if ((uint32_t)distance >= depth[pixel_index]) {
                continue;
            }

            shade = (uint32_t)(brightness * (int32_t)(sizeof(shades) - 2u)) / 256u;
            if (shade >= sizeof(shades) - 1u) {
                shade = sizeof(shades) - 2u;
            }
            pixels[pixel_index] = shades[shade];
            depth[pixel_index] = (uint16_t)distance;
        }
    }

    for (index = 0u; index < DONUT_HEIGHT; ++index) {
        uint32_t column;
        for (column = 0u; column < DONUT_WIDTH; ++column) {
            const char pixel = pixels[index * DONUT_WIDTH + column];
            framebuffer_draw_char(x + column * 8u, y + index * 10u,
                                  pixel, FRAMEBUFFER_COLOR_CYAN);
        }
    }
}
