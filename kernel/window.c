/* ============================================================
 *  MexaOS Window Manager
 *  Compositing window manager with glass effects
 * ============================================================ */

#include "../include/mexaos.h"
#include "include/kernlib.h"
#include "include/memory.h"
#include "include/gui.h"
#include "include/window.h"

/* ─── Forward Declarations ─── */
static void window_draw_title_bar(struct window *win);
static void window_draw_border(struct window *win);
static void window_draw_content(struct window *win);
static void window_add_to_list(struct window *win);
static void window_remove_from_list(struct window *win);

/* ─── Window List ─── */
static struct window *window_list_head = NULL;
static struct window *window_list_tail = NULL;
static int window_count = 0;
static struct window *focused_window = NULL;

/* ─── Desktop wallpaper buffer ─── */
static uint32_t *desktop_buffer = NULL;

/* ============================================================
 *  Window Manager Core
 * ============================================================ */

void window_manager_init(void) {
    window_list_head = NULL;
    window_list_tail = NULL;
    window_count = 0;
    focused_window = NULL;
    
    kprintf("[WM] Window manager initialized. Max windows: %d\n", MAX_WINDOWS);
}

/* ============================================================
 *  Window Lifecycle
 * ============================================================ */

struct window *window_create(const char *title, int32_t x, int32_t y,
                              int32_t w, int32_t h, uint32_t flags) {
    if (window_count >= MAX_WINDOWS) {
        kprintf("[WM] Error: Maximum window count reached (%d)\n", MAX_WINDOWS);
        return NULL;
    }
    
    struct window *win = (struct window *)kmalloc(sizeof(struct window));
    if (!win) {
        kprintf("[WM] Error: Failed to allocate window structure\n");
        return NULL;
    }
    
    /* Initialize window properties */
    memset(win, 0, sizeof(struct window));
    
    win->x = x;
    win->y = y;
    win->width = w;
    win->height = h;
    win->visible = 1;
    win->focused = 0;
    win->minimized = 0;
    win->maximized = 0;
    win->movable = !(flags & WIN_FLAG_FIXED);
    win->resizable = !(flags & WIN_FLAG_FIXED);
    win->has_shadow = (flags & WIN_FLAG_SHADOW) != 0;
    win->has_glass = (flags & WIN_FLAG_GLASS) != 0;
    win->rounded = (flags & WIN_FLAG_ROUNDED) != 0;
    win->next = NULL;
    win->prev = NULL;
    
    /* Copy title */
    strncpy(win->title, title, WIN_TITLE_LEN - 1);
    win->title[WIN_TITLE_LEN - 1] = '\0';
    
    /* Set default colors based on theme */
    win->bg_color = MEXA_BG;
    win->border_color = MEXA_GLASS_BORDER;
    win->title_bg = MEXA_ACCENT;
    win->title_fg = COLOR_WHITE;
    
    /* Override for special flags */
    if (flags & WIN_FLAG_NO_BORDER) {
        win->border_color = 0;
    }
    if (flags & WIN_FLAG_NO_TITLE) {
        win->title_bg = 0;
    }
    
    /* Allocate content buffer */
    win->buffer = (uint32_t *)kmalloc(w * h * sizeof(uint32_t));
    if (!win->buffer) {
        kprintf("[WM] Error: Failed to allocate window buffer\n");
        kfree(win);
        return NULL;
    }
    
    /* Clear buffer to transparent */
    memset(win->buffer, 0, w * h * sizeof(uint32_t));
    
    /* Add to window list */
    window_add_to_list(win);
    window_count++;
    
    kprintf("[WM] Created window '%s' (%dx%d+%d+%d)\n", title, w, h, x, y);
    return win;
}

void window_destroy(struct window *win) {
    if (!win) return;
    
    window_remove_from_list(win);
    
    if (win->buffer) {
        kfree(win->buffer);
        win->buffer = NULL;
    }
    
    if (focused_window == win) {
        focused_window = window_list_tail;
        if (focused_window) focused_window->focused = 1;
    }
    
    kfree(win);
    window_count--;
    
    /* Redraw all windows to fill the gap */
    window_draw_all();
}

void window_show(struct window *win) {
    if (!win) return;
    win->visible = 1;
    window_focus(win);
    window_draw(win);
}

