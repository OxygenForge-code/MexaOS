/* ============================================================
 *  MexaOS Keyboard Driver
 *  PS/2 Keyboard with MexaOS key bindings
 * ============================================================ */

#include <stdint.h>
#include <stddef.h>

#include "../../include/mexaos.h"
#include "include/keyboard.h"
#include "include/interrupt.h"
#include "include/vga.h"
#include "include/memory.h"

#define KEYBOARD_BUFFER_SIZE 256

static volatile uint8_t keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static volatile uint32_t keyboard_buffer_head = 0;
static volatile uint32_t keyboard_buffer_tail = 0;

static struct {
    uint8_t shift;
    uint8_t ctrl;
    uint8_t alt;
    uint8_t caps_lock;
    uint8_t num_lock;
    uint8_t scroll_lock;
} kb_state;

static const char scancode_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

static const char scancode_ascii_shift[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

static void keyboard_irq_handler(struct interrupt_frame *frame) {
    (void)frame;
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    if (scancode & 0x80) {
        scancode &= 0x7F;
        switch (scancode) {
            case KEY_LSHIFT: case KEY_RSHIFT: kb_state.shift = 0; return;
            case KEY_LCTRL: kb_state.ctrl = 0; return;
            case KEY_LALT: kb_state.alt = 0; return;
        }
        return;
    }
    
    switch (scancode) {
        case KEY_LSHIFT: case KEY_RSHIFT: kb_state.shift = 1; return;
        case KEY_LCTRL: kb_state.ctrl = 1; return;
        case KEY_LALT: kb_state.alt = 1; return;
        case KEY_UP: keyboard_put_buffer(KEY_UP); return;
        case KEY_DOWN: keyboard_put_buffer(KEY_DOWN); return;
        case KEY_LEFT: keyboard_put_buffer(KEY_LEFT); return;
        case KEY_RIGHT: keyboard_put_buffer(KEY_RIGHT); return;
        case KEY_HOME: keyboard_put_buffer(KEY_HOME); return;
        case KEY_END: keyboard_put_buffer(KEY_END); return;
        case KEY_DELETE: keyboard_put_buffer(KEY_DELETE); return;
    }
    
    if (kb_state.ctrl) {
        if (scancode == KEY_F1) {
            keyboard_put_buffer(KEY_AI_INTENT);
            return;
        }
        if (scancode >= KEY_WS_1 && scancode <= KEY_WS_5) {
            keyboard_put_buffer(scancode);
            return;
        }
        switch (scancode) {
            case KEY_WS_QUICK: keyboard_put_buffer(KEY_WS_QUICK); return;
            case KEY_WS_FOCUS: keyboard_put_buffer(KEY_WS_FOCUS); return;
            case KEY_WS_SETTINGS: keyboard_put_buffer(KEY_WS_SETTINGS); return;
        }
    }
    
    char c = keyboard_scancode_to_ascii(scancode);
    if (c) keyboard_put_buffer(c);
}

void keyboard_init(void) {
    kb_state.shift = 0;
    kb_state.ctrl = 0;
    kb_state.alt = 0;
    kb_state.caps_lock = 0;
    kb_state.num_lock = 0;
    kb_state.scroll_lock = 0;
    
    register_irq_handler(1, keyboard_irq_handler);
    pic_unmask_irq(1);
    keyboard_update_leds();
}

uint8_t keyboard_get_scancode(void) {
    return inb(KEYBOARD_DATA_PORT);
}

char keyboard_scancode_to_ascii(uint8_t scancode) {
    if (scancode >= sizeof(scancode_ascii)) return 0;
    
    const char *table = kb_state.shift ? scancode_ascii_shift : scancode_ascii;
    char c = table[scancode];
    
    if (kb_state.caps_lock && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) {
        c ^= 0x20;
    }
    
    return c;
}

int keyboard_buffer_empty(void) {
    return keyboard_buffer_head == keyboard_buffer_tail;
}

void keyboard_put_buffer(uint8_t key) {
    uint32_t next = (keyboard_buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
    if (next != keyboard_buffer_tail) {
        keyboard_buffer[keyboard_buffer_head] = key;
        keyboard_buffer_head = next;
    }
}

char keyboard_read_char(void) {
    while (keyboard_buffer_empty()) {
        __asm__ __volatile__("pause");
    }
    char c = keyboard_buffer[keyboard_buffer_tail];
    keyboard_buffer_tail = (keyboard_buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}

int keyboard_read_line(char *buf, int max_len, uint8_t echo) {
    int pos = 0;
    while (1) {
        char key = keyboard_read_char();
        
        if (key == '\n') {
            if (echo) vga_putchar('\n');
            buf[pos] = '\0';
            return pos;
        }
        if (key == '\b' && pos > 0) {
            pos--;
            if (echo) { vga_putchar('\b'); vga_putchar(' '); vga_putchar('\b'); }
            continue;
        }
        if (key == KEY_HOME) { pos = 0; continue; }
        if (key == KEY_END) { continue; }
        if (key == KEY_AI_INTENT) { buf[pos] = '\0'; return -2; }
        if (key >= ' ' && pos < max_len - 1) {
            buf[pos++] = key;
            if (echo) vga_putchar(key);
        }
    }
}

uint8_t keyboard_wait_key(void) {
    return keyboard_read_char();
}

void keyboard_update_leds(void) {
    uint8_t status = 0;
    if (kb_state.scroll_lock) status |= 1;
    if (kb_state.num_lock) status |= 2;
    if (kb_state.caps_lock) status |= 4;
    
    while (inb(KEYBOARD_STATUS_PORT) & 2);
    outb(KEYBOARD_DATA_PORT, 0xED);
    while (inb(KEYBOARD_STATUS_PORT) & 2);
    outb(KEYBOARD_DATA_PORT, status);
}
