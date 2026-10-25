#include <stdint.h>

#include "donut3d.h"
#include "framebuffer.h"
#include "ps2.h"

#define MULTIBOOT2_BOOT_MAGIC 0x36D76289u
#define VGA_WIDTH 80u
#define VGA_HEIGHT 25u
#define VGA_MEMORY ((volatile uint16_t *)0xB8000)
#define VGA_COLOR 0x0Fu
#define COLOR_BACKGROUND 0x00121D27u
#define COLOR_PANEL 0x001B2A36u
#define COLOR_PANEL_LIGHT 0x00263743u
#define COLOR_ACCENT 0x0000BFA5u
#define COLOR_TEXT 0x00E8F0F2u
#define COLOR_MUTED 0x0095A8B2u
#define COLOR_RED 0x00E45B5Bu
#define COLOR_BLUE 0x005B9DE6u
#define COLOR_GREEN 0x0000C27Au
#define APP_NONE 0u
#define APP_TERMINAL 1u
#define APP_NOTEPAD 2u
#define APP_COMPUTER 3u
#define APP_SETTINGS 4u
#define APP_BROWSER 5u
#define APP_CALCULATOR 6u
#define APP_PAINT 7u
#define TERMINAL_LINE_COUNT 10u
#define TERMINAL_LINE_LENGTH 64u
#define NOTE_LINE_COUNT 12u
#define NOTE_LINE_LENGTH 72u
#define PAINT_STROKE_CAPACITY 512u
#define COOKIE_GAME_COLUMNS 12u
#define COOKIE_GAME_ROWS 8u
#define COOKIE_GAME_TARGET 10u
#define COOKIE_GAME_CELL_SIZE 32u

typedef enum {
    APP_ICON_CALCULATOR = 0,
    APP_ICON_SETTINGS,
    APP_ICON_BROWSER,
    APP_ICON_TERMINAL,
    APP_ICON_PAINT,
    APP_ICON_COUNT
} app_icon_id_t;

typedef enum {
    ICON_CARPETA = 0,
    ICON_DESCARGAR,
    ICON_DOCUMENTOS,
    ICON_IMAGEN,
    ICON_IMAGENS,
    ICON_MUSICA,
    ICON_NIGUN,
    ICON_MUSICALES,
    ICON_TEXTOS,
    ICON_VIDEO,
    ICON_VIDEOS,
    ICON_COUNT
} icon_id_t;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t pixels[];
} wallpaper_image_t;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t pixels[];
} cursor_image_t;

extern const uint8_t minux_wallpaper_start[];
extern const uint8_t minux_wallpaper_arriune_start[];
extern const uint8_t minux_cursor_start[];
extern const uint8_t minux_icon_carpeta_start[];
extern const uint8_t minux_icon_descargar_start[];
extern const uint8_t minux_icon_documentos_start[];
extern const uint8_t minux_icon_imagen_start[];
extern const uint8_t minux_icon_imagens_start[];
extern const uint8_t minux_icon_musica_start[];
extern const uint8_t minux_icon_nigun_start[];
extern const uint8_t minux_icon_musicales_start[];
extern const uint8_t minux_icon_textos_start[];
extern const uint8_t minux_icon_video_start[];
extern const uint8_t minux_icon_videos_start[];
extern const uint8_t minux_app_icon_calculator_start[];
extern const uint8_t minux_app_icon_settings_start[];
extern const uint8_t minux_app_icon_browser_start[];
extern const uint8_t minux_app_icon_terminal_start[];
extern const uint8_t minux_app_icon_paint_start[];

static const cursor_image_t *const g_icons[ICON_COUNT] = {
    (const cursor_image_t *)minux_icon_carpeta_start,
    (const cursor_image_t *)minux_icon_descargar_start,
    (const cursor_image_t *)minux_icon_documentos_start,
    (const cursor_image_t *)minux_icon_imagen_start,
    (const cursor_image_t *)minux_icon_imagens_start,
    (const cursor_image_t *)minux_icon_musica_start,
    (const cursor_image_t *)minux_icon_nigun_start,
    (const cursor_image_t *)minux_icon_musicales_start,
    (const cursor_image_t *)minux_icon_textos_start,
    (const cursor_image_t *)minux_icon_video_start,
    (const cursor_image_t *)minux_icon_videos_start
};

static const cursor_image_t *const g_app_icons[APP_ICON_COUNT] = {
    (const cursor_image_t *)minux_app_icon_calculator_start,
    (const cursor_image_t *)minux_app_icon_settings_start,
    (const cursor_image_t *)minux_app_icon_browser_start,
    (const cursor_image_t *)minux_app_icon_terminal_start,
    (const cursor_image_t *)minux_app_icon_paint_start
};

typedef enum {
    BOOT_MODE_DESKTOP = 0,
    BOOT_MODE_DONUT3D,
    BOOT_MODE_COKIESGAME
} boot_mode_t;

static void console_write(const char *text)
{
    static uint32_t column;
    static uint32_t row;
    volatile uint16_t *const video = VGA_MEMORY;

    while (*text != '\0') {
        const char character = *text++;

        if (character == '\n') {
            column = 0;
            ++row;
        } else {
            video[row * VGA_WIDTH + column] =
                (uint16_t)character | ((uint16_t)VGA_COLOR << 8);
            ++column;

            if (column == VGA_WIDTH) {
                column = 0;
                ++row;
            }
        }

        if (row == VGA_HEIGHT) {
            row = 0;
        }
    }
}

static uint32_t g_screen_width;
static uint32_t g_screen_height;
static uint32_t g_active_app;
static uint32_t g_accent_color = COLOR_ACCENT;
static uint8_t g_wallpaper_selected;
static uint8_t g_launcher_open;
static uint8_t g_power_menu_open;
static uint8_t g_wifi_menu_open;
static uint8_t g_wifi_scan_requested;
static char g_terminal_lines[TERMINAL_LINE_COUNT][TERMINAL_LINE_LENGTH];
static uint32_t g_terminal_line_count;
static char g_terminal_input[TERMINAL_LINE_LENGTH];
static uint32_t g_terminal_input_length;
static char g_note_lines[NOTE_LINE_COUNT][NOTE_LINE_LENGTH];
static uint32_t g_note_line;
static uint32_t g_note_column;
static uint8_t g_shift_down;
static uint8_t g_extended_key;
static uint8_t g_donut3d_running;
static uint32_t g_donut3d_frame;
static uint8_t g_cookiesgame_running;
static uint8_t g_cookiesgame_over;
static uint8_t g_cookiesgame_won;
static uint8_t g_cookie_collected[COOKIE_GAME_TARGET];
static uint32_t g_cookies_collected_count;
static uint32_t g_player_x;
static uint32_t g_player_y;
static uint32_t g_monster_x;
static uint32_t g_monster_y;
static int32_t g_calculator_accumulator;
static int32_t g_calculator_entry;
static uint8_t g_calculator_operator;
static uint8_t g_calculator_new_entry = 1u;
static uint8_t g_calculator_error;
static struct {
    uint16_t x;
    uint16_t y;
    uint32_t color;
} g_paint_strokes[PAINT_STROKE_CAPACITY];
static uint32_t g_paint_stroke_count;
static uint32_t g_paint_color = FRAMEBUFFER_COLOR_BLACK;

static void draw_icon(icon_id_t icon, uint32_t x, uint32_t y, uint32_t size);
static void draw_app_icon(app_icon_id_t icon, uint32_t x, uint32_t y,
                          uint32_t size);
static void draw_wallpaper_preview(const wallpaper_image_t *image, uint32_t x,
                                   uint32_t y, uint32_t width,
                                   uint32_t height);
static void draw_computer_item(uint32_t x, uint32_t y, const char *label,
                               icon_id_t folder_icon, icon_id_t file_icon);

static void copy_text(char *destination, const char *source, uint32_t limit)
{
    uint32_t index = 0u;
    while (index + 1u < limit && source[index] != '\0') {
        destination[index] = source[index];
        ++index;
    }
    destination[index] = '\0';
}

static int text_equals(const char *left, const char *right)
{
    uint32_t index = 0u;
    while (left[index] != '\0' && right[index] != '\0') {
        char left_char = left[index];
        char right_char = right[index];
        if (left_char >= 'a' && left_char <= 'z') {
            left_char = (char)(left_char - 'a' + 'A');
        }
        if (right_char >= 'a' && right_char <= 'z') {
            right_char = (char)(right_char - 'a' + 'A');
        }
        if (left_char != right_char) {
            return 0;
        }
        ++index;
    }
    return left[index] == right[index];
}

static void terminal_append(const char *text)
{
    if (g_terminal_line_count == TERMINAL_LINE_COUNT) {
        uint32_t row;
        for (row = 1u; row < TERMINAL_LINE_COUNT; ++row) {
            copy_text(g_terminal_lines[row - 1u], g_terminal_lines[row], TERMINAL_LINE_LENGTH);
        }
        g_terminal_line_count = TERMINAL_LINE_COUNT - 1u;
    }
    copy_text(g_terminal_lines[g_terminal_line_count], text, TERMINAL_LINE_LENGTH);
    ++g_terminal_line_count;
}

static void terminal_clear(void)
{
    uint32_t row;
    for (row = 0u; row < TERMINAL_LINE_COUNT; ++row) {
        g_terminal_lines[row][0] = '\0';
    }
    g_terminal_line_count = 0u;
}

static void cookiesgame_start(void)
{
    uint32_t index;
    for (index = 0u; index < COOKIE_GAME_TARGET; ++index) {
        g_cookie_collected[index] = 0u;
    }
    g_cookies_collected_count = 0u;
    g_player_x = 0u;
    g_player_y = 0u;
    g_monster_x = COOKIE_GAME_COLUMNS - 1u;
    g_monster_y = COOKIE_GAME_ROWS - 1u;
    g_cookiesgame_over = 0u;
    g_cookiesgame_won = 0u;
    g_cookiesgame_running = 1u;
    g_donut3d_running = 0u;
}

static void calculator_clear(void)
{
    g_calculator_accumulator = 0;
    g_calculator_entry = 0;
    g_calculator_operator = 0u;
    g_calculator_new_entry = 1u;
    g_calculator_error = 0u;
}

static void calculator_digit(uint8_t digit)
{
    if (g_calculator_error) {
        calculator_clear();
    }
    if (g_calculator_new_entry) {
        g_calculator_entry = digit;
        g_calculator_new_entry = 0u;
    } else if (g_calculator_entry <= 99999999) {
        g_calculator_entry = g_calculator_entry * 10 + digit;
    }
}

static void calculator_apply_operator(uint8_t operation)
{
    g_calculator_accumulator = g_calculator_entry;
    g_calculator_operator = operation;
    g_calculator_new_entry = 1u;
}

static void calculator_equals(void)
{
    int64_t result;
    if (g_calculator_error || g_calculator_operator == 0u) {
        return;
    }
    switch (g_calculator_operator) {
    case '+':
        result = (int64_t)g_calculator_accumulator + g_calculator_entry;
        break;
    case '-':
        result = (int64_t)g_calculator_accumulator - g_calculator_entry;
        break;
    case '*':
        result = (int64_t)g_calculator_accumulator * g_calculator_entry;
        break;
    case '/':
        if (g_calculator_entry == 0) {
            g_calculator_error = 1u;
            return;
        }
        result = g_calculator_accumulator / g_calculator_entry;
        break;
    default:
        return;
    }
    if (result > 999999999 || result < -999999999) {
        g_calculator_error = 1u;
        return;
    }
    g_calculator_entry = (int32_t)result;
    g_calculator_accumulator = 0;
    g_calculator_operator = 0u;
    g_calculator_new_entry = 1u;
}

static void format_integer(int32_t value, char *buffer, uint32_t capacity)
{
    char reversed[12];
    uint32_t length = 0u;
    uint32_t output = 0u;
    uint32_t magnitude;

    if (capacity == 0u) {
        return;
    }
    if (value < 0) {
        buffer[output++] = '-';
        magnitude = (uint32_t)(-(int64_t)value);
    } else {
        magnitude = (uint32_t)value;
    }
    do {
        reversed[length++] = (char)('0' + magnitude % 10u);
        magnitude /= 10u;
    } while (magnitude != 0u && length < sizeof(reversed));
    while (length > 0u && output + 1u < capacity) {
        buffer[output++] = reversed[--length];
    }
    buffer[output] = '\0';
}

static int boot_token_equals(const uint8_t *text, uint32_t length,
                             const char *expected)
{
    uint32_t index = 0u;
    while (index < length && expected[index] != '\0') {
        char character = (char)text[index];
        if (character >= 'A' && character <= 'Z') {
            character = (char)(character - 'A' + 'a');
        }
        if (character != expected[index]) {
            return 0;
        }
        ++index;
    }
    return index == length && expected[index] == '\0';
}

static boot_mode_t parse_boot_mode(uint32_t multiboot_info)
{
    const uint8_t *info = (const uint8_t *)(uintptr_t)multiboot_info;
    uint32_t total_size;
    uint32_t offset;

    if (multiboot_info == 0u) {
        return BOOT_MODE_DESKTOP;
    }
    total_size = *(const uint32_t *)info;
    if (total_size < 16u || total_size > 0x00100000u) {
        return BOOT_MODE_DESKTOP;
    }

    for (offset = 8u; offset + 8u <= total_size;) {
        const uint32_t *tag = (const uint32_t *)(info + offset);
        const uint32_t type = tag[0];
        const uint32_t size = tag[1];
        uint32_t cursor;
        const uint8_t *command_line;
        uint32_t command_length;

        if (size < 8u || size > total_size - offset) {
            break;
        }
        if (type == 0u) {
            break;
        }
        if (type == 1u && size > 8u) {
            command_line = info + offset + 8u;
            command_length = size - 8u;
            for (cursor = 0u; cursor < command_length &&
                               command_line[cursor] != '\0';) {
                uint32_t token_start;
                while (cursor < command_length &&
                       (command_line[cursor] == ' ' ||
                        command_line[cursor] == '\0')) {
                    ++cursor;
                }
                token_start = cursor;
                while (cursor < command_length &&
                       command_line[cursor] != ' ' &&
                       command_line[cursor] != '\0') {
                    ++cursor;
                }
                if (boot_token_equals(command_line + token_start,
                                      cursor - token_start, "donut3d")) {
                    return BOOT_MODE_DONUT3D;
                }
                if (boot_token_equals(command_line + token_start,
                                      cursor - token_start, "cokiesgame") ||
                    boot_token_equals(command_line + token_start,
                                      cursor - token_start, "cookiesgame")) {
                    return BOOT_MODE_COKIESGAME;
                }
            }
        }
        offset = (offset + size + 7u) & ~7u;
    }
    return BOOT_MODE_DESKTOP;
}

static void terminal_execute(void)
{
    char command[TERMINAL_LINE_LENGTH];
    uint32_t index;

    for (index = 0u; index + 1u < TERMINAL_LINE_LENGTH; ++index) {
        command[index] = g_terminal_input[index];
        if (g_terminal_input[index] == '\0') {
            break;
        }
    }
    command[TERMINAL_LINE_LENGTH - 1u] = '\0';

    if (command[0] == '\0') {
        return;
    }
    if (text_equals(command, "CLEAR")) {
        terminal_clear();
        g_terminal_input[0] = '\0';
        g_terminal_input_length = 0u;
        return;
    }

    {
        char prompt[TERMINAL_LINE_LENGTH];
        copy_text(prompt, "MINUX: ", TERMINAL_LINE_LENGTH);
        index = 7u;
        while (index + 1u < TERMINAL_LINE_LENGTH && command[index - 7u] != '\0') {
            prompt[index] = command[index - 7u];
            ++index;
        }
        prompt[index] = '\0';
        terminal_append(prompt);
    }

    if (text_equals(command, "HELP")) {
        terminal_append("HELP ECHO CLEAR ABOUT DONUT3D");
        terminal_append("COKIESGAME: WASD MOVE, ESC EXIT");
        terminal_append("WIFI SCAN NEEDS A WIFI DRIVER");
    } else if (text_equals(command, "ABOUT") || text_equals(command, "VER")) {
        terminal_append("MINUX OS DEMO KERNEL");
    } else if (text_equals(command, "DONUT3D")) {
        g_cookiesgame_running = 0u;
        g_donut3d_running = 1u;
        g_donut3d_frame = 0u;
        terminal_append("DONUT3D RUNNING - PRESS ESC TO EXIT");
    } else if (text_equals(command, "COKIESGAME") ||
               text_equals(command, "COOKIESGAME")) {
        cookiesgame_start();
        terminal_append("COLLECT 10 COOKIES. AVOID THE MONSTER!");
    } else if (text_equals(command, "WIFI SCAN")) {
        terminal_append("SCAN UNAVAILABLE: NO WIFI DRIVER");
    } else if (text_equals(command, "WIFI")) {
        terminal_append("WI-FI UNAVAILABLE: NO ADAPTER DRIVER");
    } else if (command[0] == 'e' || command[0] == 'E') {
        if ((command[1] == 'c' || command[1] == 'C') &&
            (command[2] == 'h' || command[2] == 'H') &&
            (command[3] == 'o' || command[3] == 'O') && command[4] == ' ') {
            terminal_append(command + 5u);
        } else {
            terminal_append("UNKNOWN COMMAND TRY HELP");
        }
    } else {
        terminal_append("UNKNOWN COMMAND TRY HELP");
    }
    g_terminal_input[0] = '\0';
    g_terminal_input_length = 0u;
}

