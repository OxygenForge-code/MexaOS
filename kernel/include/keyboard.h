/* ============================================================
 *  MexaOS Keyboard Driver
 * ============================================================ */

#ifndef MEXAOS_KEYBOARD_H
#define MEXAOS_KEYBOARD_H

#include <stdint.h>

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

#define KEY_F1          0x3B
#define KEY_F12         0x58

/* ─── Functions ─── */
void keyboard_init(void);
uint8_t keyboard_get_scancode(void);
char keyboard_scancode_to_ascii(uint8_t scancode);
int keyboard_buffer_empty(void);
char keyboard_read_char(void);
void keyboard_read_line(char *buf, size_t size, int echo);
void keyboard_wait_key(void);
void keyboard_update_leds(uint8_t status);

#endif
