#ifndef IDT_H
#define IDT_H

struct idt_entry {
    unsigned short base_lo;
    unsigned short sel;
    unsigned char  always0;
    unsigned char  flags;
    unsigned short base_hi;
} __attribute__((packed));

struct idt_ptr {
    unsigned short limit;
    unsigned int   base;
} __attribute__((packed));

void idt_init();
void idt_set_gate(unsigned char, unsigned long, unsigned short, unsigned char);
void idt_load(unsigned int);

#endif
