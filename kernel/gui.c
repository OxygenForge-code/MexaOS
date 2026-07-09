/* ============================================================
 *  MexaOS GUI Subsystem
 *  Framebuffer graphics with MexaOS indigo theme
 * ============================================================ */

#include "../include/mexaos.h"
#include "include/kernlib.h"
#include "include/vga.h"
#include "include/memory.h"
#include "include/keyboard.h"
#include "include/timer.h"
#include "include/gui.h"
#include "include/window.h"

/* ─── Forward Declarations ─── */
static void gui_draw_char(int32_t x, int32_t y, char c, uint32_t color);
static void gui_draw_mouse_cursor(int32_t x, int32_t y);
static void gui_taskbar_update_clock(void);
static void gui_render_notifications(void);

/* ─── 8x8 Bitmap Font (ASCII 32-127) ─── */
static const uint8_t font_8x8[96][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* space */
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, /* ! */
    {0x36,0x36,0x24,0x00,0x00,0x00,0x00,0x00}, /* " */
    {0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00}, /* # */
    {0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0x00}, /* $ */
    {0x00,0x63,0x33,0x18,0x0C,0x66,0x63,0x00}, /* % */
    {0x1C,0x36,0x1C,0x6E,0x3B,0x33,0x6E,0x00}, /* & */
    {0x06,0x06,0x03,0x00,0x00,0x00,0x00,0x00}, /* ' */
    {0x18,0x0C,0x06,0x06,0x06,0x0C,0x18,0x00}, /* ( */
    {0x06,0x0C,0x18,0x18,0x18,0x0C,0x06,0x00}, /* ) */
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, /* * */
    {0x00,0x0C,0x0C,0x3F,0x0C,0x0C,0x00,0x00}, /* + */
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x06}, /* , */
    {0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00}, /* - */
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x00}, /* . */
    {0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0x00}, /* / */
    {0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0x00}, /* 0 */
    {0x0C,0x0E,0x0C,0x0C,0x0C,0x0C,0x3F,0x00}, /* 1 */
    {0x1E,0x33,0x30,0x1C,0x06,0x33,0x3F,0x00}, /* 2 */
    {0x1E,0x33,0x30,0x1C,0x30,0x33,0x1E,0x00}, /* 3 */
    {0x38,0x3C,0x36,0x33,0x7F,0x30,0x78,0x00}, /* 4 */
    {0x3F,0x03,0x1F,0x30,0x30,0x33,0x1E,0x00}, /* 5 */
    {0x1C,0x06,0x03,0x1F,0x33,0x33,0x1E,0x00}, /* 6 */
    {0x3F,0x33,0x30,0x18,0x0C,0x0C,0x0C,0x00}, /* 7 */
    {0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E,0x00}, /* 8 */
    {0x1E,0x33,0x33,0x3E,0x30,0x18,0x0E,0x00}, /* 9 */
    {0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x00}, /* : */
    {0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x06}, /* ; */
    {0x18,0x0C,0x06,0x03,0x06,0x0C,0x18,0x00}, /* < */
    {0x00,0x00,0x3F,0x00,0x00,0x3F,0x00,0x00}, /* = */
    {0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00}, /* > */
    {0x1E,0x33,0x30,0x18,0x0C,0x00,0x0C,0x00}, /* ? */
    {0x1E,0x33,0x37,0x37,0x37,0x03,0x1E,0x00}, /* @ */
    {0x0C,0x1E,0x33,0x33,0x3F,0x33,0x33,0x00}, /* A */
    {0x3F,0x66,0x66,0x3E,0x66,0x66,0x3F,0x00}, /* B */
    {0x3C,0x66,0x03,0x03,0x03,0x66,0x3C,0x00}, /* C */
    {0x1F,0x36,0x66,0x66,0x66,0x36,0x1F,0x00}, /* D */
    {0x7F,0x46,0x16,0x1E,0x16,0x46,0x7F,0x00}, /* E */
    {0x7F,0x46,0x16,0x1E,0x16,0x06,0x0F,0x00}, /* F */
    {0x3C,0x66,0x03,0x03,0x73,0x66,0x7C,0x00}, /* G */
    {0x33,0x33,0x33,0x3F,0x33,0x33,0x33,0x00}, /* H */
    {0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00}, /* I */
    {0x78,0x30,0x30,0x30,0x33,0x33,0x1E,0x00}, /* J */
    {0x67,0x66,0x36,0x1E,0x36,0x66,0x67,0x00}, /* K */
    {0x0F,0x06,0x06,0x06,0x46,0x66,0x7F,0x00}, /* L */
    {0x63,0x77,0x7F,0x7F,0x6B,0x63,0x63,0x00}, /* M */
    {0x63,0x67,0x6F,0x7B,0x73,0x63,0x63,0x00}, /* N */
    {0x1C,0x36,0x63,0x63,0x63,0x36,0x1C,0x00}, /* O */
    {0x3F,0x66,0x66,0x3E,0x06,0x06,0x0F,0x00}, /* P */
    {0x1E,0x33,0x33,0x33,0x3B,0x1E,0x38,0x00}, /* Q */
    {0x3F,0x66,0x66,0x3E,0x36,0x66,0x67,0x00}, /* R */
    {0x1E,0x33,0x07,0x0E,0x38,0x33,0x1E,0x00}, /* S */
    {0x3F,0x2D,0x0C,0x0C,0x0C,0x0C,0x1E,0x00}, /* T */
    {0x33,0x33,0x33,0x33,0x33,0x33,0x3F,0x00}, /* U */
    {0x33,0x33,0x33,0x33,0x33,0x1E,0x0C,0x00}, /* V */
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00}, /* W */
    {0x63,0x63,0x36,0x1C,0x1C,0x36,0x63,0x00}, /* X */
    {0x33,0x33,0x33,0x1E,0x0C,0x0C,0x1E,0x00}, /* Y */
    {0x7F,0x63,0x31,0x18,0x4C,0x66,0x7F,0x00}, /* Z */
    {0x1E,0x06,0x06,0x06,0x06,0x06,0x1E,0x00}, /* [ */
    {0x01,0x03,0x06,0x0C,0x18,0x30,0x60,0x00}, /* \ */
    {0x1E,0x18,0x18,0x18,0x18,0x18,0x1E,0x00}, /* ] */
    {0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00}, /* ^ */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF}, /* _ */
    {0x0C,0x0C,0x18,0x00,0x00,0x00,0x00,0x00}, /* ` */
    {0x00,0x00,0x1E,0x30,0x3E,0x33,0x6E,0x00}, /* a */
    {0x07,0x06,0x06,0x3E,0x66,0x66,0x3B,0x00}, /* b */
    {0x00,0x00,0x1E,0x33,0x03,0x33,0x1E,0x00}, /* c */
    {0x38,0x30,0x30,0x3E,0x33,0x33,0x6E,0x00}, /* d */
    {0x00,0x00,0x1E,0x33,0x3F,0x03,0x1E,0x00}, /* e */
    {0x1C,0x36,0x06,0x0F,0x06,0x06,0x0F,0x00}, /* f */
    {0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x1F}, /* g */
    {0x07,0x06,0x36,0x6E,0x66,0x66,0x67,0x00}, /* h */
    {0x0C,0x00,0x0E,0x0C,0x0C,0x0C,0x1E,0x00}, /* i */
    {0x30,0x00,0x30,0x30,0x30,0x33,0x33,0x1E}, /* j */
    {0x07,0x06,0x66,0x36,0x1E,0x36,0x67,0x00}, /* k */
    {0x0E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00}, /* l */
    {0x00,0x00,0x33,0x7F,0x7F,0x6B,0x63,0x00}, /* m */
    {0x00,0x00,0x3F,0x66,0x66,0x66,0x66,0x00}, /* n */
    {0x00,0x00,0x1E,0x33,0x33,0x33,0x1E,0x00}, /* o */
    {0x00,0x00,0x3B,0x66,0x66,0x3E,0x06,0x0F}, /* p */
    {0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x78}, /* q */
    {0x00,0x00,0x3B,0x6E,0x66,0x06,0x0F,0x00}, /* r */
    {0x00,0x00,0x3E,0x03,0x1E,0x30,0x1F,0x00}, /* s */
    {0x08,0x0C,0x3E,0x0C,0x0C,0x2C,0x18,0x00}, /* t */
    {0x00,0x00,0x33,0x33,0x33,0x33,0x6E,0x00}, /* u */
    {0x00,0x00,0x33,0x33,0x33,0x1E,0x0C,0x00}, /* v */
    {0x00,0x00,0x63,0x6B,0x7F,0x77,0x63,0x00}, /* w */
    {0x00,0x00,0x63,0x36,0x1C,0x36,0x63,0x00}, /* x */
    {0x00,0x00,0x33,0x33,0x33,0x3E,0x30,0x1F}, /* y */
    {0x00,0x00,0x3F,0x19,0x0C,0x26,0x3F,0x00}, /* z */
    {0x38,0x0C,0x0C,0x07,0x0C,0x0C,0x38,0x00}, /* { */
    {0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00}, /* | */
    {0x07,0x0C,0x0C,0x38,0x0C,0x0C,0x07,0x00}, /* } */
    {0x6E,0x3B,0x00,0x00,0x00,0x00,0x00,0x00}, /* ~ */
};

