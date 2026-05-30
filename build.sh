#!/bin/bash
set -e

cd ~/myos

echo "==> Cleaning old build..."
rm -f src/*.o boot/kernel.elf myos.iso

echo "==> Compiling kernel..."
nasm -f elf32 src/kernel_entry.asm -o src/kernel_entry.o
i686-elf-gcc -ffreestanding -m32 -c src/kernel.c   -o src/kernel.o   -Iinclude
i686-elf-gcc -ffreestanding -m32 -c src/keyboard.c -o src/keyboard.o -Iinclude
i686-elf-gcc -ffreestanding -m32 -c src/fs.c       -o src/fs.o       -Iinclude

echo "==> Linking..."
i686-elf-ld -T linker.ld -o boot/kernel.elf \
    src/kernel_entry.o \
    src/kernel.o \
    src/keyboard.o \
    src/fs.o

echo "==> Building ISO with UEFI + BIOS support..."
rm -f myos.iso
grub-mkrescue -o myos.iso . \
    --modules="part_gpt part_msdos fat iso9660 normal multiboot multiboot2 echo test video_bochs video_cirrus"

echo "==> Done! myos.iso is ready."
