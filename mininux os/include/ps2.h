#ifndef PS2_H
#define PS2_H

#include <stdint.h>

#define PS2_COMMAND_PORT 0x64u
#define PS2_DATA_PORT 0x60u
#define PS2_EVENT_MOUSE 0x01u
#define PS2_EVENT_KEYBOARD 0x02u

int ps2_init(void);
uint8_t ps2_poll_events(int16_t *delta_x, int16_t *delta_y,
						uint8_t *buttons, uint8_t *scan_code);
uint8_t ps2_read_byte(void);
int ps2_mouse_available(void);
uint8_t ps2_mouse_read(void);

#endif