/* ─── Mouse cursor bitmap (12x16) ─── */
static const uint16_t mouse_cursor[16] = {
    0x000, 0x001, 0x003, 0x007, 0x00F, 0x01F, 0x03F, 0x07F,
    0x0FF, 0x01F, 0x01D, 0x00B, 0x00E, 0x004, 0x004, 0x000
};

/* ─── Notification queue ─── */
#define MAX_NOTIFICATIONS 8
struct gui_notification {
    char title[64];
    char message[128];
    uint64_t expires_at;
    uint8_t active;
};
static struct gui_notification notifications[MAX_NOTIFICATIONS];

/* ─── Global GUI context ─── */
static struct gui_context gui;

/* ============================================================
 *  Initialization
 * ============================================================ */

void gui_init(uint32_t fb_addr, uint32_t width, uint32_t height, 
              uint32_t pitch, uint8_t bpp) {
    gui.framebuffer = (uint32_t *)(uint64_t)fb_addr;
    gui.width = width;
    gui.height = height;
    gui.pitch = pitch;
    gui.bpp = bpp;
    gui.mouse_x = width / 2;
    gui.mouse_y = height / 2;
    gui.mouse_visible = 1;
    gui.font_size = 8;
    gui.font_color = MEXA_FG;
    
    /* Set default MexaOS theme */
    gui.bg_color = MEXA_BG;
    gui.fg_color = MEXA_FG;
    gui.accent_color = MEXA_ACCENT;
    gui.glass_bg = MEXA_GLASS_BG;
    gui.glass_border = MEXA_GLASS_BORDER;
    
    /* Clear to background */
    gui_fill_rect(0, 0, width, height, gui.bg_color);
    
    /* Initialize notifications */
    memset(notifications, 0, sizeof(notifications));
}

