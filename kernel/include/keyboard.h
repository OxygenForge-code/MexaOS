/* ============================================================
 *  MexaOS Keyboard Driver Header
 * ============================================================ */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../../include/mexaos.h"

/* ─── Special Key Codes ─── */
#define KEY_UP          0x80
#define KEY_DOWN        0x81
#define KEY_LEFT        0x82
#define KEY_RIGHT       0x83
#define KEY_HOME        0x84
#define KEY_END         0x85
#define KEY_PGUP        0x86
#define KEY_PGDOWN      0x87
#define KEY_INSERT      0x88
#define KEY_DELETE      0x89

/* ─── MexaOS Special Keys ─── */
#define KEY_AI_INTENT   0xF0    /* Meta+Space: AI Intent prompt */
#define KEY_WS_1        0xF1    /* Meta+1: Workspace 1 */
#define KEY_WS_2        0xF2    /* Meta+2: Workspace 2 */
#define KEY_WS_3        0xF3    /* Meta+3: Workspace 3 */
#define KEY_WS_4        0xF4    /* Meta+4: Workspace 4 */
#define KEY_WS_5        0xF5    /* Meta+5: Workspace 5 */
#define KEY_WS_QUICK    0xF6    /* Meta+Q: Quick actions */
#define KEY_WS_FOCUS    0xF7    /* Meta+E: Focus mode */
#define KEY_WS_SETTINGS 0xF8    /* Meta+C: Control center */

/* ─── Functions ─── */
void keyboard_init(void);
void keyboard_put_buffer(uint8_t key);
uint8_t keyboard_get_buffer(void);
uint8_t keyboard_peek_buffer(void);
int keyboard_buffer_empty(void);
void keyboard_buffer_clear(void);

uint8_t keyboard_shift(void);
uint8_t keyboard_ctrl(void);
uint8_t keyboard_alt(void);
uint8_t keyboard_meta(void);
uint8_t keyboard_caps_lock(void);

void keyboard_update_leds(void);
void keyboard_set_handler(void (*handler)(uint8_t scancode, char ascii, uint8_t press));
uint8_t keyboard_wait_key(void);
int keyboard_read_line(char *buf, int max_len, uint8_t echo);

#endif /* KEYBOARD_H */
