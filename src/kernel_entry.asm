[bits 32]
[extern kernel_main]

section .multiboot
    dd 0x1BADB002
    dd 0x00000000
    dd -(0x1BADB002 + 0x00000000)

section .text
global _start
_start:
    mov esp, stack_top
    call kernel_main
    jmp $

section .bss
stack_bottom:
    resb 16384
stack_top:
