#include "../include/idt.h"
#include "../include/irq.h"
#include "../include/ports.h"

void *irq_routines[16] = { 0 };

void irq_install_handler(int irq, void (*handler)(struct regs *r)) {
    irq_routines[irq] = handler;
}

void irq_handler(struct regs *r) {
    void (*handler)(struct regs *r);
    handler = irq_routines[r->int_no - 32];
    if (handler)
        handler(r);

    if (r->int_no >= 40)
        port_byte_out(0xA0, 0x20);
    port_byte_out(0x20, 0x20);
}

void irq_init() {
    // remap PIC properly
    port_byte_out(0x20, 0x11);
    port_byte_out(0xA0, 0x11);
    port_byte_out(0x21, 0x20);
    port_byte_out(0xA1, 0x28);
    port_byte_out(0x21, 0x04);
    port_byte_out(0xA1, 0x02);
    port_byte_out(0x21, 0x01);
    port_byte_out(0xA1, 0x01);

    // unmask all IRQs
    port_byte_out(0x21, 0x00);
    port_byte_out(0xA1, 0x00);

    // install IRQ gates
    idt_set_gate(32, (unsigned long)irq0,  0x08, 0x8E);
    idt_set_gate(33, (unsigned long)irq1,  0x08, 0x8E);
    idt_set_gate(34, (unsigned long)irq2,  0x08, 0x8E);
    idt_set_gate(35, (unsigned long)irq3,  0x08, 0x8E);
    idt_set_gate(36, (unsigned long)irq4,  0x08, 0x8E);
    idt_set_gate(37, (unsigned long)irq5,  0x08, 0x8E);
    idt_set_gate(38, (unsigned long)irq6,  0x08, 0x8E);
    idt_set_gate(39, (unsigned long)irq7,  0x08, 0x8E);
    idt_set_gate(40, (unsigned long)irq8,  0x08, 0x8E);
    idt_set_gate(41, (unsigned long)irq9,  0x08, 0x8E);
    idt_set_gate(42, (unsigned long)irq10, 0x08, 0x8E);
    idt_set_gate(43, (unsigned long)irq11, 0x08, 0x8E);
    idt_set_gate(44, (unsigned long)irq12, 0x08, 0x8E);
    idt_set_gate(45, (unsigned long)irq13, 0x08, 0x8E);
    idt_set_gate(46, (unsigned long)irq14, 0x08, 0x8E);
    idt_set_gate(47, (unsigned long)irq15, 0x08, 0x8E);

    // enable interrupts
    asm volatile("sti");
}
