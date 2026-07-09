/* ============================================================
 *  MexaOS Window Manager
 * ============================================================ */

#ifndef MEXAOS_WINDOW_H
#define MEXAOS_WINDOW_H

#include <stdint.h>

void wm_init(void);
void wm_create_window(const char *title, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
void wm_render(void);

#endif
