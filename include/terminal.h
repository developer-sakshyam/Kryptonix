#ifndef TERMINAL_H
#define TERMINAL_H

void terminal_putchar(char c);
void terminal_backspace();
void print(const char *str);
void print_hex(unsigned int num);
void print_int(int num);
void clear_screen();
void render_screen();
void scroll_up();
void scroll_down();

#endif
