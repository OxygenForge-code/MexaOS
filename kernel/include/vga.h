/* ============================================================
 *  MexaOS VGA Driver
 * ============================================================ */

#ifndef MEXAOS_VGA_H
#define MEXAOS_VGA_H

#include <stdint.h>
#include <stddef.h>

/* ─── VGA Hardware ─── */
#define VGA_BUFFER      0xB8000
#define VGA_CTRL_PORT   0x3D4
#define VGA_DATA_PORT   0x3D5
#define VGA_COLS        80
#define VGA_ROWS        25

/* ─── VGA Colors ─── */
#define VGA_COLOR_BLACK         0
#define VGA_COLOR_BLUE          1
#define VGA_COLOR_GREEN         2
#define VGA_COLOR_CYAN          3
#define VGA_COLOR_RED           4
#define VGA_COLOR_MAGENTA       5
#define VGA_COLOR_BROWN         6
#define VGA_COLOR_LIGHT_GRAY    7
#define VGA_COLOR_DARK_GRAY     8
#define VGA_COLOR_LIGHT_BLUE    9
#define VGA_COLOR_LIGHT_GREEN   10
#define VGA_COLOR_LIGHT_CYAN    11
#define VGA_COLOR_LIGHT_RED     12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_YELLOW        14
#define VGA_COLOR_WHITE         15

/* Aliases */
#define VGA_COLOR_DIM           VGA_COLOR_DARK_GRAY
#define VGA_COLOR_ORANGE        VGA_COLOR_BROWN
#define VGA_COLOR_LBLUE         VGA_COLOR_LIGHT_BLUE
#define VGA_COLOR_LGREEN        VGA_COLOR_LIGHT_GREEN
#define VGA_COLOR_LCYAN         VGA_COLOR_LIGHT_CYAN
#define VGA_COLOR_LRED          VGA_COLOR_LIGHT_RED
#define VGA_COLOR_LMAGENTA      VGA_COLOR_LIGHT_MAGENTA
#define VGA_COLOR_DGRAY         VGA_COLOR_DARK_GRAY

/* ─── VGA State ─── */
struct vga_state {
    uint16_t *buffer;
    uint8_t fg_color;
    uint8_t bg_color;
    uint8_t accent_color;
    uint32_t row;
    uint32_t col;
    uint32_t width;
    uint32_t height;
    uint8_t cursor_visible;
    uint8_t echo_enabled;
};

/* ─── Helper Macros ─── */
#define vga_entry_color(fg, bg) ((fg) | ((bg) << 4))
#define vga_entry(uc, color)    ((uint16_t)(uc) | ((uint16_t)(color) << 8))

/* ─── Basic Functions ─── */
void vga_init(void);
void vga_clear(void);
void vga_putchar(char c);
void vga_puts(const char *s);
void vga_setcolor(uint8_t fg, uint8_t bg);

/* ─── Cursor ─── */
void vga_early_init(void);
void vga_cursor_show(void);
void vga_cursor_hide(void);
void vga_cursor_set(uint32_t row, uint32_t col);
void vga_update_cursor(void);

/* ─── Position & Echo ─── */
void vga_set_echo(uint8_t enabled);
void vga_gotoxy(uint32_t x, uint32_t y);
void vga_getxy(uint32_t *x, uint32_t *y);

/* ─── Scrolling ─── */
void vga_scroll(void);

/* ─── Drawing ─── */
void vga_draw_box(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t color);
void vga_draw_line_h(uint32_t x, uint32_t y, uint32_t len, uint8_t color);
void vga_draw_line_v(uint32_t x, uint32_t y, uint32_t len, uint8_t color);
void vga_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t color);

/* ─── Accent & Splash ─── */
void vga_set_accent(uint8_t color);
void vga_splash_screen(void);

/* ─── Number Printing ─── */
void vga_print_dec(uint64_t n);
void vga_print_hex(uint64_t n);
void vga_print_size(uint64_t bytes);

/* ─── Logo ─── */
void vga_draw_logo(void);

#endif
