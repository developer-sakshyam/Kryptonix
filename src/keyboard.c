#include "../include/keyboard.h"
#include "../include/ports.h"
#include "../include/terminal.h"

static char keys[58] = {
    0,  0,  '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,  '\\','z','x','c','v','b','n','m',',','.','/',0,
    '*', 0,  ' '
};

static char input_buffer[256];
static int  buf_pos = 0;
static void (*enter_callback)(char *) = 0;

void keyboard_init(void (*callback)(char *)) {
    enter_callback = callback;
}

void keyboard_poll() {
    unsigned char status = port_byte_in(0x64);
    if (!(status & 0x01)) return;

    unsigned char scancode = port_byte_in(0x60);

    // ignore key releases (bit 7 set)
    if (scancode & 0x80) return;

    // Page Up = 0x49, Page Down = 0x51
    if (scancode == 0x49) { scroll_up();   return; }
    if (scancode == 0x51) { scroll_down(); return; }

    if (scancode >= 58) return;

    char c = keys[scancode];
    if (c == 0) return;

    if (c == '\b') {
        if (buf_pos > 0) {
            buf_pos--;
            input_buffer[buf_pos] = 0;
            terminal_backspace();
        }
    } else if (c == '\n') {
        input_buffer[buf_pos] = 0;
        terminal_putchar('\n');
        if (enter_callback)
            enter_callback(input_buffer);
        buf_pos = 0;
    } else {
        if (buf_pos < 255) {
            input_buffer[buf_pos++] = c;
            terminal_putchar(c);
        }
    }
}