static char keyboard_char(uint8_t scan_code)
{
    static const char keymap[128] = {
        [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4',
        [0x06] = '5', [0x07] = '6', [0x08] = '7', [0x09] = '8',
        [0x0A] = '9', [0x0B] = '0', [0x0C] = '-', [0x0E] = '\b',
        [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r',
        [0x14] = 't', [0x15] = 'y', [0x16] = 'u', [0x17] = 'i',
        [0x18] = 'o', [0x19] = 'p', [0x1C] = '\n',
        [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd', [0x21] = 'f',
        [0x22] = 'g', [0x23] = 'h', [0x24] = 'j', [0x25] = 'k',
        [0x26] = 'l', [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c',
        [0x2F] = 'v', [0x30] = 'b', [0x31] = 'n', [0x32] = 'm',
        [0x39] = ' '
    };
    char character;

    if (scan_code >= 128u) {
        return '\0';
    }
    character = keymap[scan_code];
    if (g_shift_down && character >= 'a' && character <= 'z') {
        character = (char)(character - 'a' + 'A');
    }
    return character;
}

static void handle_key(uint8_t scan_code)
{
    if (scan_code == 0xE0u) {
        g_extended_key = 1u;
        return;
    }
    if (g_extended_key) {
        g_extended_key = 0u;
        return;
    }
    if (scan_code == 0x2Au || scan_code == 0x36u) {
        g_shift_down = 1u;
        return;
    }
    if (scan_code == 0xAAu || scan_code == 0xB6u) {
        g_shift_down = 0u;
        return;
    }
    if ((scan_code & 0x80u) != 0u) {
        return;
    }

    if (g_active_app == APP_CALCULATOR) {
        const char character = keyboard_char(scan_code);
        if (character >= '0' && character <= '9') {
            calculator_digit((uint8_t)(character - '0'));
        } else if (character == '\n') {
            calculator_equals();
        } else if (character == 'c' || character == 'C') {
            calculator_clear();
        } else if (character == '\b') {
            g_calculator_entry /= 10;
        }
        return;
    }

    if (g_active_app == APP_TERMINAL && g_cookiesgame_running) {
        char movement = keyboard_char(scan_code);
        uint32_t index;

        if (scan_code == 0x01u) {
            g_cookiesgame_running = 0u;
            terminal_append("COKIESGAME EXITED");
            return;
        }
        if (g_cookiesgame_over || g_cookiesgame_won) {
            if (movement == 'r' || movement == 'R') {
                cookiesgame_start();
            }
            return;
        }

        if (movement == 'w' && g_player_y > 0u) {
            --g_player_y;
        } else if (movement == 's' && g_player_y + 1u < COOKIE_GAME_ROWS) {
            ++g_player_y;
        } else if (movement == 'a' && g_player_x > 0u) {
            --g_player_x;
        } else if (movement == 'd' && g_player_x + 1u < COOKIE_GAME_COLUMNS) {
            ++g_player_x;
        }
        for (index = 0u; index < COOKIE_GAME_TARGET; ++index) {
            const uint32_t cookie_x = (index * 7u + 2u) % COOKIE_GAME_COLUMNS;
            const uint32_t cookie_y = (index * 5u + 1u) % COOKIE_GAME_ROWS;
            if (!g_cookie_collected[index] &&
                g_player_x == cookie_x && g_player_y == cookie_y) {
                g_cookie_collected[index] = 1u;
                ++g_cookies_collected_count;
                if (g_cookies_collected_count == COOKIE_GAME_TARGET) {
                    g_cookiesgame_won = 1u;
                }
            }
        }
        if (g_player_x == g_monster_x && g_player_y == g_monster_y) {
            g_cookiesgame_over = 1u;
        }
        return;
    }

    if (g_active_app == APP_TERMINAL) {
        if (scan_code == 0x01u && g_donut3d_running) {
            g_donut3d_running = 0u;
            terminal_append("DONUT3D STOPPED");
            return;
        }
        const char character = keyboard_char(scan_code);
        if (character == '\n') {
            terminal_execute();
        } else if (character == '\b') {
            if (g_terminal_input_length > 0u) {
                --g_terminal_input_length;
                g_terminal_input[g_terminal_input_length] = '\0';
            }
        } else if (character != '\0' && g_terminal_input_length + 1u < TERMINAL_LINE_LENGTH) {
            g_terminal_input[g_terminal_input_length++] = character;
            g_terminal_input[g_terminal_input_length] = '\0';
        }
    } else if (g_active_app == APP_NOTEPAD) {
        const char character = keyboard_char(scan_code);
        if (character == '\n') {
            if (g_note_line + 1u < NOTE_LINE_COUNT) {
                ++g_note_line;
                g_note_column = 0u;
            }
        } else if (character == '\b') {
            if (g_note_column > 0u) {
                --g_note_column;
                g_note_lines[g_note_line][g_note_column] = '\0';
            }
        } else if (character != '\0' && g_note_column + 1u < NOTE_LINE_LENGTH) {
            g_note_lines[g_note_line][g_note_column++] = character;
            g_note_lines[g_note_line][g_note_column] = '\0';
        }
    }
}

static void port_write8(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static void port_write16(uint16_t port, uint16_t value)
{
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

static __attribute__((noreturn)) void power_action(uint8_t reboot)
{
    __asm__ volatile ("cli");
    if (reboot) {
        port_write8(0x64u, 0xFEu);
        port_write8(0xCF9u, 0x06u);
    } else {
        port_write16(0x604u, 0x2000u);
        port_write16(0xB004u, 0x2000u);
    }
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

static void draw_button(uint32_t x, uint32_t y, uint32_t width,
                        const char *label, uint32_t app)
{
    const uint32_t selected = g_active_app == app ? COLOR_ACCENT : COLOR_PANEL_LIGHT;
    framebuffer_draw_rect(x, y, width, 34u, selected);
    framebuffer_draw_string(x + 8u, y + 13u, label, COLOR_TEXT);
}

static void draw_donut_animation(void);

static void draw_donut_animation(void)
{
    const uint32_t x = g_screen_width / 8u + 192u;
    const uint32_t y = g_screen_height / 8u + 112u;

    framebuffer_draw_rect(x, y, 48u * 8u, 20u * 10u, COLOR_PANEL);
    donut3d_draw(x, y, g_donut3d_frame++);
}

static void draw_cookiesgame(void)
{
    static const uint32_t board_width = COOKIE_GAME_COLUMNS * COOKIE_GAME_CELL_SIZE;
    static const uint32_t board_height = COOKIE_GAME_ROWS * COOKIE_GAME_CELL_SIZE;
    const uint32_t x = (g_screen_width - board_width) / 2u;
    const uint32_t y = g_screen_height / 8u + 132u;
    uint32_t index;
    uint32_t column;
    uint32_t row;

    framebuffer_draw_string(x, y - 54u, "COOKIES: MOVE WITH W A S D", COLOR_TEXT);
    framebuffer_draw_string(x, y - 30u, "COLLECT 10 COOKIES. AVOID THE MONSTER.",
                            COLOR_MUTED);
    framebuffer_draw_rect(x, y, board_width, board_height, COLOR_PANEL_LIGHT);
    for (column = 1u; column < COOKIE_GAME_COLUMNS; ++column) {
        framebuffer_draw_rect(x + column * COOKIE_GAME_CELL_SIZE, y, 1u,
                              board_height, COLOR_PANEL);
    }
    for (row = 1u; row < COOKIE_GAME_ROWS; ++row) {
        framebuffer_draw_rect(x, y + row * COOKIE_GAME_CELL_SIZE,
                              board_width, 1u, COLOR_PANEL);
    }

    for (index = 0u; index < COOKIE_GAME_TARGET; ++index) {
        const uint32_t cookie_x = (index * 7u + 2u) % COOKIE_GAME_COLUMNS;
        const uint32_t cookie_y = (index * 5u + 1u) % COOKIE_GAME_ROWS;
        if (!g_cookie_collected[index]) {
            framebuffer_draw_char(x + cookie_x * COOKIE_GAME_CELL_SIZE + 12u,
                                  y + cookie_y * COOKIE_GAME_CELL_SIZE + 10u,
                                  '*', 0x00FFD166u);
        }
    }
    framebuffer_draw_char(x + g_player_x * COOKIE_GAME_CELL_SIZE + 12u,
                          y + g_player_y * COOKIE_GAME_CELL_SIZE + 10u,
                          '@', COLOR_GREEN);
    framebuffer_draw_char(x + g_monster_x * COOKIE_GAME_CELL_SIZE + 12u,
                          y + g_monster_y * COOKIE_GAME_CELL_SIZE + 10u,
                          'M', COLOR_RED);

    if (g_cookiesgame_won) {
        framebuffer_draw_string(x, y + board_height + 16u,
                                "YOU WIN! PRESS R TO PLAY AGAIN", COLOR_GREEN);
    } else if (g_cookiesgame_over) {
        framebuffer_draw_string(x, y + board_height + 16u,
                                "COOKIE MONSTER CAUGHT YOU! PRESS R", COLOR_RED);
    } else {
        framebuffer_draw_string(x, y + board_height + 16u,
                                "ESC: EXIT", COLOR_MUTED);
    }
    framebuffer_draw_string(x, y + board_height + 38u, "GALLETAS:", COLOR_TEXT);
    framebuffer_draw_char(x + 104u, y + board_height + 38u,
                          (char)('0' + g_cookies_collected_count / 10u),
                          COLOR_ACCENT);
    framebuffer_draw_char(x + 112u, y + board_height + 38u,
                          (char)('0' + g_cookies_collected_count % 10u),
                          COLOR_ACCENT);
    framebuffer_draw_char(x + 128u, y + board_height + 38u, '/', COLOR_TEXT);
    framebuffer_draw_char(x + 144u, y + board_height + 38u, '1', COLOR_ACCENT);
    framebuffer_draw_char(x + 152u, y + board_height + 38u, '0', COLOR_ACCENT);
}

static void cookiesgame_update_monster(void)
{
    if (!g_cookiesgame_running || g_cookiesgame_over || g_cookiesgame_won) {
        return;
    }
    if (g_monster_x < g_player_x) {
        ++g_monster_x;
    } else if (g_monster_x > g_player_x) {
        --g_monster_x;
    } else if (g_monster_y < g_player_y) {
        ++g_monster_y;
    } else if (g_monster_y > g_player_y) {
        --g_monster_y;
    }
    if (g_player_x == g_monster_x && g_player_y == g_monster_y) {
        g_cookiesgame_over = 1u;
    }
}

static void draw_app_window(void)
{
    const uint32_t x = g_screen_width / 8u;
    const uint32_t y = g_screen_height / 8u;
    const uint32_t width = g_screen_width * 3u / 4u;
    const uint32_t height = g_screen_height * 5u / 8u;
    const uint32_t close_x = x + width - 32u;
    uint32_t row;
    const char *title = "MINUX";

    if (g_active_app == APP_TERMINAL) {
        title = "TERMINAL";
    } else if (g_active_app == APP_NOTEPAD) {
        title = "BLOC DE NOTAS";
    } else if (g_active_app == APP_COMPUTER) {
        title = "ESTE EQUIPO";
    } else if (g_active_app == APP_SETTINGS) {
        title = "AJUSTES";
    } else if (g_active_app == APP_BROWSER) {
        title = "NAVEGADOR";
    } else if (g_active_app == APP_CALCULATOR) {
        title = "CALCULADORA";
    } else if (g_active_app == APP_PAINT) {
        title = "PAINT";
    }

    framebuffer_draw_rect(x, y, width, height, COLOR_PANEL);
    framebuffer_draw_rect(x, y, width, 36u, COLOR_PANEL_LIGHT);
    framebuffer_draw_string(x + 14u, y + 14u, title, COLOR_TEXT);
    framebuffer_draw_rect(close_x, y + 5u, 24u, 24u, COLOR_RED);
    framebuffer_draw_char(close_x + 8u, y + 9u, 'X', COLOR_TEXT);

    if (g_active_app == APP_TERMINAL) {
        framebuffer_draw_string(x + 24u, y + 58u, "MINUX COMMAND LINE", COLOR_ACCENT);
        if (g_cookiesgame_running) {
            draw_cookiesgame();
        } else if (g_donut3d_running) {
            framebuffer_draw_string(x + 24u, y + 82u,
                                    "DONUT3D - ESC TO EXIT", COLOR_MUTED);
            draw_donut_animation();
        } else {
            for (row = 0u; row < g_terminal_line_count; ++row) {
                framebuffer_draw_string(x + 24u, y + 88u + row * 22u,
                                        g_terminal_lines[row], COLOR_TEXT);
            }
            framebuffer_draw_string(x + 24u, y + height - 42u, "MINUX> ", COLOR_GREEN);
            framebuffer_draw_string(x + 80u, y + height - 42u,
                                    g_terminal_input, COLOR_TEXT);
            framebuffer_draw_string(x + 24u, y + height - 22u,
                                    "DONUT3D COKIESGAME HELP WIFI SCAN", COLOR_MUTED);
        }
    } else if (g_active_app == APP_NOTEPAD) {
        framebuffer_draw_string(x + 24u, y + 58u, "UNSAVED NOTE", COLOR_ACCENT);
        for (row = 0u; row < NOTE_LINE_COUNT; ++row) {
            framebuffer_draw_string(x + 24u, y + 86u + row * 22u,
                                    g_note_lines[row], COLOR_TEXT);
        }
        framebuffer_draw_string(x + 24u, y + height - 28u,
                                "TEXT IS KEPT UNTIL REBOOT", COLOR_MUTED);
    } else if (g_active_app == APP_COMPUTER) {
        framebuffer_draw_string(x + 24u, y + 58u, "CARPETAS", g_accent_color);
        draw_computer_item(x + 32u, y + 76u, "DESCARGAS",
                           ICON_DESCARGAR, ICON_NIGUN);
        draw_computer_item(x + 270u, y + 76u, "VIDEOS",
                           ICON_VIDEOS, ICON_VIDEO);
        draw_computer_item(x + 32u, y + 116u, "IMAGENES",
                           ICON_IMAGENS, ICON_IMAGEN);
        draw_computer_item(x + 270u, y + 116u, "MUSICA",
                           ICON_MUSICA, ICON_MUSICALES);
        draw_computer_item(x + 32u, y + 156u, "DOCUMENTOS",
                           ICON_DOCUMENTOS, ICON_TEXTOS);
        framebuffer_draw_string(x + 24u, y + 190u,
                                "ICONOS DE MUESTRA - SIN ARCHIVOS REALES",
                                COLOR_MUTED);
        draw_icon(ICON_IMAGEN, x + 32u, y + 202u, 22u);
        framebuffer_draw_string(x + 24u, y + 229u, "IMAGEN", COLOR_TEXT);
        draw_icon(ICON_MUSICALES, x + 124u, y + 202u, 22u);
        framebuffer_draw_string(x + 112u, y + 229u, "MUSICA", COLOR_TEXT);
        draw_icon(ICON_TEXTOS, x + 216u, y + 202u, 22u);
        framebuffer_draw_string(x + 208u, y + 229u, "TEXTO", COLOR_TEXT);
        draw_icon(ICON_VIDEO, x + 308u, y + 202u, 22u);
        framebuffer_draw_string(x + 300u, y + 229u, "VIDEO", COLOR_TEXT);
        draw_icon(ICON_NIGUN, x + 400u, y + 202u, 22u);
        framebuffer_draw_string(x + 394u, y + 229u, "OTRO", COLOR_TEXT);
    } else if (g_active_app == APP_SETTINGS) {
        framebuffer_draw_string(x + 24u, y + 68u, "AJUSTES DEL SISTEMA", g_accent_color);
        framebuffer_draw_rect(x + 24u, y + 92u, 300u, 42u, COLOR_PANEL_LIGHT);
        framebuffer_draw_string(x + 38u, y + 110u,
                                g_accent_color == COLOR_ACCENT
                                    ? "COLOR: TURQUESA (CLIC)"
                                    : "COLOR: AZUL (CLIC)",
                                COLOR_TEXT);
        {
            const uint32_t option_width = (width - 64u) / 2u;
            const uint32_t first_x = x + 24u;
            const uint32_t second_x = first_x + option_width + 16u;
            const uint32_t option_y = y + (height > 340u ? 180u : 160u);
            const uint32_t option_height = height > 340u ? 86u : 68u;
            const uint32_t preview_width = option_width - 16u;
            const uint32_t preview_height = option_height - 38u;

            framebuffer_draw_string(x + 24u, option_y - 30u,
                                    "FONDO DEL ESCRITORIO (CLIC PARA ELEGIR)",
                                    COLOR_TEXT);
            framebuffer_draw_rect(first_x, option_y, option_width, option_height,
                                  g_wallpaper_selected == 0u ? COLOR_ACCENT : COLOR_PANEL_LIGHT);
            draw_wallpaper_preview((const wallpaper_image_t *)minux_wallpaper_start,
                                   first_x + 8u, option_y + 5u,
                                   preview_width, preview_height);
            framebuffer_draw_string(first_x + 8u, option_y + option_height - 18u,
                                    g_wallpaper_selected == 0u
                                        ? "MINUX ACTUAL *"
                                        : "MINUX ACTUAL",
                                    COLOR_TEXT);

            framebuffer_draw_rect(second_x, option_y, option_width, option_height,
                                  g_wallpaper_selected == 1u ? COLOR_ACCENT : COLOR_PANEL_LIGHT);
            draw_wallpaper_preview(
                (const wallpaper_image_t *)minux_wallpaper_arriune_start,
                second_x + 8u, option_y + 5u, preview_width, preview_height);
            framebuffer_draw_string(second_x + 8u, option_y + option_height - 18u,
                                    g_wallpaper_selected == 1u
                                        ? "ARRIUNE *"
                                        : "ARRIUNE",
                                    COLOR_TEXT);
            framebuffer_draw_string(x + 24u, option_y + option_height + 8u,
                                    "FONDO ACTIVO HASTA REINICIAR",
                                    COLOR_MUTED);
        }
    } else if (g_active_app == APP_BROWSER) {
        framebuffer_draw_string(x + 24u, y + 68u, "MINUX WEB", g_accent_color);
        framebuffer_draw_rect(x + 24u, y + 92u, width - 48u, 30u, COLOR_PANEL_LIGHT);
        framebuffer_draw_string(x + 34u, y + 103u, "HTTP://", COLOR_TEXT);
        framebuffer_draw_string(x + 24u, y + 148u, "NAVEGACION NO DISPONIBLE", COLOR_TEXT);
        framebuffer_draw_string(x + 24u, y + 178u, "FALTA RED, DNS, TLS Y MOTOR WEB", COLOR_MUTED);
    } else if (g_active_app == APP_CALCULATOR) {
        static const char *const keys[4][4] = {
            {"7", "8", "9", "/"},
            {"4", "5", "6", "*"},
            {"1", "2", "3", "-"},
            {"C", "0", "=", "+"}
        };
        char display[16];
        uint32_t key_row;
        uint32_t key_column;

        if (g_calculator_error) {
            copy_text(display, "ERROR", sizeof(display));
        } else {
            format_integer(g_calculator_entry, display, sizeof(display));
        }
        framebuffer_draw_rect(x + 24u, y + 56u, width - 48u, 44u, COLOR_PANEL_LIGHT);
        framebuffer_draw_string(x + 38u, y + 74u, display, COLOR_TEXT);
        for (key_row = 0u; key_row < 4u; ++key_row) {
            for (key_column = 0u; key_column < 4u; ++key_column) {
                const uint32_t key_x = x + 120u + key_column * 76u;
                const uint32_t key_y = y + 124u + key_row * 62u;
                framebuffer_draw_rect(key_x, key_y, 64u, 48u,
                                      key_column == 3u ? g_accent_color : COLOR_PANEL_LIGHT);
                framebuffer_draw_string(key_x + 26u, key_y + 20u,
                                        keys[key_row][key_column], COLOR_TEXT);
            }
        }
        framebuffer_draw_string(x + 24u, y + height - 28u,
                                "TECLADO: NUMEROS, ENTER, C, BACKSPACE", COLOR_MUTED);
    } else if (g_active_app == APP_PAINT) {
        const uint32_t canvas_x = x + 24u;
        const uint32_t canvas_y = y + 62u;
        const uint32_t canvas_width = width - 48u;
        const uint32_t canvas_height = height - 150u;
        static const uint32_t colors[] = {
            FRAMEBUFFER_COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_BLUE,
            0x00FFD166u
        };
        uint32_t stroke;
        uint32_t palette_index;

        framebuffer_draw_rect(canvas_x, canvas_y, canvas_width, canvas_height,
                              FRAMEBUFFER_COLOR_WHITE);
        for (stroke = 0u; stroke < g_paint_stroke_count; ++stroke) {
            framebuffer_draw_rect(g_paint_strokes[stroke].x,
                                  g_paint_strokes[stroke].y, 8u, 8u,
                                  g_paint_strokes[stroke].color);
        }
        framebuffer_draw_string(canvas_x, canvas_y + canvas_height + 18u,
                                "COLOR:", COLOR_TEXT);
        for (palette_index = 0u; palette_index < 5u; ++palette_index) {
            const uint32_t swatch_x = canvas_x + 72u + palette_index * 42u;
            framebuffer_draw_rect(swatch_x, canvas_y + canvas_height + 12u,
                                  28u, 28u, colors[palette_index]);
        }
        framebuffer_draw_rect(canvas_x + canvas_width - 92u,
                              canvas_y + canvas_height + 12u,
                              88u, 28u, COLOR_RED);
        framebuffer_draw_string(canvas_x + canvas_width - 78u,
                                canvas_y + canvas_height + 22u,
                                "BORRAR", COLOR_TEXT);
        framebuffer_draw_string(canvas_x, y + height - 20u,
                                "DIBUJO TEMPORAL: SE PIERDE AL REINICIAR", COLOR_MUTED);
    }
}

static void draw_pointer(uint32_t x, uint32_t y)
{
    const cursor_image_t *const image =
        (const cursor_image_t *)minux_cursor_start;
    const uint32_t cursor_width = 28u;
    const uint32_t cursor_height = 30u;
    const uint32_t origin_x = x > 4u ? x - 4u : 0u;
    const uint32_t origin_y = y > 4u ? y - 4u : 0u;
    uint32_t cursor_y;

    for (cursor_y = 0u; cursor_y < cursor_height; ++cursor_y) {
        const uint32_t source_y =
            (uint32_t)(((uint64_t)cursor_y * image->height) / cursor_height);
        uint32_t cursor_x;

        for (cursor_x = 0u; cursor_x < cursor_width; ++cursor_x) {
            const uint32_t screen_x = origin_x + cursor_x;
            const uint32_t screen_y = origin_y + cursor_y;
            const uint32_t source_x =
                (uint32_t)(((uint64_t)cursor_x * image->width) / cursor_width);
            const uint64_t pixel_index =
                ((uint64_t)source_y * image->width + source_x) * 4u;
            const uint8_t *const pixel = image->pixels + pixel_index;

            if (screen_x < g_screen_width && screen_y < g_screen_height &&
                pixel[3] != 0u) {
                const uint32_t color = ((uint32_t)pixel[0] << 16) |
                                       ((uint32_t)pixel[1] << 8) |
                                       (uint32_t)pixel[2];
                framebuffer_put_pixel(screen_x, screen_y, color);
            }
        }
    }
}

static void draw_icon(icon_id_t icon, uint32_t x, uint32_t y, uint32_t size)
{
    const cursor_image_t *const image = g_icons[icon];
    uint32_t icon_y;

    for (icon_y = 0u; icon_y < size; ++icon_y) {
        const uint32_t source_y =
            (uint32_t)(((uint64_t)icon_y * image->height) / size);
        uint32_t icon_x;

        for (icon_x = 0u; icon_x < size; ++icon_x) {
            const uint32_t screen_x = x + icon_x;
            const uint32_t screen_y = y + icon_y;
            const uint32_t source_x =
                (uint32_t)(((uint64_t)icon_x * image->width) / size);
            const uint64_t pixel_index =
                ((uint64_t)source_y * image->width + source_x) * 4u;
            const uint8_t *const pixel = image->pixels + pixel_index;

            if (screen_x < g_screen_width && screen_y < g_screen_height &&
                pixel[3] != 0u) {
                const uint32_t color = ((uint32_t)pixel[0] << 16) |
                                       ((uint32_t)pixel[1] << 8) |
                                       (uint32_t)pixel[2];
                framebuffer_put_pixel(screen_x, screen_y, color);
            }
        }
    }
}

static void draw_app_icon(app_icon_id_t icon, uint32_t x, uint32_t y,
                          uint32_t size)
{
    const cursor_image_t *const image = g_app_icons[icon];
    uint32_t icon_y;

    for (icon_y = 0u; icon_y < size; ++icon_y) {
        const uint32_t source_y =
            (uint32_t)(((uint64_t)icon_y * image->height) / size);
        uint32_t icon_x;

        for (icon_x = 0u; icon_x < size; ++icon_x) {
            const uint32_t screen_x = x + icon_x;
            const uint32_t screen_y = y + icon_y;
            const uint32_t source_x =
                (uint32_t)(((uint64_t)icon_x * image->width) / size);
            const uint64_t pixel_index =
                ((uint64_t)source_y * image->width + source_x) * 4u;
            const uint8_t *const pixel = image->pixels + pixel_index;

            if (screen_x < g_screen_width && screen_y < g_screen_height &&
                pixel[3] != 0u) {
                const uint32_t color = ((uint32_t)pixel[0] << 16) |
                                       ((uint32_t)pixel[1] << 8) |
                                       (uint32_t)pixel[2];
                framebuffer_put_pixel(screen_x, screen_y, color);
            }
        }
    }
}

static void draw_computer_item(uint32_t x, uint32_t y, const char *label,
                               icon_id_t folder_icon, icon_id_t file_icon)
{
    draw_icon(folder_icon, x, y, 34u);
    framebuffer_draw_string(x + 42u, y + 13u, label, COLOR_TEXT);
    draw_icon(file_icon, x + 176u, y + 4u, 26u);
}

static void draw_wallpaper_preview(const wallpaper_image_t *image, uint32_t x,
                                   uint32_t y, uint32_t width,
                                   uint32_t height)
{
    uint32_t crop_width = image->width;
    uint32_t crop_height = image->height;
    uint32_t crop_x = 0u;
    uint32_t crop_y = 0u;
    uint32_t preview_y;

    if ((uint64_t)image->width * height >
        (uint64_t)image->height * width) {
        crop_width = (uint32_t)(((uint64_t)image->height * width) / height);
        crop_x = (image->width - crop_width) / 2u;
    } else {
        crop_height = (uint32_t)(((uint64_t)image->width * height) / width);
        crop_y = (image->height - crop_height) / 2u;
    }

    for (preview_y = 0u; preview_y < height; ++preview_y) {
        const uint32_t source_y =
            crop_y + (uint32_t)(((uint64_t)preview_y * crop_height) / height);
        uint32_t preview_x;

        for (preview_x = 0u; preview_x < width; ++preview_x) {
            const uint32_t source_x =
                crop_x + (uint32_t)(((uint64_t)preview_x * crop_width) / width);
            const uint64_t pixel_index =
                ((uint64_t)source_y * image->width + source_x) * 3u;
            const uint8_t *const pixel = image->pixels + pixel_index;
            const uint32_t color = ((uint32_t)pixel[0] << 16) |
                                   ((uint32_t)pixel[1] << 8) |
                                   (uint32_t)pixel[2];

            framebuffer_put_pixel(x + preview_x, y + preview_y, color);
        }
    }
}

static void draw_wallpaper(void)
{
    const wallpaper_image_t *const image = g_wallpaper_selected == 0u
        ? (const wallpaper_image_t *)minux_wallpaper_start
        : (const wallpaper_image_t *)minux_wallpaper_arriune_start;

    draw_wallpaper_preview(image, 0u, 0u, g_screen_width, g_screen_height);
}

static void paint_add_point(uint32_t x, uint32_t y)
{
    const uint32_t window_x = g_screen_width / 8u;
    const uint32_t window_y = g_screen_height / 8u;
    const uint32_t window_width = g_screen_width * 3u / 4u;
    const uint32_t window_height = g_screen_height * 5u / 8u;
    const uint32_t canvas_x = window_x + 24u;
    const uint32_t canvas_y = window_y + 62u;
    const uint32_t canvas_width = window_width - 48u;
    const uint32_t canvas_height = window_height - 150u;

    if (x < canvas_x || x >= canvas_x + canvas_width ||
        y < canvas_y || y >= canvas_y + canvas_height ||
        g_paint_stroke_count >= PAINT_STROKE_CAPACITY) {
        return;
    }
    if (g_paint_stroke_count != 0u) {
        const uint32_t last = g_paint_stroke_count - 1u;
        const int32_t dx = (int32_t)x - g_paint_strokes[last].x;
        const int32_t dy = (int32_t)y - g_paint_strokes[last].y;
        if (dx > -5 && dx < 5 && dy > -5 && dy < 5) {
            return;
        }
    }
    g_paint_strokes[g_paint_stroke_count].x = (uint16_t)x;
    g_paint_strokes[g_paint_stroke_count].y = (uint16_t)y;
    g_paint_strokes[g_paint_stroke_count].color = g_paint_color;
    ++g_paint_stroke_count;
}

static void draw_desktop(uint32_t pointer_x, uint32_t pointer_y, int input_ready)
{
    const uint32_t taskbar_y = g_screen_height - 48u;

    draw_wallpaper();
    framebuffer_draw_rect(0u, 0u, g_screen_width, 52u, COLOR_PANEL);
    framebuffer_draw_rect(0u, 50u, g_screen_width, 2u, COLOR_ACCENT);
    framebuffer_draw_string(20u, 20u, "MINUX OS", COLOR_TEXT);
    framebuffer_draw_string(160u, 20u, input_ready ? "INPUT PS/2" : "INPUT N/A", COLOR_MUTED);
    framebuffer_draw_string(g_screen_width - 160u, 20u, "WI-FI N/A", COLOR_MUTED);

    framebuffer_draw_rect(24u, 92u, 144u, 66u, COLOR_PANEL);
    draw_app_icon(APP_ICON_TERMINAL, 30u, 100u, 40u);
    framebuffer_draw_string(74u, 114u, "TERMINAL", COLOR_TEXT);
    framebuffer_draw_rect(190u, 92u, 156u, 66u, COLOR_PANEL);
    draw_icon(ICON_DOCUMENTOS, 198u, 100u, 40u);
    framebuffer_draw_string(244u, 114u, "NOTAS", COLOR_TEXT);
    framebuffer_draw_rect(368u, 92u, 168u, 66u, COLOR_PANEL);
    draw_icon(ICON_CARPETA, 376u, 100u, 46u);
    framebuffer_draw_string(428u, 110u, "ESTE EQUIPO", COLOR_TEXT);
    framebuffer_draw_rect(24u, 174u, 144u, 66u, COLOR_PANEL);
    draw_app_icon(APP_ICON_SETTINGS, 30u, 182u, 40u);
    framebuffer_draw_string(74u, 196u, "AJUSTES", COLOR_TEXT);
    framebuffer_draw_rect(190u, 174u, 156u, 66u, COLOR_PANEL);
    draw_app_icon(APP_ICON_BROWSER, 198u, 182u, 40u);
    framebuffer_draw_string(244u, 196u, "NAVEGADOR", COLOR_TEXT);
    framebuffer_draw_rect(24u, 256u, 144u, 66u, COLOR_PANEL);
    draw_app_icon(APP_ICON_CALCULATOR, 30u, 264u, 40u);
    framebuffer_draw_string(74u, 278u, "CALCULADORA", COLOR_TEXT);
    framebuffer_draw_rect(190u, 256u, 156u, 66u, COLOR_PANEL);
    draw_app_icon(APP_ICON_PAINT, 198u, 264u, 40u);
    framebuffer_draw_string(244u, 278u, "PAINT", COLOR_TEXT);

    framebuffer_draw_rect(0u, taskbar_y, g_screen_width, 48u, COLOR_PANEL);
    framebuffer_draw_rect(0u, taskbar_y, g_screen_width, 2u, COLOR_ACCENT);
    draw_button(8u, taskbar_y + 7u, 64u, "MENU", APP_NONE);
    draw_button(78u, taskbar_y + 7u, 88u, "TERMINAL", APP_TERMINAL);
    draw_button(172u, taskbar_y + 7u, 76u, "NOTAS", APP_NOTEPAD);
    draw_button(254u, taskbar_y + 7u, 84u, "EQUIPO", APP_COMPUTER);
    draw_button(344u, taskbar_y + 7u, 82u, "AJUSTES", APP_SETTINGS);
    draw_button(432u, taskbar_y + 7u, 96u, "NAVEGADOR", APP_BROWSER);
    framebuffer_draw_rect(536u, taskbar_y + 7u, 82u, 34u,
                          g_power_menu_open ? COLOR_RED : COLOR_PANEL_LIGHT);
    framebuffer_draw_string(544u, taskbar_y + 20u, "ENERGIA", COLOR_TEXT);
    framebuffer_draw_rect(g_screen_width - 144u, taskbar_y + 7u, 136u, 34u,
                          g_wifi_menu_open ? COLOR_ACCENT : COLOR_PANEL_LIGHT);
    framebuffer_draw_string(g_screen_width - 134u, taskbar_y + 20u,
                            "WI-FI N/A", COLOR_TEXT);

    if (g_wifi_menu_open) {
        const uint32_t menu_x = g_screen_width - 270u;
        const uint32_t menu_y = taskbar_y - 112u;
        framebuffer_draw_rect(menu_x, menu_y, 258u, 100u, COLOR_PANEL);
        framebuffer_draw_string(menu_x + 12u, menu_y + 16u,
                                "ADAPTADOR NO DISPONIBLE", COLOR_RED);
        framebuffer_draw_rect(menu_x + 8u, menu_y + 32u, 242u, 28u,
                              COLOR_PANEL_LIGHT);
        framebuffer_draw_string(menu_x + 16u, menu_y + 42u,
                                "BUSCAR REDES", COLOR_TEXT);
        framebuffer_draw_string(menu_x + 12u, menu_y + 78u,
                                g_wifi_scan_requested
                                    ? "NO HAY DRIVER WI-FI"
                                    : "NO CONECTADO",
                                COLOR_MUTED);
    }

    if (g_launcher_open) {
        const uint32_t menu_y = taskbar_y - 292u;
        framebuffer_draw_rect(8u, menu_y, 244u, 270u, COLOR_PANEL);
        framebuffer_draw_string(24u, menu_y + 18u, "APLICACIONES", COLOR_ACCENT);
        draw_app_icon(APP_ICON_TERMINAL, 18u, menu_y + 38u, 22u);
        framebuffer_draw_string(50u, menu_y + 48u, "TERMINAL", COLOR_TEXT);
        draw_icon(ICON_DOCUMENTOS, 18u, menu_y + 68u, 22u);
        framebuffer_draw_string(50u, menu_y + 78u, "BLOC DE NOTAS", COLOR_TEXT);
        draw_icon(ICON_CARPETA, 18u, menu_y + 98u, 22u);
        framebuffer_draw_string(50u, menu_y + 108u, "ESTE EQUIPO", COLOR_TEXT);
        draw_app_icon(APP_ICON_SETTINGS, 18u, menu_y + 128u, 22u);
        framebuffer_draw_string(50u, menu_y + 138u, "AJUSTES", COLOR_TEXT);
        draw_app_icon(APP_ICON_BROWSER, 18u, menu_y + 158u, 22u);
        framebuffer_draw_string(50u, menu_y + 168u, "NAVEGADOR", COLOR_TEXT);
        draw_app_icon(APP_ICON_CALCULATOR, 18u, menu_y + 188u, 22u);
        framebuffer_draw_string(50u, menu_y + 198u, "CALCULADORA", COLOR_TEXT);
        draw_app_icon(APP_ICON_PAINT, 18u, menu_y + 218u, 22u);
        framebuffer_draw_string(50u, menu_y + 228u, "PAINT", COLOR_TEXT);
    }
    if (g_power_menu_open) {
        const uint32_t menu_x = g_screen_width - 200u;
        const uint32_t menu_y = taskbar_y - 104u;
        framebuffer_draw_rect(menu_x, menu_y, 188u, 92u, COLOR_PANEL);
        framebuffer_draw_string(menu_x + 14u, menu_y + 24u, "APAGAR", COLOR_TEXT);
        framebuffer_draw_string(menu_x + 14u, menu_y + 62u, "REINICIAR", COLOR_TEXT);
    }

    if (g_active_app != APP_NONE) {
        draw_app_window();
    }
    draw_pointer(pointer_x, pointer_y);
}

static void handle_click(uint32_t x, uint32_t y)
{
    const uint32_t taskbar_y = g_screen_height - 48u;

    if (g_wifi_menu_open) {
        const uint32_t menu_x = g_screen_width - 270u;
        const uint32_t menu_y = taskbar_y - 112u;
        if (x >= menu_x + 8u && x < menu_x + 250u &&
            y >= menu_y + 32u && y < menu_y + 60u) {
            g_wifi_scan_requested = 1u;
            return;
        }
        if (x >= g_screen_width - 144u && x < g_screen_width - 8u &&
            y >= taskbar_y + 7u && y < taskbar_y + 41u) {
            g_wifi_menu_open = 0u;
            return;
        }
        g_wifi_menu_open = 0u;
    }

    if (g_power_menu_open) {
        const uint32_t menu_x = g_screen_width - 200u;
        const uint32_t menu_y = taskbar_y - 104u;
        if (x >= menu_x && x < menu_x + 188u && y >= menu_y && y < menu_y + 92u) {
            if (y < menu_y + 46u) {
                power_action(0u);
            }
            power_action(1u);
        }
        g_power_menu_open = 0u;
        return;
    }

    if (g_launcher_open && y >= taskbar_y - 292u && y < taskbar_y - 12u && x < 252u) {
        if (y >= taskbar_y - 250u && y < taskbar_y - 220u) {
            g_active_app = APP_TERMINAL;
        } else if (y >= taskbar_y - 220u && y < taskbar_y - 190u) {
            g_active_app = APP_NOTEPAD;
        } else if (y >= taskbar_y - 190u && y < taskbar_y - 160u) {
            g_active_app = APP_COMPUTER;
        } else if (y >= taskbar_y - 160u && y < taskbar_y - 130u) {
            g_active_app = APP_SETTINGS;
        } else if (y >= taskbar_y - 130u && y < taskbar_y - 100u) {
            g_active_app = APP_BROWSER;
        } else if (y >= taskbar_y - 100u && y < taskbar_y - 70u) {
            g_active_app = APP_CALCULATOR;
        } else if (y >= taskbar_y - 70u && y < taskbar_y - 40u) {
            g_active_app = APP_PAINT;
        }
        g_launcher_open = 0u;
        return;
    }

    if (y >= taskbar_y) {
        if (x < 72u) {
            g_launcher_open = (uint8_t)!g_launcher_open;
        } else if (x < 166u) {
            g_active_app = APP_TERMINAL;
        } else if (x < 248u) {
            g_active_app = APP_NOTEPAD;
        } else if (x < 338u) {
            g_active_app = APP_COMPUTER;
        } else if (x < 426u) {
            g_active_app = APP_SETTINGS;
        } else if (x < 528u) {
            g_active_app = APP_BROWSER;
        } else if (x >= 536u && x < 618u) {
            g_power_menu_open = (uint8_t)!g_power_menu_open;
            g_wifi_menu_open = 0u;
        } else if (x >= g_screen_width - 144u &&
                   x < g_screen_width - 8u) {
            g_wifi_menu_open = (uint8_t)!g_wifi_menu_open;
            g_wifi_scan_requested = 0u;
            g_power_menu_open = 0u;
        }
        return;
    }

    if (g_active_app != APP_NONE) {
        const uint32_t close_x = g_screen_width * 7u / 8u - 32u;
        const uint32_t window_x = g_screen_width / 8u;
        const uint32_t window_y = g_screen_height / 8u;
        const uint32_t window_width = g_screen_width * 3u / 4u;
        const uint32_t window_height = g_screen_height * 5u / 8u;
        if (x >= close_x && y >= window_y &&
            y < g_screen_height / 8u + 36u) {
            g_active_app = APP_NONE;
        } else if (x >= window_x && x < window_x + window_width &&
                   y >= window_y && y < window_y + window_height) {
            const uint32_t local_x = x - window_x;
            const uint32_t local_y = y - window_y;
            if (g_active_app == APP_CALCULATOR &&
                local_x >= 120u && local_x < 424u &&
                local_y >= 124u && local_y < 372u) {
                const uint32_t column = (local_x - 120u) / 76u;
                const uint32_t row = (local_y - 124u) / 62u;
                if (column < 4u && row < 4u &&
                    (local_x - 120u) % 76u < 64u &&
                    (local_y - 124u) % 62u < 48u) {
                    static const char keys[4][4] = {
                        {'7', '8', '9', '/'},
                        {'4', '5', '6', '*'},
                        {'1', '2', '3', '-'},
                        {'C', '0', '=', '+'}
                    };
                    const char key = keys[row][column];
                    if (key >= '0' && key <= '9') {
                        calculator_digit((uint8_t)(key - '0'));
                    } else if (key == 'C') {
                        calculator_clear();
                    } else if (key == '=') {
                        calculator_equals();
                    } else {
                        calculator_apply_operator((uint8_t)key);
                    }
                }
            } else if (g_active_app == APP_SETTINGS &&
                       local_x >= 24u && local_x < 324u &&
                       local_y >= 92u && local_y < 134u) {
                g_accent_color = g_accent_color == COLOR_ACCENT
                                     ? COLOR_BLUE : COLOR_ACCENT;
            } else if (g_active_app == APP_SETTINGS) {
                const uint32_t option_width = (window_width - 64u) / 2u;
                const uint32_t option_y = window_height > 340u ? 180u : 160u;
                const uint32_t option_height =
                    window_height > 340u ? 86u : 68u;

                if (local_y >= option_y &&
                    local_y < option_y + option_height) {
                    if (local_x >= 24u &&
                        local_x < 24u + option_width) {
                        g_wallpaper_selected = 0u;
                    } else if (local_x >= 40u + option_width &&
                               local_x < window_width - 24u) {
                        g_wallpaper_selected = 1u;
                    }
                }
            } else if (g_active_app == APP_PAINT) {
                const uint32_t canvas_x = window_x + 24u;
                const uint32_t canvas_y = window_y + 62u;
                const uint32_t canvas_width = window_width - 48u;
                const uint32_t canvas_height = window_height - 150u;
                static const uint32_t palette[5] = {
                    FRAMEBUFFER_COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_BLUE,
                    0x00FFD166u
                };
                uint32_t palette_index;
                for (palette_index = 0u; palette_index < 5u; ++palette_index) {
                    const uint32_t swatch_x = canvas_x + 72u + palette_index * 42u;
                    if (x >= swatch_x && x < swatch_x + 28u &&
                        y >= canvas_y + canvas_height + 12u &&
                        y < canvas_y + canvas_height + 40u) {
                        g_paint_color = palette[palette_index];
                        break;
                    }
                }
                if (x >= canvas_x + canvas_width - 92u &&
                    x < canvas_x + canvas_width - 4u &&
                    y >= canvas_y + canvas_height + 12u &&
                    y < canvas_y + canvas_height + 40u) {
                    g_paint_stroke_count = 0u;
                }
            }
        }
    } else if (x >= 24u && x < 168u) {
        if (y >= 92u && y < 158u) {
            g_active_app = APP_TERMINAL;
        } else if (y >= 174u && y < 240u) {
            g_active_app = APP_SETTINGS;
        } else if (y >= 256u && y < 322u) {
            calculator_clear();
            g_active_app = APP_CALCULATOR;
        }
    } else if (x >= 190u && x < 346u) {
        if (y >= 92u && y < 158u) {
            g_active_app = APP_NOTEPAD;
        } else if (y >= 174u && y < 240u) {
            g_active_app = APP_BROWSER;
        } else if (y >= 256u && y < 322u) {
            g_active_app = APP_PAINT;
        }
    } else if (x >= 368u && x < 536u && y >= 92u && y < 158u) {
        g_active_app = APP_COMPUTER;
    }
}

__attribute__((noreturn))
void kernel_main(uint32_t multiboot_info, uint32_t multiboot_magic)
{
    if (multiboot_magic != MULTIBOOT2_BOOT_MAGIC) {
        console_write("MINUX OS: error de arranque Multiboot2.\n");
        for (;;) {
            __asm__ volatile ("cli; hlt");
        }
    }
    framebuffer_init_from_multiboot(multiboot_info);
    g_screen_width = framebuffer_get_width();
    g_screen_height = framebuffer_get_height();
    if (g_screen_width < 512u || g_screen_height < 400u) {
        console_write("MINUX OS: framebuffer insuficiente.\n");
        for (;;) {
            __asm__ volatile ("cli; hlt");
        }
    }

    const int input_ready = ps2_init();
    const boot_mode_t boot_mode = parse_boot_mode(multiboot_info);
    uint32_t pointer_x = g_screen_width / 2u;
    uint32_t pointer_y = g_screen_height / 2u;
    uint32_t animation_ticks = 0u;
    uint32_t monster_ticks = 0u;
    uint8_t previous_buttons = 0u;
    if (boot_mode == BOOT_MODE_DONUT3D) {
        g_active_app = APP_TERMINAL;
        g_donut3d_running = 1u;
        terminal_append("GRUB: DONUT3D - ESC TO EXIT");
    } else if (boot_mode == BOOT_MODE_COKIESGAME) {
        g_active_app = APP_TERMINAL;
        cookiesgame_start();
        terminal_append("GRUB: COKIESGAME");
    }
    draw_desktop(pointer_x, pointer_y, input_ready);
    for (;;) {
        int16_t delta_x;
        int16_t delta_y;
        uint8_t buttons;
        uint8_t scan_code;
        const uint8_t events = ps2_poll_events(&delta_x, &delta_y, &buttons, &scan_code);

        if ((events & PS2_EVENT_KEYBOARD) != 0u) {
            handle_key(scan_code);
            draw_desktop(pointer_x, pointer_y, input_ready);
        }
        if ((events & PS2_EVENT_MOUSE) != 0u) {
            const uint8_t left_pressed = (buttons & 0x01u) != 0u;
            const uint8_t was_left_pressed = (previous_buttons & 0x01u) != 0u;
            int32_t next_x = (int32_t)pointer_x + delta_x;
            int32_t next_y = (int32_t)pointer_y + delta_y;

            if (next_x < 0) {
                next_x = 0;
            } else if ((uint32_t)next_x >= g_screen_width) {
                next_x = (int32_t)g_screen_width - 1;
            }
            if (next_y < 0) {
                next_y = 0;
            } else if ((uint32_t)next_y >= g_screen_height) {
                next_y = (int32_t)g_screen_height - 1;
            }

            pointer_x = (uint32_t)next_x;
            pointer_y = (uint32_t)next_y;
            if (g_active_app == APP_PAINT && (buttons & 0x01u) != 0u) {
                paint_add_point(pointer_x, pointer_y);
            }
            if (left_pressed && !was_left_pressed) {
                handle_click(pointer_x, pointer_y);
            }
            previous_buttons = buttons;
            draw_desktop(pointer_x, pointer_y, input_ready);
        }
        if ((g_donut3d_running || g_cookiesgame_running) &&
            g_active_app == APP_TERMINAL && ++animation_ticks >= 1000000u) {
            animation_ticks = 0u;
            if (g_donut3d_running) {
                draw_donut_animation();
            }
            if (g_cookiesgame_running && ++monster_ticks >= 3u) {
                monster_ticks = 0u;
                cookiesgame_update_monster();
                draw_desktop(pointer_x, pointer_y, input_ready);
            }
        }
        __asm__ volatile ("pause");
    }
}
