#include "../include/idt.h"
#include "../include/ports.h"

struct idt_entry idt[256];
struct idt_ptr idtp;

void idt_set_gate(unsigned char num, unsigned long base, unsigned short sel, unsigned char flags) {
    idt[num].base_lo = (base & 0xFFFF);
    idt[num].base_hi = (base >> 16) & 0xFFFF;
    idt[num].sel     = sel;
    idt[num].always0 = 0;
    idt[num].flags   = flags | 0x60;
}

void idt_init() {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base  = (unsigned int) &idt;

    // zero out entire IDT
    unsigned char *ptr = (unsigned char*) &idt;
    int i;
    for (i = 0; i < (int)(sizeof(struct idt_entry) * 256); i++)
        ptr[i] = 0;

    idt_load((unsigned int) &idtp);
}
