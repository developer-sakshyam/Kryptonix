#ifndef KEYBOARD_H
#define KEYBOARD_H

void keyboard_init(void (*callback)(char *));
void keyboard_poll();

#endif