/* ============================================================
 *  Primitive Drawing
 * ============================================================ */

void gui_pixel(int32_t x, int32_t y, uint32_t color) {
    if (x < 0 || y < 0 || (uint32_t)x >= gui.width || (uint32_t)y >= gui.height)
        return;
    gui.framebuffer[y * (gui.pitch / 4) + x] = color;
}

uint32_t gui_get_pixel(int32_t x, int32_t y) {
    if (x < 0 || y < 0 || (uint32_t)x >= gui.width || (uint32_t)y >= gui.height)
        return 0;
    return gui.framebuffer[y * (gui.pitch / 4) + x];
}

void gui_clear(uint32_t color) {
    gui_fill_rect(0, 0, gui.width, gui.height, color);
}

void gui_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    gui_line(x, y, x + w - 1, y, color);
    gui_line(x, y + h - 1, x + w - 1, y + h - 1, color);
    gui_line(x, y, x, y + h - 1, color);
    gui_line(x + w - 1, y, x + w - 1, y + h - 1, color);
}

void gui_fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > (int32_t)gui.width) w = gui.width - x;
    if (y + h > (int32_t)gui.height) h = gui.height - y;
    if (w <= 0 || h <= 0) return;
    
    uint32_t *fb = gui.framebuffer + y * (gui.pitch / 4) + x;
    for (int32_t row = 0; row < h; row++) {
        for (int32_t col = 0; col < w; col++) {
            fb[col] = color;
        }
        fb += gui.pitch / 4;
    }
}

