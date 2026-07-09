/* ============================================================
 *  MexaOS Window Manager Header
 *  Compositing window manager with glass effects
 * ============================================================ */

#ifndef WINDOW_H
#define WINDOW_H

#include "../../include/mexaos.h"
#include "gui.h"

/* ─── Window ─── */
struct window {
    int32_t x, y;
    int32_t width, height;
    char title[WIN_TITLE_LEN];
    
    /* State */
    uint8_t visible;
    uint8_t focused;
    uint8_t minimized;
    uint8_t maximized;
    uint8_t movable;
    uint8_t resizable;
    
    /* Style */
    uint32_t bg_color;
    uint32_t border_color;
    uint32_t title_bg;
    uint32_t title_fg;
    uint8_t has_shadow;
    uint8_t has_glass;
    uint8_t rounded;
    
    /* Content buffer */
    uint32_t *buffer;
    
    /* Callbacks */
    void (*on_draw)(struct window *win);
    void (*on_focus)(struct window *win);
    void (*on_unfocus)(struct window *win);
    void (*on_close)(struct window *win);
    void (*on_click)(struct window *win, int32_t x, int32_t y);
    
    /* Linked list */
    struct window *next;
    struct window *prev;
};

/* ─── Window Manager Functions ─── */
void window_manager_init(void);
struct window *window_create(const char *title, int32_t x, int32_t y, 
                              int32_t w, int32_t h, uint32_t flags);
void window_destroy(struct window *win);
void window_show(struct window *win);
void window_hide(struct window *win);
void window_focus(struct window *win);
void window_move(struct window *win, int32_t x, int32_t y);
void window_resize(struct window *win, int32_t w, int32_t h);
void window_set_title(struct window *win, const char *title);
void window_draw(struct window *win);
void window_draw_all(void);
void window_close(struct window *win);
struct window *window_find_at(int32_t x, int32_t y);
void window_raise(struct window *win);
void window_lower(struct window *win);

/* Window creation flags */
#define WIN_FLAG_NONE       0x00
#define WIN_FLAG_NO_BORDER  0x01
#define WIN_FLAG_NO_TITLE   0x02
#define WIN_FLAG_FIXED      0x04
#define WIN_FLAG_GLASS      0x08
#define WIN_FLAG_SHADOW     0x10
#define WIN_FLAG_ROUNDED    0x20
#define WIN_FLAG_FULLSCREEN 0x40
#define WIN_FLAG_POPUP      0x80

#endif /* WINDOW_H */
