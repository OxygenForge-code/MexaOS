/* ============================================================
 *  MexaOS VGA Driver
 *  Hardware text mode with MexaOS indigo theme
 * ============================================================ */

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "../../include/mexaos.h"
#include "include/vga.h"
#include "include/memory.h"

#define STR(x)  #x
#define XSTR(x) STR(x)

static struct vga_state vga = {
    .buffer = (uint16_t *)VGA_BUFFER,
    .fg_color = VGA_COLOR_WHITE,
    .bg_color = VGA_COLOR_BLACK,
    .accent_color = VGA_COLOR_LBLUE,
    .row = 0,
    .col = 0,
    .width = VGA_COLS,
    .height = VGA_ROWS,
    .cursor_visible = 1,
    .echo_enabled = 1
};

/* ─── Early Init (no interrupts, no heap) ─── */
void vga_early_init(void) {
    vga.buffer = (uint16_t *)VGA_BUFFER;
    vga.fg_color = VGA_COLOR_WHITE;
    vga.bg_color = VGA_COLOR_BLACK;
    vga.accent_color = VGA_COLOR_LBLUE;
    vga.row = 0;
    vga.col = 0;
    vga.width = VGA_COLS;
    vga.height = VGA_ROWS;
    vga.cursor_visible = 1;
    vga.echo_enabled = 1;
    
    inb(VGA_CTRL_PORT);
    outb(VGA_CTRL_PORT, 0x0A);
    uint8_t cur_start = inb(VGA_DATA_PORT);
    outb(VGA_DATA_PORT, cur_start & ~0x20);
    
    vga_splash_screen();
}

/* ─── Full Init ─── */
void vga_init(void) {
    vga_clear();
    vga_cursor_show();
}

/* ─── Clear Screen ─── */
void vga_clear(void) {
    uint8_t color = vga_entry_color(vga.fg_color, vga.bg_color);
    for (uint32_t i = 0; i < vga.width * vga.height; i++) {
        vga.buffer[i] = vga_entry(' ', color);
    }
    vga.row = 0;
    vga.col = 0;
    vga_update_cursor();
}

/* ─── Set Colors ─── */
void vga_setcolor(uint8_t fg, uint8_t bg) {
    vga.fg_color = fg;
    vga.bg_color = bg;
}

void vga_set_accent(uint8_t color) {
    vga.accent_color = color;
}

/* ─── Position ─── */
void vga_gotoxy(uint32_t x, uint32_t y) {
    if (x < vga.width) vga.col = x;
    if (y < vga.height) vga.row = y;
    vga_update_cursor();
}

void vga_getxy(uint32_t *x, uint32_t *y) {
    *x = vga.col;
    *y = vga.row;
}

/* ─── Cursor ─── */
void vga_cursor_show(void) {
    vga.cursor_visible = 1;
    outb(VGA_CTRL_PORT, 0x0A);
    outb(VGA_DATA_PORT, (inb(VGA_DATA_PORT) & 0xC0) | 14);
    outb(VGA_CTRL_PORT, 0x0B);
    outb(VGA_DATA_PORT, (inb(VGA_DATA_PORT) & 0xE0) | 15);
    vga_update_cursor();
}

void vga_cursor_hide(void) {
    vga.cursor_visible = 0;
    outb(VGA_CTRL_PORT, 0x0A);
    outb(VGA_DATA_PORT, 0x20);
}

void vga_cursor_set(uint32_t row, uint32_t col) {
    vga.row = row;
    vga.col = col;
    vga_update_cursor();
}

void vga_update_cursor(void) {
    if (!vga.cursor_visible) return;
    uint16_t pos = vga.row * vga.width + vga.col;
    outb(VGA_CTRL_PORT, 0x0F);
    outb(VGA_DATA_PORT, (uint8_t)(pos & 0xFF));
    outb(VGA_CTRL_PORT, 0x0E);
    outb(VGA_DATA_PORT, (uint8_t)((pos >> 8) & 0xFF));
}

/* ─── Echo Control ─── */
void vga_set_echo(uint8_t enabled) {
    vga.echo_enabled = enabled;
}

