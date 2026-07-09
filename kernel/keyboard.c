/* ============================================================
 *  MexaOS Keyboard Driver
 *  PS/2 keyboard with full keymap, modifiers, and AI shortcut
 * ============================================================ */

#include "include/keyboard.h"
#include "include/interrupt.h"
#include "include/vga.h"

static struct {
    uint8_t shift_pressed;
    uint8_t ctrl_pressed;
    uint8_t alt_pressed;
    uint8_t meta_pressed;
    uint8_t caps_lock;
    uint8_t num_lock;
    uint8_t scroll_lock;
    uint8_t led_state;
} kb_state;

#define KEY_BUFFER_SIZE 256
static volatile uint8_t key_buffer[KEY_BUFFER_SIZE];
static volatile int key_buffer_head = 0;
static volatile int key_buffer_tail = 0;

static const char keymap_lower[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t', 'q','w','e','r','t','y','u','i','o','p','[',']', '\n',
    0, 'a','s','d','f','g','h','j','k','l',';','\'','`',
    0, '\\','z','x','c','v','b','n','m',',','.','/',
    0, '*', 0, ' ', 0,
    0,0,0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0,
    0, 0,
};

static const char keymap_upper[128] = {
    0, 27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t', 'Q','W','E','R','T','Y','U','I','O','P','{','}', '\n',
    0, 'A','S','D','F','G','H','J','K','L',':','"','~',
    0, '|','Z','X','C','V','B','N','M','<','>','?',
    0, '*', 0, ' ', 0,
    0,0,0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0,
    0, 0,
};

static void (*key_event_handler)(uint8_t scancode, char ascii, uint8_t press);

static void keyboard_irq_handler(struct interrupt_frame *frame) {
    (void)frame;
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    if (scancode == 0xE0) {
        uint8_t ext = inb(KEYBOARD_DATA_PORT);
        switch (ext) {
            case 0x1D: kb_state.ctrl_pressed = 1; return;
            case 0x9D: kb_state.ctrl_pressed = 0; return;
            case 0x38: kb_state.alt_pressed = 1; return;
            case 0xB8: kb_state.alt_pressed = 0; return;
            case 0x5B: case 0x5C: kb_state.meta_pressed = 1; return;
            case 0xDB: case 0xDC: kb_state.meta_pressed = 0; return;
            case 0x48: keyboard_put_buffer(KEY_UP); return;
            case 0x50: keyboard_put_buffer(KEY_DOWN); return;
            case 0x4B: keyboard_put_buffer(KEY_LEFT); return;
            case 0x4D: keyboard_put_buffer(KEY_RIGHT); return;
            case 0x47: keyboard_put_buffer(KEY_HOME); return;
            case 0x4F: keyboard_put_buffer(KEY_END); return;
            case 0x53: keyboard_put_buffer(KEY_DELETE); return;
        }
        return;
    }
    
    if (scancode & 0x80) {
        uint8_t key = scancode & 0x7F;
        switch (key) {
            case 0x2A: case 0x36: kb_state.shift_pressed = 0; break;
            case 0x1D: kb_state.ctrl_pressed = 0; break;
            case 0x38: kb_state.alt_pressed = 0; break;
        }
        if (key_event_handler) key_event_handler(key, 0, 0);
        return;
    }
    
    switch (scancode) {
        case 0x2A: case 0x36: kb_state.shift_pressed = 1; return;
        case 0x1D: kb_state.ctrl_pressed = 1; return;
        case 0x38: kb_state.alt_pressed = 1; return;
        case 0x3A: kb_state.caps_lock = !kb_state.caps_lock; keyboard_update_leds(); return;
        case 0x45: kb_state.num_lock = !kb_state.num_lock; keyboard_update_leds(); return;
    }
    
    char ascii = 0;
    if (scancode < 128) {
        int use_upper = kb_state.shift_pressed;
        if (kb_state.caps_lock) {
            char c = keymap_lower[scancode];
            if (c >= 'a' && c <= 'z') use_upper = !use_upper;
        }
        ascii = use_upper ? keymap_upper[scancode] : keymap_lower[scancode];
    }
    
    if (kb_state.meta_pressed && scancode == 0x39) {
        keyboard_put_buffer(KEY_AI_INTENT);
        return;
    }
    
    if (kb_state.meta_pressed) {
        switch (scancode) {
            case 0x02: keyboard_put_buffer(KEY_WS_1); return;
            case 0x03: keyboard_put_buffer(KEY_WS_2); return;
            case 0x04: keyboard_put_buffer(KEY_WS_3); return;
            case 0x05: keyboard_put_buffer(KEY_WS_4); return;
            case 0x06: keyboard_put_buffer(KEY_WS_5); return;
            case 0x10: keyboard_put_buffer(KEY_WS_QUICK); return;
            case 0x12: keyboard_put_buffer(KEY_WS_FOCUS); return;
            case 0x2E: keyboard_put_buffer(KEY_WS_SETTINGS); return;
        }
    }
    
    if (ascii) keyboard_put_buffer(ascii);
    if (key_event_handler) key_event_handler(scancode, ascii, 1);
}

