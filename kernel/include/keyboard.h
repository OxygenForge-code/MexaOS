/* ============================================================
 *  MexaOS Keyboard Driver
 * ============================================================ */

#ifndef MEXAOS_KEYBOARD_H
#define MEXAOS_KEYBOARD_H

#include <stdint.h>
#include <stddef.h>

/* ─── Keyboard Ports ─── */
#define KEYBOARD_DATA_PORT      0x60
#define KEYBOARD_STATUS_PORT    0x64

/* ─── Key Codes ─── */
#define KEY_ESC         0x01
#define KEY_ENTER       0x1C
#define KEY_BACKSPACE   0x0E
#define KEY_TAB         0x0F
#define KEY_SPACE       0x39

#define KEY_LSHIFT      0x2A
#define KEY_RSHIFT      0x36
#define KEY_LCTRL       0x1D
#define KEY_LALT        0x38

#define KEY_UP          0x48
#define KEY_DOWN        0x50
#define KEY_LEFT        0x4B
#define KEY_RIGHT       0x4D

#define KEY_HOME        0x47
#define KEY_END         0x4F
#define KEY_DELETE      0x53

#define KEY_F1          0x3B
#define KEY_F2          0x3C
#define KEY_F3          0x3D
#define KEY_F4          0x3E
#define KEY_F5          0x3F
#define KEY_F6          0x40
#define KEY_F7          0x41
#define KEY_F8          0x42
#define KEY_F9          0x43
#define KEY_F10         0x44
#define KEY_F11         0x57
#define KEY_F12         0x58

#define KEY_AI_INTENT   0x01  /* F1 = AI Intent */
#define KEY_WS_1        0x02  /* 1 */
#define KEY_WS_2        0x03  /* 2 */
#define KEY_WS_3        0x04  /* 3 */
#define KEY_WS_4        0x05  /* 4 */
#define KEY_WS_5        0x06  /* 5 */
#define KEY_WS_QUICK    0x10  /* Q */
#define KEY_WS_FOCUS    0x12  /* E */
#define KEY_WS_SETTINGS 0x2E  /* C */

/* ─── Functions ─── */
void keyboard_init(void);
uint8_t keyboard_get_scancode(void);
char keyboard_scancode_to_ascii(uint8_t scancode);
int keyboard_buffer_empty(void);
void keyboard_put_buffer(uint8_t key);
char keyboard_read_char(void);
int keyboard_read_line(char *buf, int max_len, uint8_t echo);
uint8_t keyboard_wait_key(void);
void keyboard_update_leds(void);

#endif