/* ─── Scroll ─── */
void vga_scroll(void) {
    uint8_t color = vga_entry_color(vga.fg_color, vga.bg_color);
    
    for (uint32_t y = 0; y < vga.height - 1; y++) {
        for (uint32_t x = 0; x < vga.width; x++) {
            vga.buffer[y * vga.width + x] = vga.buffer[(y + 1) * vga.width + x];
        }
    }
    
    for (uint32_t x = 0; x < vga.width; x++) {
        vga.buffer[(vga.height - 1) * vga.width + x] = vga_entry(' ', color);
    }
    
    vga.row = vga.height - 1;
}

/* ─── Put Character ─── */
void vga_putc(char c) {
    if (!vga.echo_enabled && c != '\n' && c != '\r' && c != '\t' && c != '\b') {
        return;
    }
    
    uint8_t color = vga_entry_color(vga.fg_color, vga.bg_color);
    
    switch (c) {
        case '\n':
            vga.col = 0;
            vga.row++;
            break;
            
        case '\r':
            vga.col = 0;
            break;
            
        case '\t':
            vga.col = (vga.col + 8) & ~7;
            break;
            
        case '\b':
            if (vga.col > 0) {
                vga.col--;
                vga.buffer[vga.row * vga.width + vga.col] = vga_entry(' ', color);
            }
            break;
            
        default:
            if (c >= ' ') {
                vga.buffer[vga.row * vga.width + vga.col] = vga_entry(c, color);
                vga.col++;
            }
            break;
    }
    
    if (vga.col >= vga.width) {
        vga.col = 0;
        vga.row++;
    }
    
    if (vga.row >= vga.height) {
        vga_scroll();
    }
    
    vga_update_cursor();
}

/* ─── Put String ─── */
void vga_puts(const char *s) {
    while (*s) {
        vga_putc(*s++);
    }
}

/* ─── Drawing Primitives ─── */
void vga_draw_box(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t color) {
    if (x + w > vga.width || y + h > vga.height) return;
    
    uint8_t box_color = vga_entry_color(color, vga.bg_color);
    
    vga.buffer[y * vga.width + x] = vga_entry('+', box_color);
    vga.buffer[y * vga.width + x + w - 1] = vga_entry('+', box_color);
    vga.buffer[(y + h - 1) * vga.width + x] = vga_entry('+', box_color);
    vga.buffer[(y + h - 1) * vga.width + x + w - 1] = vga_entry('+', box_color);
    
    for (uint32_t i = 1; i < w - 1; i++) {
        vga.buffer[y * vga.width + x + i] = vga_entry('-', box_color);
        vga.buffer[(y + h - 1) * vga.width + x + i] = vga_entry('-', box_color);
    }
    
    for (uint32_t i = 1; i < h - 1; i++) {
        vga.buffer[(y + i) * vga.width + x] = vga_entry('|', box_color);
        vga.buffer[(y + i) * vga.width + x + w - 1] = vga_entry('|', box_color);
    }
}

void vga_draw_line_h(uint32_t x, uint32_t y, uint32_t len, uint8_t color) {
    if (y >= vga.height) return;
    uint8_t lc = vga_entry_color(color, vga.bg_color);
    for (uint32_t i = 0; i < len && x + i < vga.width; i++) {
        vga.buffer[y * vga.width + x + i] = vga_entry('-', lc);
    }
}

void vga_draw_line_v(uint32_t x, uint32_t y, uint32_t len, uint8_t color) {
    if (x >= vga.width) return;
    uint8_t lc = vga_entry_color(color, vga.bg_color);
    for (uint32_t i = 0; i < len && y + i < vga.height; i++) {
        vga.buffer[(y + i) * vga.width + x] = vga_entry('|', lc);
    }
}

void vga_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t color) {
    if (x + w > vga.width || y + h > vga.height) return;
    uint8_t fc = vga_entry_color(vga.bg_color, color);
    for (uint32_t j = 0; j < h; j++) {
        for (uint32_t i = 0; i < w; i++) {
            vga.buffer[(y + j) * vga.width + x + i] = vga_entry(' ', fc);
        }
    }
}

/* ─── Number Printing ─── */
void vga_print_dec(uint64_t n) {
    char buf[32];
    int i = 30;
    buf[31] = '\0';
    
    if (n == 0) {
        vga_putc('0');
        return;
    }
    
    while (n > 0 && i > 0) {
        buf[i--] = '0' + (n % 10);
        n /= 10;
    }
    
    vga_puts(&buf[i + 1]);
}