void window_hide(struct window *win) {
    if (!win) return;
    win->visible = 0;
    window_draw_all();
}

/* ============================================================
 *  Focus & Z-Order
 * ============================================================ */

void window_focus(struct window *win) {
    if (!win || win == focused_window) return;
    
    /* Unfocus previous */
    if (focused_window) {
        focused_window->focused = 0;
        if (focused_window->on_unfocus)
            focused_window->on_unfocus(focused_window);
        window_draw(focused_window);
    }
    
    /* Focus new */
    focused_window = win;
    win->focused = 1;
    window_raise(win);
    
    if (win->on_focus)
        win->on_focus(win);
    
    window_draw(win);
}

void window_raise(struct window *win) {
    if (!win || win == window_list_tail) return;
    
    window_remove_from_list(win);
    window_add_to_list(win);
}

void window_lower(struct window *win) {
    if (!win || win == window_list_head) return;
    
    window_remove_from_list(win);
    
    /* Insert at head */
    win->next = window_list_head;
    win->prev = NULL;
    if (window_list_head)
        window_list_head->prev = win;
    window_list_head = win;
    if (!window_list_tail)
        window_list_tail = win;
}

/* ============================================================
 *  Geometry
 * ============================================================ */

void window_move(struct window *win, int32_t x, int32_t y) {
    if (!win || !win->movable) return;
    
    /* Erase old position */
    window_draw_all();
    
    win->x = x;
    win->y = y;
    
    if (win->visible)
        window_draw(win);
}

void window_resize(struct window *win, int32_t w, int32_t h) {
    if (!win || !win->resizable) return;
    if (w < 100) w = 100;
    if (h < 60) h = 60;
    
    /* Reallocate buffer */
    uint32_t *new_buf = (uint32_t *)kmalloc(w * h * sizeof(uint32_t));
    if (!new_buf) return;
    
    memset(new_buf, 0, w * h * sizeof(uint32_t));
    
    /* Copy old content */
    int32_t copy_w = w < win->width ? w : win->width;
    int32_t copy_h = h < win->height ? h : win->height;
    for (int32_t row = 0; row < copy_h; row++) {
        for (int32_t col = 0; col < copy_w; col++) {
            new_buf[row * w + col] = win->buffer[row * win->width + col];
        }
    }
    
    kfree(win->buffer);
    win->buffer = new_buf;
    win->width = w;
    win->height = h;
    
    if (win->visible)
        window_draw(win);
}

void window_set_title(struct window *win, const char *title) {
    if (!win) return;
    strncpy(win->title, title, WIN_TITLE_LEN - 1);
    win->title[WIN_TITLE_LEN - 1] = '\0';
    window_draw(win);
}

/* ============================================================
 *  Drawing
 * ============================================================ */

void window_draw(struct window *win) {
    if (!win || !win->visible || win->minimized) return;
    
    /* Shadow */
    if (win->has_shadow) {
        gui_shadow(win->x, win->y, win->width, win->height, 12);
    }
    
    /* Window background */
    if (win->has_glass) {
        gui_fill_rounded_rect(win->x, win->y, win->width, win->height, 
                              win->rounded ? 8 : 0, MEXA_GLASS_BG);
    } else {
        gui_fill_rounded_rect(win->x, win->y, win->width, win->height,
                              win->rounded ? 8 : 0, win->bg_color);
    }
    
    /* Border */
    window_draw_border(win);
    
    /* Title bar */
    window_draw_title_bar(win);
    
    /* Content area */
    window_draw_content(win);
    
    /* Callback for custom drawing */
    if (win->on_draw)
        win->on_draw(win);
}

void window_draw_all(void) {
    /* Redraw desktop background first */
    gui_draw_desktop();
    
    /* Draw all windows in z-order (bottom to top) */
    struct window *win = window_list_head;
    while (win) {
        if (win->visible && !win->minimized) {
            window_draw(win);
        }
        win = win->next;
    }
    
    /* Draw taskbar on top */
    gui_draw_taskbar();
}

