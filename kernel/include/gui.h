/* ============================================================
 *  MexaOS GUI Subsystem Header
 *  Framebuffer graphics with MexaOS indigo theme
 * ============================================================ */

#ifndef GUI_H
#define GUI_H

#include "../../include/mexaos.h"

/* ─── Color (32-bit ARGB) ─── */
#define COLOR_BLACK     0xFF000000
#define COLOR_WHITE     0xFFFFFFFF
#define COLOR_GRAY      0xFF808080
#define COLOR_DKGRAY    0xFF404040
#define COLOR_LTGRAY    0xFFC0C0C0

/* MexaOS Theme Palette */
#define MEXA_BG         0xFF050507      /* Deep black-blue */
#define MEXA_FG         0xFFFFFFFF      /* White */
#define MEXA_ACCENT     0xFF6366F1      /* Indigo */
#define MEXA_ACCENT_LT  0xFF818CF8      /* Light indigo */
#define MEXA_ACCENT_DK  0xFF4F46E5      /* Dark indigo */
#define MEXA_PINK       0xFFEC4899      /* Pink accent */
#define MEXA_CYAN       0xFF06B6D4      /* Cyan accent */
#define MEXA_AMBER      0xFFF59E0B      /* Amber accent */
#define MEXA_SUCCESS    0xFF22C55E      /* Green */
#define MEXA_WARN       0xFFEAB308      /* Yellow */
#define MEXA_ERROR      0xFFEF4444      /* Red */
#define MEXA_GLASS_BG   0x08FFFFFF      /* Glass background */
#define MEXA_GLASS_BORDER 0x15FFFFFF    /* Glass border */

/* ─── GUI Context ─── */
struct gui_context {
    uint32_t *framebuffer;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t bpp;
    
    /* Theme */
    uint32_t bg_color;
    uint32_t fg_color;
    uint32_t accent_color;
    uint32_t glass_bg;
    uint32_t glass_border;
    
    /* Font */
    uint32_t font_color;
    uint32_t font_size;
    
    /* Cursor */
    int32_t mouse_x;
    int32_t mouse_y;
    uint8_t mouse_visible;
};

/* ─── Functions ─── */
void gui_init(uint32_t fb_addr, uint32_t width, uint32_t height, uint32_t pitch, uint8_t bpp);
void gui_clear(uint32_t color);
void gui_pixel(int32_t x, int32_t y, uint32_t color);
uint32_t gui_get_pixel(int32_t x, int32_t y);
void gui_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
void gui_fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
void gui_rounded_rect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color);
void gui_fill_rounded_rect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color);
void gui_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);
void gui_circle(int32_t x, int32_t y, int32_t r, uint32_t color);
void gui_fill_circle(int32_t x, int32_t y, int32_t r, uint32_t color);
void gui_triangle(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3, uint32_t color);

/* Text */
void gui_char(int32_t x, int32_t y, char c, uint32_t color);
void gui_string(int32_t x, int32_t y, const char *s, uint32_t color);
void gui_string_centered(int32_t x, int32_t y, int32_t w, const char *s, uint32_t color);

/* Effects */
void gui_blur(int32_t x, int32_t y, int32_t w, int32_t h, int radius);
void gui_gradient_v(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t top, uint32_t bottom);
void gui_gradient_h(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t left, uint32_t right);
void gui_glow(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color, int intensity);
void gui_shadow(int32_t x, int32_t y, int32_t w, int32_t h, int blur_radius);

/* Desktop */
void gui_desktop_start(void);
void gui_draw_desktop(void);
void gui_draw_taskbar(void);
void gui_draw_start_menu(void);
void gui_draw_notification(const char *title, const char *message);

/* Mouse */
void gui_mouse_move(int32_t dx, int32_t dy);
void gui_mouse_draw(void);
void gui_mouse_hide(void);

/* Theme */
void gui_set_theme(const char *theme_name);

/* Utility */
uint32_t gui_blend(uint32_t bg, uint32_t fg, uint8_t alpha);
uint32_t gui_color(uint8_t r, uint8_t g, uint8_t b);

#endif /* GUI_H */