void vga_print_hex(uint64_t n) {
    char buf[32];
    const char *hex = "0123456789ABCDEF";
    int i = 30;
    buf[31] = '\0';
    
    if (n == 0) {
        vga_puts("0x0");
        return;
    }
    
    while (n > 0 && i > 0) {
        buf[i--] = hex[n & 0xF];
        n >>= 4;
    }
    
    buf[i--] = 'x';
    buf[i] = '0';
    vga_puts(&buf[i]);
}

void vga_print_size(uint64_t bytes) {
    if (bytes < 1024) {
        vga_print_dec(bytes);
        vga_puts(" B");
    } else if (bytes < 1024 * 1024) {
        vga_print_dec(bytes / 1024);
        vga_puts(" KB");
    } else if (bytes < 1024 * 1024 * 1024) {
        vga_print_dec(bytes / (1024 * 1024));
        vga_puts(" MB");
    } else {
        vga_print_dec(bytes / (1024 * 1024 * 1024));
        vga_puts(" GB");
    }
}

/* ─── Splash Screen ─── */
void vga_splash_screen(void) {
    vga_clear();
    
    uint8_t accent = vga_entry_color(VGA_COLOR_LBLUE, VGA_COLOR_BLACK);
    
    for (int x = 5; x < 75; x++) {
        vga.buffer[3 * vga.width + x] = vga_entry('=', accent);
        vga.buffer[21 * vga.width + x] = vga_entry('=', accent);
    }
    
    for (int y = 4; y < 21; y++) {
        vga.buffer[y * vga.width + 5] = vga_entry('|', accent);
        vga.buffer[y * vga.width + 74] = vga_entry('|', accent);
    }
    
    vga.buffer[3 * vga.width + 5] = vga_entry('+', accent);
    vga.buffer[3 * vga.width + 74] = vga_entry('+', accent);
    vga.buffer[21 * vga.width + 5] = vga_entry('+', accent);
    vga.buffer[21 * vga.width + 74] = vga_entry('+', accent);
    
    const char *logo[] = {
        " __  ___                      ____   _____ ",
        " /  |/  /____ _   _____  _____/ __ \\/ ___/ ",
        "/ /|_/ // __ \\ | / / _ \\/ ___/ / / /\\__ \\  ",
        "/ /  / // /_/ / |/ /  __/ /  / /_/ /___/ /  ",
        "/_/  /_/ \\____/|___/\\___/_/   \\____//____/   "
    };
    
    for (int i = 0; i < 5; i++) {
        int len = strlen(logo[i]);
        int start_x = (vga.width - len) / 2;
        for (int j = 0; j < len && logo[i][j]; j++) {
            uint8_t c = vga_entry_color(VGA_COLOR_LBLUE, VGA_COLOR_BLACK);
            vga.buffer[(6 + i) * vga.width + start_x + j] = vga_entry(logo[i][j], c);
        }
    }
    
    const char *tagline = "Intent-Driven Operating System";
    int tag_x = (vga.width - strlen(tagline)) / 2;
    for (int i = 0; tagline[i]; i++) {
        uint8_t c = vga_entry_color(VGA_COLOR_LMAGENTA, VGA_COLOR_BLACK);
        vga.buffer[13 * vga.width + tag_x + i] = vga_entry(tagline[i], c);
    }
    
    const char *ver = "v" MEXAOS_VERSION " (Build " XSTR(MEXAOS_BUILD) ")";
    int ver_x = (vga.width - strlen(ver)) / 2;
    for (int i = 0; ver[i]; i++) {
        uint8_t c = vga_entry_color(VGA_COLOR_DGRAY, VGA_COLOR_BLACK);
        vga.buffer[15 * vga.width + ver_x + i] = vga_entry(ver[i], c);
    }
    
    const char *boot_msg = "Booting MexaOS...";
    int boot_x = (vga.width - strlen(boot_msg)) / 2;
    for (int i = 0; boot_msg[i]; i++) {
        uint8_t c = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga.buffer[18 * vga.width + boot_x + i] = vga_entry(boot_msg[i], c);
    }
    
    int pb_x = (vga.width - 40) / 2;
    for (int i = 0; i < 40; i++) {
        uint8_t c = vga_entry_color(VGA_COLOR_LBLUE, VGA_COLOR_BLACK);
        vga.buffer[19 * vga.width + pb_x + i] = vga_entry('.', c);
    }
    
    vga.row = 23;
    vga.col = 0;
}

/* ─── Logo Draw ─── */
void vga_draw_logo(void) {
    vga_puts("MexaOS");
}
