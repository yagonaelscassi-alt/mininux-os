#include "ps2.h"

#include <stdint.h>

static inline uint8_t io_in8(uint16_t port)
{
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void io_out8(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static uint8_t ps2_wait_read(void)
{
    uint32_t timeout = 100000u;
    while (timeout-- > 0u) {
        if ((io_in8(0x64u) & 0x01u) != 0u) {
            return 1u;
        }
    }
    return 0u;
}

static int ps2_wait_write(void)
{
    uint32_t timeout = 100000u;
    while (timeout-- > 0u) {
        if ((io_in8(0x64u) & 0x02u) == 0u) {
            return 1;
        }
    }
    return 0;
}

static int ps2_mouse_command(uint8_t command)
{
    if (!ps2_wait_write()) {
        return 0;
    }
    io_out8(0x64u, 0xD4u);
    if (!ps2_wait_write()) {
        return 0;
    }
    io_out8(PS2_DATA_PORT, command);
    if (!ps2_wait_read()) {
        return 0;
    }
    return io_in8(PS2_DATA_PORT) == 0xFAu;
}

int ps2_mouse_init(void)
{
    uint8_t config;

    __asm__ volatile ("cli");
    if (!ps2_wait_write()) {
        return 0;
    }
    io_out8(PS2_COMMAND_PORT, 0xADu);
    if (!ps2_wait_write()) {
        return 0;
    }
    io_out8(PS2_COMMAND_PORT, 0xA7u);
    if (!ps2_wait_write()) {
        return 0;
    }
    io_out8(PS2_COMMAND_PORT, 0x20u);
    if (!ps2_wait_read()) {
        return 0;
    }
    config = io_in8(PS2_DATA_PORT);
    config &= (uint8_t)~0x23u;
    if (!ps2_wait_write()) {
        return 0;
    }
    io_out8(PS2_COMMAND_PORT, 0x60u);
    if (!ps2_wait_write()) {
        return 0;
    }
    io_out8(PS2_DATA_PORT, config);
    if (!ps2_wait_write()) {
        return 0;
    }
    io_out8(PS2_COMMAND_PORT, 0xA8u);
    if (!ps2_mouse_command(0xF4u)) {
        return 0;
    }
    return 1;
}

int ps2_init(void)
{
    uint8_t config;
    uint32_t drain_count = 32u;

    __asm__ volatile ("cli");
    while (drain_count-- > 0u && (io_in8(PS2_COMMAND_PORT) & 0x01u) != 0u) {
        (void)io_in8(PS2_DATA_PORT);
    }

    if (!ps2_wait_write()) {
        return 0;
    }
    io_out8(PS2_COMMAND_PORT, 0xADu);
    if (!ps2_wait_write()) {
        return 0;
    }
    io_out8(PS2_COMMAND_PORT, 0xA7u);
    if (!ps2_wait_write()) {
        return 0;
    }
    io_out8(PS2_COMMAND_PORT, 0x20u);
    if (!ps2_wait_read()) {
        return 0;
    }
    config = io_in8(PS2_DATA_PORT);
    config = (uint8_t)((config & (uint8_t)~0x33u) | 0x40u);

    if (!ps2_wait_write()) {
        return 0;
    }
    io_out8(PS2_COMMAND_PORT, 0x60u);
    if (!ps2_wait_write()) {
        return 0;
    }
    io_out8(PS2_DATA_PORT, config);
    if (!ps2_wait_write()) {
        return 0;
    }
    io_out8(PS2_COMMAND_PORT, 0xAEu);
    if (!ps2_wait_write()) {
        return 0;
    }
    io_out8(PS2_COMMAND_PORT, 0xA8u);

    if (!ps2_wait_write()) {
        return 0;
    }
    io_out8(PS2_DATA_PORT, 0xF4u);
    if (!ps2_wait_read() || io_in8(PS2_DATA_PORT) != 0xFAu) {
        return 0;
    }

    return ps2_mouse_command(0xF4u);
}

uint8_t ps2_poll_events(int16_t *delta_x, int16_t *delta_y,
                        uint8_t *buttons, uint8_t *scan_code)
{
    static uint8_t packet[3];
    static uint8_t packet_index;
    uint8_t events = 0u;
    uint32_t budget = 16u;

    while (budget-- > 0u) {
        const uint8_t status = io_in8(PS2_COMMAND_PORT);
        uint8_t value;

        if ((status & 0x01u) == 0u) {
            break;
        }
        value = io_in8(PS2_DATA_PORT);

        if ((status & 0x20u) == 0u) {
            *scan_code = value;
            return (uint8_t)(events | PS2_EVENT_KEYBOARD);
        }
        if (packet_index == 0u && (value & 0x08u) == 0u) {
            continue;
        }

        packet[packet_index++] = value;
        if (packet_index == 3u) {
            packet_index = 0u;
            *buttons = packet[0] & 0x07u;
            *delta_x = (packet[0] & 0x40u) != 0u ? 0 : (int8_t)packet[1];
            *delta_y = (packet[0] & 0x80u) != 0u ? 0 : -(int8_t)packet[2];
            events |= PS2_EVENT_MOUSE;
        }
    }

    return events;
}

int ps2_mouse_poll(int16_t *delta_x, int16_t *delta_y, uint8_t *buttons)
{
    static uint8_t packet[3];
    static uint8_t packet_index;
    uint32_t budget = 16u;

    while (budget-- > 0u) {
        const uint8_t status = io_in8(PS2_COMMAND_PORT);
        if ((status & 0x01u) == 0u) {
            break;
        }

        const uint8_t value = io_in8(PS2_DATA_PORT);
        if ((status & 0x20u) == 0u) {
            continue;
        }
        if (packet_index == 0u && (value & 0x08u) == 0u) {
            continue;
        }

        packet[packet_index++] = value;
        if (packet_index == 3u) {
            packet_index = 0u;
            *buttons = packet[0] & 0x07u;
            *delta_x = (packet[0] & 0x40u) != 0u ? 0 : (int8_t)packet[1];
            *delta_y = (packet[0] & 0x80u) != 0u ? 0 : -(int8_t)packet[2];
            return 1;
        }
    }

    return 0;
}

uint8_t ps2_read_byte(void)
{
    if (!ps2_wait_read()) {
        return 0u;
    }
    return io_in8(PS2_DATA_PORT);
}

int ps2_mouse_available(void)
{
    return (int)((io_in8(0x64u) & 0x01u) != 0u);
}

uint8_t ps2_mouse_read(void)
{
    if (!ps2_wait_read()) {
        return 0u;
    }
    return io_in8(PS2_DATA_PORT);
}