void gui_rounded_rect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color) {
    int32_t max_r = w < h ? w / 2 : h / 2;
    if (r > max_r) r = max_r;
    
    /* Top and bottom edges */
    gui_line(x + r, y, x + w - r, y, color);
    gui_line(x + r, y + h - 1, x + w - r, y + h - 1, color);
    /* Left and right edges */
    gui_line(x, y + r, x, y + h - r, color);
    gui_line(x + w - 1, y + r, x + w - 1, y + h - r, color);
    
    /* Corners */
    gui_arc(x + r, y + r, r, 180, 270, color);
    gui_arc(x + w - r - 1, y + r, r, 270, 360, color);
    gui_arc(x + w - r - 1, y + h - r - 1, r, 0, 90, color);
    gui_arc(x + r, y + h - r - 1, r, 90, 180, color);
}

void gui_fill_rounded_rect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color) {
    int32_t max_r = w < h ? w / 2 : h / 2;
    if (r > max_r) r = max_r;
    
    /* Fill center rectangle */
    gui_fill_rect(x + r, y, w - 2 * r, h, color);
    gui_fill_rect(x, y + r, w, h - 2 * r, color);
    
    /* Fill corner circles */
    gui_fill_circle(x + r, y + r, r, color);
    gui_fill_circle(x + w - r - 1, y + r, r, color);
    gui_fill_circle(x + w - r - 1, y + h - r - 1, r, color);
    gui_fill_circle(x + r, y + h - r - 1, r, color);
}

void gui_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) {
    int32_t dx = x2 - x1;
    int32_t dy = y2 - y1;
    int32_t steps = ABS(dx) > ABS(dy) ? ABS(dx) : ABS(dy);
    
    if (steps == 0) {
        gui_pixel(x1, y1, color);
        return;
    }
    
    float x_inc = (float)dx / steps;
    float y_inc = (float)dy / steps;
    float x = x1;
    float y = y1;
    
    for (int32_t i = 0; i <= steps; i++) {
        gui_pixel((int32_t)(x + 0.5f), (int32_t)(y + 0.5f), color);
        x += x_inc;
        y += y_inc;
    }
}

void gui_circle(int32_t x, int32_t y, int32_t r, uint32_t color) {
    int32_t xx = r;
    int32_t yy = 0;
    int32_t d = 1 - r;
    
    while (xx >= yy) {
        gui_pixel(x + xx, y + yy, color);
        gui_pixel(x + yy, y + xx, color);
        gui_pixel(x - yy, y + xx, color);
        gui_pixel(x - xx, y + yy, color);
        gui_pixel(x - xx, y - yy, color);
        gui_pixel(x - yy, y - xx, color);
        gui_pixel(x + yy, y - xx, color);
        gui_pixel(x + xx, y - yy, color);
        
        yy++;
        if (d <= 0) {
            d += 2 * yy + 1;
        } else {
            xx--;
            d += 2 * (yy - xx) + 1;
        }
    }
}

void gui_fill_circle(int32_t x, int32_t y, int32_t r, uint32_t color) {
    for (int32_t dy = -r; dy <= r; dy++) {
        int32_t dx = (int32_t)sqrtf_approx(r * r - dy * dy);
        gui_line(x - dx, y + dy, x + dx, y + dy, color);
    }
}

void gui_triangle(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3, uint32_t color) {
    gui_line(x1, y1, x2, y2, color);
    gui_line(x2, y2, x3, y3, color);
    gui_line(x3, y3, x1, y1, color);
}

/* Draw arc (angles in degrees) */
static void gui_arc(int32_t x, int32_t y, int32_t r, int32_t start_angle, int32_t end_angle, uint32_t color) {
    for (int32_t a = start_angle; a <= end_angle; a++) {
        float rad = (float)a * 3.14159265f / 180.0f;
        int32_t px = x + (int32_t)(cosf_approx(rad) * r + 0.5f);
        int32_t py = y + (int32_t)(sinf_approx(rad) * r + 0.5f);
        gui_pixel(px, py, color);
    }
}

/* ============================================================
 *  Text Rendering
 * ============================================================ */