static void window_draw_title_bar(struct window *win) {
    if (win->title_bg == 0) return; /* No title bar */
    
    int32_t tb_height = WIN_TITLE_HEIGHT;
    
    if (win->focused) {
        /* Active title bar with accent gradient */
        gui_gradient_h(win->x, win->y, win->width, tb_height, MEXA_ACCENT, MEXA_ACCENT_DK);
    } else {
        /* Inactive title bar */
        gui_fill_rect(win->x, win->y, win->width, tb_height, 0xFF2A2A3A);
    }
    
    /* Title text */
    int32_t title_x = win->x + 10;
    int32_t title_y = win->y + (tb_height - 8) / 2;
    gui_string(title_x, title_y, win->title, win->focused ? COLOR_WHITE : COLOR_GRAY);
    
    /* Window controls (right side) */
    int32_t btn_y = win->y + (tb_height - 14) / 2;
    int32_t btn_x = win->x + win->width - 20;
    
    /* Close button */
    gui_fill_circle(btn_x, btn_y + 6, 6, MEXA_ERROR);
    btn_x -= 20;
    
    /* Maximize button */
    gui_fill_circle(btn_x, btn_y + 6, 6, MEXA_WARN);
    btn_x -= 20;
    
    /* Minimize button */
    gui_fill_circle(btn_x, btn_y + 6, 6, MEXA_SUCCESS);
    
    /* Bottom border of title bar */
    gui_fill_rect(win->x, win->y + tb_height, win->width, 1, MEXA_GLASS_BORDER);
}

static void window_draw_border(struct window *win) {
    if (win->border_color == 0) return;
    
    if (win->rounded) {
        gui_rounded_rect(win->x, win->y, win->width, win->height, 8, 
                        win->focused ? MEXA_ACCENT : win->border_color);
    } else {
        gui_rect(win->x, win->y, win->width, win->height,
                win->focused ? MEXA_ACCENT : win->border_color);
    }
}

static void window_draw_content(struct window *win) {
    /* Base content fill */
    int32_t content_y = win->y + WIN_TITLE_HEIGHT + 1;
    int32_t content_h = win->height - WIN_TITLE_HEIGHT - 1;
    
    if (win->has_glass) {
        /* Semi-transparent glass content area */
        gui_fill_rect(win->x + 1, content_y, win->width - 2, content_h, 0x18FFFFFF);
    } else {
        gui_fill_rect(win->x + 1, content_y, win->width - 2, content_h, win->bg_color);
    }
    
    /* If window has a buffer, composite it */
    if (win->buffer) {
        for (int32_t row = 0; row < content_h; row++) {
            for (int32_t col = 0; col < win->width - 2; col++) {
                uint32_t pixel = win->buffer[row * win->width + col];
                if (pixel & 0xFF000000) { /* Alpha check */
                    gui_pixel(win->x + 1 + col, content_y + row, pixel);
                }
            }
        }
    }
}

/* ============================================================
 *  Hit Testing
 * ============================================================ */

struct window *window_find_at(int32_t x, int32_t y) {
    /* Search from top (tail) to bottom (head) */
    struct window *win = window_list_tail;
    while (win) {
        if (win->visible && !win->minimized) {
            if (x >= win->x && x < win->x + win->width &&
                y >= win->y && y < win->y + win->height) {
                return win;
            }
        }
        win = win->prev;
    }
    return NULL;
}

/* ============================================================
 *  Window Actions
 * ============================================================ */

void window_close(struct window *win) {
    if (!win) return;
    
    if (win->on_close)
        win->on_close(win);
    
    window_destroy(win);
}

/* ============================================================
 *  Window List Management
 * ============================================================ */

static void window_add_to_list(struct window *win) {
    if (!window_list_head) {
        window_list_head = win;
        window_list_tail = win;
        win->prev = NULL;
        win->next = NULL;
        return;
    }
    
    /* Add to tail (top of z-order) */
    win->prev = window_list_tail;
    win->next = NULL;
    window_list_tail->next = win;
    window_list_tail = win;
}

static void window_remove_from_list(struct window *win) {
    if (win->prev)
        win->prev->next = win->next;
    else
        window_list_head = win->next;
    
    if (win->next)
        win->next->prev = win->prev;
    else
        window_list_tail = win->prev;
    
    win->prev = NULL;
    win->next = NULL;
}

/* ============================================================
 *  Default Window Callbacks
 * ============================================================ */

/* These can be overridden per-window via the callback pointers */

/* ============================================================
 *  Utility Functions
 * ============================================================ */

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
