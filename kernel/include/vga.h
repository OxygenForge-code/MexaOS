/* ============================================================
 *  MexaOS VGA Driver Header
 *  Text mode display with MexaOS theme
 * ============================================================ */

#ifndef VGA_H
#define VGA_H

#include "../../include/mexaos.h"

/* ─── VGA State ─── */
struct vga_state {
    uint16_t *buffer;
    uint8_t  fg_color;
    uint8_t  bg_color;
    uint8_t  accent_color;
    uint32_t row;
    uint32_t col;
    uint32_t width;
    uint32_t height;
    uint8_t  cursor_visible;
    uint8_t  echo_enabled;
};

/* ─── Functions ─── */
void vga_early_init(void);
void vga_init(void);
void vga_clear(void);
void vga_putc(char c);
void vga_puts(const char *s);
void vga_setcolor(uint8_t fg, uint8_t bg);
void vga_set_accent(uint8_t color);
void vga_gotoxy(uint32_t x, uint32_t y);
void vga_getxy(uint32_t *x, uint32_t *y);
void vga_scroll(void);
void vga_cursor_show(void);
void vga_cursor_hide(void);
void vga_cursor_set(uint32_t row, uint32_t col);
void vga_update_cursor(void);
void vga_set_echo(uint8_t enabled);
void vga_draw_box(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t color);
void vga_draw_line_h(uint32_t x, uint32_t y, uint32_t len, uint8_t color);
void vga_draw_line_v(uint32_t x, uint32_t y, uint32_t len, uint8_t color);
void vga_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t color);
void vga_print_dec(uint64_t n);
void vga_print_hex(uint64_t n);
void vga_print_size(uint64_t bytes);
void vga_draw_logo(void);
void vga_splash_screen(void);

/* ─── Color helpers ─── */
static inline uint8_t vga_entry_color(uint8_t fg, uint8_t bg) {
    return fg | (bg << 4);
}

static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

/* ─── VGA port I/O ─── */
#define VGA_CTRL_PORT   0x3D4
#define VGA_DATA_PORT   0x3D5

#endif /* VGA_H */
