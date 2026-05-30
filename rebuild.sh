#!/bin/bash
# Quick rebuild script - cleans everything and rebuilds fresh

cd ~/myos

echo "Removing old files..."
rm -f src/*.o boot/kernel.elf myos.iso

echo "Rebuilding..."
bash build.sh

echo "Done! New myos.iso ready."
echo ""
echo "Run with:"
echo "qemu-system-i386 -cdrom myos.iso -m 512 -nic user,model=e1000"
