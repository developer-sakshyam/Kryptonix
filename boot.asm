[org 0x7C00]

mov bp, 0x9000
mov sp, bp

mov [boot_drive], dl

call load_kernel

jmp 0x1000

load_kernel:
    ; reset disk
    mov ah, 0x00
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    ; set ES:BX = 0x0000:0x1000 (where to load kernel)
    mov ax, 0x0000
    mov es, ax
    mov bx, 0x1000

    ; read sectors
    mov ah, 0x02
    mov al, 15
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [boot_drive]
    int 0x13
    jc disk_error
    ret

disk_error:
    mov si, error_msg
    call print_string
    jmp $

print_string:
    mov ah, 0x0E
.loop:
    lodsb
    cmp al, 0
    je .done
    int 0x10
    jmp .loop
.done:
    ret

boot_drive db 0
error_msg db 'Disk read error!', 0

times 510 - ($ - $$) db 0
dw 0xAA55