void gui_char(int32_t x, int32_t y, char c, uint32_t color) {
    if (c < 32 || c > 127) return;
    
    const uint8_t *bitmap = font_8x8[c - 32];
    for (int row = 0; row < 8; row++) {
        uint8_t line = bitmap[row];
        for (int col = 0; col < 8; col++) {
            if (line & (1 << col)) {
                gui_pixel(x + col, y + row, color);
            }
        }
    }
}

void gui_string(int32_t x, int32_t y, const char *s, uint32_t color) {
    int32_t cx = x;
    while (*s) {
        if (*s == '\n') {
            cx = x;
            y += 10;
        } else {
            gui_char(cx, y, *s, color);
            cx += 8;
        }
        s++;
    }
}

void gui_string_centered(int32_t x, int32_t y, int32_t w, const char *s, uint32_t color) {
    int32_t len = 0;
    const char *p = s;
    while (*p++) len++;
    int32_t tw = len * 8;
    gui_string(x + (w - tw) / 2, y, s, color);
}

static void gui_draw_char(int32_t x, int32_t y, char c, uint32_t color) {
    gui_char(x, y, c, color);
}

/* ============================================================
 *  Effects
 * ============================================================ */

void gui_blur(int32_t x, int32_t y, int32_t w, int32_t h, int radius) {
    if (radius <= 0) return;
    /* Simple box blur approximation */
    for (int32_t row = y; row < y + h; row++) {
        for (int32_t col = x; col < x + w; col++) {
            uint32_t r = 0, g = 0, b = 0;
            int count = 0;
            for (int dy = -radius; dy <= radius; dy++) {
                for (int dx = -radius; dx <= radius; dx++) {
                    uint32_t c = gui_get_pixel(col + dx, row + dy);
                    if (c != 0) {
                        r += (c >> 16) & 0xFF;
                        g += (c >> 8) & 0xFF;
                        b += c & 0xFF;
                        count++;
                    }
                }
            }
            if (count > 0) {
                uint32_t blurred = (0xFF << 24) | ((r / count) << 16) | ((g / count) << 8) | (b / count);
                gui_pixel(col, row, blurred);
            }
        }
    }
}

void gui_gradient_v(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t top, uint32_t bottom) {
    for (int32_t row = 0; row < h; row++) {
        uint8_t ratio = (uint8_t)((row * 255) / (h - 1));
        uint32_t color = gui_blend(top, bottom, ratio);
        gui_line(x, y + row, x + w - 1, y + row, color);
    }
}

void gui_gradient_h(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t left, uint32_t right) {
    for (int32_t col = 0; col < w; col++) {
        uint8_t ratio = (uint8_t)((col * 255) / (w - 1));
        uint32_t color = gui_blend(left, right, ratio);
        gui_line(x + col, y, x + col, y + h - 1, color);
    }
}

void gui_glow(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color, int intensity) {
    for (int i = intensity; i > 0; i--) {
        uint8_t alpha = (uint8_t)(0x40 / (i + 1));
        uint32_t glow_color = (alpha << 24) | (color & 0xFFFFFF);
        gui_rect(x - i, y - i, w + 2 * i, h + 2 * i, glow_color);
    }
}

void gui_shadow(int32_t x, int32_t y, int32_t w, int32_t h, int blur_radius) {
    uint32_t shadow_color = 0x40000000;
    for (int i = blur_radius; i > 0; i--) {
        uint8_t alpha = (uint8_t)(0x30 / (i));
        uint32_t c = (alpha << 24);
        gui_fill_rect(x + w + i / 2, y + i, blur_radius, h, c);
        gui_fill_rect(x + i, y + h + i / 2, w + blur_radius, blur_radius, c);
    }
}

/* ============================================================
 *  Color Utilities
 * ============================================================ */

uint32_t gui_blend(uint32_t bg, uint32_t fg, uint8_t alpha) {
    if (alpha == 0) return bg;
    if (alpha == 255) return fg;
    
    uint8_t bg_r = (bg >> 16) & 0xFF;
    uint8_t bg_g = (bg >> 8) & 0xFF;
    uint8_t bg_b = bg & 0xFF;
    uint8_t fg_r = (fg >> 16) & 0xFF;
    uint8_t fg_g = (fg >> 8) & 0xFF;
    uint8_t fg_b = fg & 0xFF;
    
    uint8_t r = (uint8_t)(((alpha * fg_r + (255 - alpha) * bg_r) >> 8) & 0xFF);
    uint8_t g = (uint8_t)(((alpha * fg_g + (255 - alpha) * bg_g) >> 8) & 0xFF);
    uint8_t b = (uint8_t)(((alpha * fg_b + (255 - alpha) * bg_b) >> 8) & 0xFF);
    
    return (0xFF << 24) | (r << 16) | (g << 8) | b;
}