void keyboard_init(void) {
    memset(&kb_state, 0, sizeof(kb_state));
    kb_state.num_lock = 1;
    key_buffer_head = 0; key_buffer_tail = 0;
    key_event_handler = NULL;
    while (inb(KEYBOARD_STATUS_PORT) & 1) inb(KEYBOARD_DATA_PORT);
    irq_register_handler(1, keyboard_irq_handler);
    pic_unmask_irq(1);
    keyboard_update_leds();
}

void keyboard_put_buffer(uint8_t key) {
    int next = (key_buffer_head + 1) % KEY_BUFFER_SIZE;
    if (next != key_buffer_tail) {
        key_buffer[key_buffer_head] = key;
        key_buffer_head = next;
    }
}

uint8_t keyboard_get_buffer(void) {
    if (key_buffer_head == key_buffer_tail) return 0;
    uint8_t key = key_buffer[key_buffer_tail];
    key_buffer_tail = (key_buffer_tail + 1) % KEY_BUFFER_SIZE;
    return key;
}

uint8_t keyboard_peek_buffer(void) {
    if (key_buffer_head == key_buffer_tail) return 0;
    return key_buffer[key_buffer_tail];
}

int keyboard_buffer_empty(void) { return key_buffer_head == key_buffer_tail; }
void keyboard_buffer_clear(void) { key_buffer_head = 0; key_buffer_tail = 0; }

uint8_t keyboard_shift(void) { return kb_state.shift_pressed; }
uint8_t keyboard_ctrl(void) { return kb_state.ctrl_pressed; }
uint8_t keyboard_alt(void) { return kb_state.alt_pressed; }
uint8_t keyboard_meta(void) { return kb_state.meta_pressed; }
uint8_t keyboard_caps_lock(void) { return kb_state.caps_lock; }

void keyboard_update_leds(void) {
    kb_state.led_state = 0;
    if (kb_state.scroll_lock) kb_state.led_state |= 1;
    if (kb_state.num_lock) kb_state.led_state |= 2;
    if (kb_state.caps_lock) kb_state.led_state |= 4;
    while (inb(KEYBOARD_STATUS_PORT) & 2);
    outb(KEYBOARD_DATA_PORT, 0xED);
    while (inb(KEYBOARD_STATUS_PORT) & 2);
    outb(KEYBOARD_DATA_PORT, kb_state.led_state);
}

void keyboard_set_handler(void (*handler)(uint8_t scancode, char ascii, uint8_t press)) {
    key_event_handler = handler;
}

uint8_t keyboard_wait_key(void) {
    while (keyboard_buffer_empty()) pause();
    return keyboard_get_buffer();
}

int keyboard_read_line(char *buf, int max_len, uint8_t echo) {
    int pos = 0;
    while (pos < max_len - 1) {
        uint8_t key = keyboard_wait_key();
        if (key == '\n' || key == '\r') {
            if (echo) vga_putc('\n');
            buf[pos] = '\0';
            return pos;
        }
        if (key == '\b' || key == 0x7F) {
            if (pos > 0) { pos--; if (echo) vga_putc('\b'); }
            continue;
        }
        if (key == KEY_LEFT) { if (pos > 0) pos--; continue; }
        if (key == KEY_RIGHT) { continue; }
        if (key == KEY_HOME) { pos = 0; continue; }
        if (key == KEY_END) { continue; }
        if (key == KEY_AI_INTENT) { buf[pos] = '\0'; return -2; }
        if (key == 3) { buf[0] = '\0'; return -1; }
        if (key == 12) { vga_clear(); continue; }
        if (key >= 32 && key < 127) {
            buf[pos++] = key;
            if (echo) vga_putc(key);
        }
    }
    buf[pos] = '\0';
    return pos;
}