uint32_t gui_color(uint8_t r, uint8_t g, uint8_t b) {
    return (0xFF << 24) | (r << 16) | (g << 8) | b;
}

/* ============================================================
 *  Desktop
 * ============================================================ */

void gui_desktop_start(void) {
    /* Draw desktop background */
    gui_draw_desktop();
    gui_draw_taskbar();
    
    /* Draw welcome window */
    struct window *welcome = window_create("Welcome to MexaOS", 
        (int32_t)gui.width / 2 - 200, (int32_t)gui.height / 2 - 150,
        400, 300, WIN_FLAG_ROUNDED | WIN_FLAG_SHADOW | WIN_FLAG_GLASS);
    if (welcome) {
        welcome->on_draw = gui_welcome_window_draw;
        window_show(welcome);
    }
    
    /* Main event loop */
    while (1) {
        gui_mouse_draw();
        gui_taskbar_update_clock();
        gui_render_notifications();
        
        /* Poll keyboard */
        char c = keyboard_getchar_nonblock();
        if (c == 0x01) { /* ESC to return to shell */
            break;
        }
        
        window_draw_all();
        timer_sleep(16); /* ~60 FPS */
    }
    
    /* Switch back to text mode */
    vga_clear();
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

void gui_draw_desktop(void) {
    /* Dark gradient background */
    gui_gradient_v(0, 0, gui.width, gui.height, MEXA_BG, 0xFF0A0A1A);
    
    /* Subtle accent line at top */
    gui_fill_rect(0, 0, gui.width, 1, MEXA_ACCENT);
    
    /* Desktop icons area - simple decorative elements */
    gui_string(24, 48, "Home", MEXA_FG);
    gui_string(24, 96, "Files", MEXA_FG);
    gui_string(24, 144, "Terminal", MEXA_FG);
    gui_string(24, 192, "AI Hub", MEXA_FG);
    
    /* Small accent dots */
    gui_fill_circle(14, 44, 3, MEXA_ACCENT);
    gui_fill_circle(14, 92, 3, MEXA_ACCENT_LT);
    gui_fill_circle(14, 140, 3, MEXA_CYAN);
    gui_fill_circle(14, 188, 3, MEXA_PINK);
}

void gui_draw_taskbar(void) {
    int32_t tb_h = 40;
    int32_t tb_y = (int32_t)gui.height - tb_h;
    
    /* Glass effect taskbar */
    gui_fill_rect(0, tb_y, gui.width, tb_h, 0xE6151525);
    gui_rect(0, tb_y, gui.width, tb_h, MEXA_GLASS_BORDER);
    
    /* Top accent line */
    gui_fill_rect(0, tb_y, gui.width, 1, MEXA_ACCENT);
    
    /* Start button */
    gui_fill_rounded_rect(8, tb_y + 8, 80, 24, 6, MEXA_ACCENT);
    gui_string_centered(8, tb_y + 12, 80, "Mexa", COLOR_WHITE);
    
    /* Taskbar items */
    gui_string(100, tb_y + 12, "Terminal", MEXA_FG);
    gui_string(180, tb_y + 12, "Files", MEXA_FG);
    gui_string(240, tb_y + 12, "Browser", MEXA_FG);
    
    /* Clock area */
    char clock_str[32];
    uint64_t ticks = timer_get_uptime();
    uint64_t mins = (ticks / TIMER_FREQ) / 60;
    uint64_t secs = (ticks / TIMER_FREQ) % 60;
    kprintf_to_buf(clock_str, sizeof(clock_str), "%02d:%02d", (int)(mins % 60), (int)secs);
    gui_string((int32_t)gui.width - 80, tb_y + 12, clock_str, MEXA_FG);
    
    /* MexaOS label */
    gui_string((int32_t)gui.width - 180, tb_y + 12, "MexaOS v" MEXAOS_VERSION, MEXA_ACCENT_LT);
}

void gui_draw_start_menu(void) {
    int32_t mw = 200;
    int32_t mh = 280;
    int32_t mx = 8;
    int32_t my = (int32_t)gui.height - 40 - mh;
    
    /* Glass panel */
    gui_fill_rounded_rect(mx, my, mw, mh, 8, 0xF0151525);
    gui_rounded_rect(mx, my, mw, mh, 8, MEXA_GLASS_BORDER);
    gui_fill_rect(mx, my, mw, 2, MEXA_ACCENT);
    
    /* Menu items */
    const char *items[] = {"Applications", "Settings", "Files", "Terminal", 
                           "AI Assistant", "Workspace", "System Info", "Reboot", "Shutdown"};
    int32_t item_y = my + 16;
    for (int i = 0; i < 9; i++) {
        if (i == 4) { /* Separator before AI */
            gui_fill_rect(mx + 12, item_y - 4, mw - 24, 1, MEXA_GLASS_BORDER);
        }
        gui_string(mx + 16, item_y, items[i], MEXA_FG);
        item_y += 28;
    }
}

void gui_draw_notification(const char *title, const char *message) {
    /* Find free slot */
    for (int i = 0; i < MAX_NOTIFICATIONS; i++) {
        if (!notifications[i].active) {
            strncpy(notifications[i].title, title, 63);
            notifications[i].title[63] = '\0';
            strncpy(notifications[i].message, message, 127);
            notifications[i].message[127] = '\0';
            notifications[i].expires_at = timer_get_uptime() + TIMER_FREQ * 5;
            notifications[i].active = 1;
            break;
        }
    }
}

static void gui_render_notifications(void) {
    int32_t nx = (int32_t)gui.width - 260;
    int32_t ny = 40;
    uint64_t now = timer_get_uptime();
    
    for (int i = 0; i < MAX_NOTIFICATIONS; i++) {
        if (!notifications[i].active) continue;
        
        if (now >= notifications[i].expires_at) {
            notifications[i].active = 0;
            continue;
        }
        
        /* Draw notification bubble */
        gui_fill_rounded_rect(nx, ny, 240, 60, 8, 0xF01A1A2E);
        gui_rounded_rect(nx, ny, 240, 60, 8, MEXA_ACCENT);
        gui_string(nx + 12, ny + 8, notifications[i].title, MEXA_ACCENT_LT);
        gui_string(nx + 12, ny + 28, notifications[i].message, MEXA_FG);
        
        ny += 70;
    }
}

static void gui_taskbar_update_clock(void) {
    /* Only update every second */
    static uint64_t last_update = 0;
    uint64_t now = timer_get_uptime();
    if (now - last_update < TIMER_FREQ) return;
    last_update = now;
    
    /* Redraw just the clock area */
    int32_t tb_y = (int32_t)gui.height - 40;
    gui_fill_rect((int32_t)gui.width - 100, tb_y + 8, 90, 20, 0xE6151525);
    
    char clock_str[32];
    uint64_t mins = (now / TIMER_FREQ) / 60;
    uint64_t secs = (now / TIMER_FREQ) % 60;
    kprintf_to_buf(clock_str, sizeof(clock_str), "%02d:%02d", (int)(mins % 60), (int)secs);
    gui_string((int32_t)gui.width - 80, tb_y + 12, clock_str, MEXA_FG);
}

/* ============================================================
 *  Mouse
 * ============================================================ */

void gui_mouse_move(int32_t dx, int32_t dy) {
    gui_mouse_hide();
    gui.mouse_x += dx;
    gui.mouse_y += dy;
    
    if (gui.mouse_x < 0) gui.mouse_x = 0;
    if (gui.mouse_y < 0) gui.mouse_y = 0;
    if ((uint32_t)gui.mouse_x >= gui.width) gui.mouse_x = gui.width - 1;
    if ((uint32_t)gui.mouse_y >= gui.height) gui.mouse_y = gui.height - 1;
    
    gui_mouse_draw();
}

void gui_mouse_draw(void) {
    if (!gui.mouse_visible) return;
    gui_draw_mouse_cursor(gui.mouse_x, gui.mouse_y);
}

void gui_mouse_hide(void) {
    /* Redraw the area under cursor */
    gui_fill_rect(gui.mouse_x, gui.mouse_y, 12, 16, MEXA_BG);
}

static void gui_draw_mouse_cursor(int32_t x, int32_t y) {
    for (int row = 0; row < 16; row++) {
        uint16_t line = mouse_cursor[row];
        for (int col = 0; col < 12; col++) {
            if (line & (1 << col)) {
                gui_pixel(x + col, y + row, COLOR_WHITE);
            }
        }
    }
}

/* ============================================================
 *  Theme
 * ============================================================ */

void gui_set_theme(const char *theme_name) {
    if (strcmp(theme_name, "matrix") == 0) {
        gui.bg_color = 0xFF000000;
        gui.accent_color = 0xFF00FF00;
        gui.fg_color = 0xFF00FF00;
    } else if (strcmp(theme_name, "ocean") == 0) {
        gui.bg_color = 0xFF001020;
        gui.accent_color = 0xFF00BFFF;
        gui.fg_color = 0xFF87CEEB;
    } else if (strcmp(theme_name, "sunset") == 0) {
        gui.bg_color = 0xFF1A0A00;
        gui.accent_color = 0xFFFF6600;
        gui.fg_color = 0xFFFFCC99;
    } else {
        /* Default MexaOS theme */
        gui.bg_color = MEXA_BG;
        gui.accent_color = MEXA_ACCENT;
        gui.fg_color = MEXA_FG;
    }
}

/* ============================================================
 *  Welcome Window Draw Callback
 * ============================================================ */

static void gui_welcome_window_draw(struct window *win) {
    /* Title area with gradient */
    gui_gradient_h(win->x, win->y + 30, win->width, win->height - 30,
                   MEXA_BG, 0xFF101025);
    
    /* Content */
    gui_string_centered(win->x, win->y + 60, win->width, 
                        "Welcome to MexaOS", COLOR_WHITE);
    gui_string_centered(win->x, win->y + 85, win->width,
                        "Intent-Driven Operating System", MEXA_ACCENT_LT);
    
    gui_string(win->x + 30, win->y + 120, "Version:    " MEXAOS_VERSION, MEXA_FG);
    gui_string(win->x + 30, win->y + 140, "Build:      " STR(MEXAOS_BUILD), MEXA_FG);
    gui_string(win->x + 30, win->y + 160, "Codename:  \"" MEXAOS_CODENAME "\"", MEXA_FG);
    
    /* Tips */
    gui_string(win->x + 30, win->y + 200, "Tip: Type your intent naturally:", MEXA_ACCENT);
    gui_string(win->x + 30, win->y + 220, "\"Organize my desktop for coding\"", MEXA_FG);
    
    /* Close button */
    gui_fill_rounded_rect(win->x + win->width / 2 - 40, win->y + win->height - 40,
                          80, 28, 6, MEXA_ACCENT);
    gui_string_centered(win->x + win->width / 2 - 40, win->y + win->height - 34,
                        80, "Got it!", COLOR_WHITE);
}

/* ============================================================
 *  Math Approximations (no libc in kernel)
 * ============================================================ */

static float sqrtf_approx(float x) {
    if (x <= 0) return 0;
    float r = x;
    for (int i = 0; i < 8; i++) {
        r = (r + x / r) * 0.5f;
    }
    return r;
}

static float sinf_approx(float x) {
    /* Taylor series approximation */
    float x2 = x * x;
    float x3 = x2 * x;
    float x5 = x3 * x2;
    float x7 = x5 * x2;
    return x - x3 / 6.0f + x5 / 120.0f - x7 / 5040.0f;
}

static float cosf_approx(float x) {
    float x2 = x * x;
    float x4 = x2 * x2;
    float x6 = x4 * x2;
    return 1.0f - x2 / 2.0f + x4 / 24.0f - x6 / 720.0f;
}

static int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

static size_t strlen(const char *s) {
    size_t n = 0;
    while (*s++) n++;
    return n;
}

static char *strncpy(char *dst, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i]; i++)
        dst[i] = src[i];
    for (; i < n; i++)
        dst[i] = '\0';
    return dst;
}

static void *memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

/* Simple string to number helper for STR macro */
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
