# Kryptonix OS

> A 32-bit x86 operating system built entirely from scratch using C and x86 Assembly.
> No Linux. No Windows. No libraries. Just raw hardware and code.

```
██╗  ██╗██████╗ ██╗   ██╗██████╗ ████████╗ ██████╗ ███╗   ██╗██╗██╗  ██╗
██║ ██╔╝██╔══██╗╚██╗ ██╔╝██╔══██╗╚══██╔══╝██╔═══██╗████╗  ██║██║╚██╗██╔╝
█████╔╝ ██████╔╝ ╚████╔╝ ██████╔╝   ██║   ██║   ██║██╔██╗ ██║██║ ╚███╔╝
██╔═██╗ ██╔══██╗  ╚██╔╝  ██╔═══╝    ██║   ██║   ██║██║╚██╗██║██║ ██╔██╗
██║  ██╗██║  ██║   ██║   ██║        ██║   ╚██████╔╝██║ ╚████║██║██╔╝ ██╗
╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝   ╚═╝        ╚═╝    ╚═════╝ ╚═╝  ╚═══╝╚═╝╚═╝  ╚═╝
```

**Developer:** Sakshyam Kharel
**Version:** v0.1
**Year:** 2025
**Platform:** x86 32-bit bare metal

---

## What is Kryptonix?

Kryptonix is a hobby operating system written from absolute scratch — starting from a 512-byte bootloader and growing into a fully interactive OS with a graphical desktop, multi-user login, file system, games, and more.

It runs on real x86 hardware and boots from a USB drive. Every single line of code was written by hand — no operating system, no standard library, no shortcuts.

---

## Features

### Boot & Startup
- Custom GRUB bootloader with KRYPTONIX ASCII art
- Hacker-themed cyan and lime green color scheme
- Animated boot sequence with typing effect and loading bar
- Flash transition into login screen

### Login System
- Centered styled login box
- Multi-user support (sakshyam, root, guest)
- Password input shown as `****`
- Locks after 3 failed attempts

### Desktop GUI
- Fake graphical desktop using VGA box-drawing characters
- 8 app icons arranged in a 4×2 grid
- Arrow key navigation, Enter to launch
- Live clock ticking in the taskbar
- Shutdown confirmation dialog

### Terminal
- Full interactive shell
- Scrollable history (PageUp / PageDown)
- Command history (`history` command)
- Colorful prompt showing username

### File System
- In-memory file system (up to 16 files)
- `create`, `write`, `append`, `read`, `delete`, `ls`
- Files persist for the entire session

### Apps & Games
- **Snake** — classic snake game with WASD controls
- **Matrix** — falling green character screensaver
- **Clock** — live updating digital clock from hardware RTC
- **Sysinfo** — neofetch-style system information with color palette
- **Calculator** — supports `+`, `-`, `*`, `/`
- **Banner** — prints any text as large ASCII art
- **Color** — change terminal text color

---

## Shell Commands

| Command | Description |
|---------|-------------|
| `help` | Show all commands |
| `clear` | Clear the screen |
| `whoami` | Show current user |
| `sysinfo` | Neofetch-style system info |
| `history` | Show command history |
| `color <name>` | Change text color |
| `calc <expr>` | Calculator (e.g. `calc 5*3`) |
| `snake` | Play snake |
| `matrix` | Matrix screensaver |
| `clock` | Live digital clock |
| `banner <text>` | Big ASCII text |
| `ls` | List files |
| `create <name>` | Create a file |
| `write <f> <t>` | Write text to file |
| `append <f> <t>` | Append text to file |
| `read <name>` | Read a file |
| `delete <name>` | Delete a file |
| `logout` | Logout and return to login |
| `desktop` | Return to desktop |
| `shutdown` | Power off |
| `exit` | Halt the CPU |

---

## Project Structure

```
myos/
├── boot/
│   ├── grub/
│   │   ├── grub.cfg          # GRUB bootloader config + ASCII art
│   │   └── themes/           # GRUB theme files
│   └── kernel.elf            # Compiled kernel binary
├── src/
│   ├── kernel_entry.asm      # Assembly entry point (Multiboot header + stack)
│   ├── kernel.c              # Main kernel (VGA, shell, desktop, apps)
│   ├── keyboard.c            # PS/2 keyboard polling driver
│   └── fs.c                  # In-memory file system
├── include/
│   ├── terminal.h            # VGA terminal function declarations
│   ├── keyboard.h            # Keyboard driver declarations
│   ├── ports.h               # x86 port I/O (inb/outb)
│   ├── fs.h                  # File system declarations
│   └── gdt.h / idt.h        # (Reserved for future use)
├── linker.ld                 # Linker script (loads kernel at 0x100000)
├── build.sh                  # Full build script
└── myos.iso                  # Bootable ISO image
```

---

## How It Works

```
Power on
    ↓
BIOS/UEFI firmware
    ↓
GRUB bootloader (reads myos.iso)
    ↓
kernel_entry.asm (sets up stack, jumps to C)
    ↓
kernel_main() in kernel.c
    ↓
Animated boot sequence
    ↓
Login screen (multi-user auth)
    ↓
Desktop (icon-based fake GUI)
    ↓
Apps: Terminal, Snake, Matrix, Clock...
```

---

## Technical Details

| Component | Details |
|-----------|---------|
| Architecture | x86 32-bit protected mode |
| Bootloader | GRUB with Multiboot specification |
| Language | C (kernel) + NASM Assembly (entry/drivers) |
| Display | VGA text mode — direct writes to `0xB8000` |
| Keyboard | PS/2 polling via port `0x60` / `0x64` |
| Clock | Hardware RTC via ports `0x70` / `0x71` |
| File System | In-memory flat array (16 files max, 512 bytes each) |
| Memory | No MMU/paging — flat 32-bit address space |
| Shutdown | ACPI via port `0x604` |

---

## Building from Source

### Prerequisites (Arch Linux)

```bash
sudo pacman -S nasm qemu-system-x86 base-devel grub xorriso mtools
yay -S i686-elf-gcc i686-elf-binutils
```

### Build

```bash
cd ~/myos
chmod +x build.sh
./build.sh
```

### Run in QEMU

```bash
qemu-system-i386 -cdrom myos.iso -display gtk
```

### Flash to USB

```bash
# Find your USB drive
lsblk

# Flash (replace sdX with your USB drive!)
sudo dd if=myos.iso of=/dev/sdX bs=4M status=progress oflag=sync
```

---

## Default Users

| Username | Password |
|----------|----------|
| `sakshyam` | `kryptonix` |
| `root` | `toor` |
| `guest` | `guest` |

---

## Hardware Compatibility

Kryptonix boots on real x86 hardware. Tested on:

- Dell desktop with Intel Core i5-4570 (with Legacy/CSM mode enabled)
- QEMU x86 emulator (fully supported)

> **Note:** Disable Secure Boot and enable Legacy/CSM mode in your BIOS settings for best compatibility.
> On Dell: F2 → General → Boot Sequence → Legacy

---

## Roadmap / Future Plans

- [ ] Memory manager (physical memory allocator)
- [ ] Paging / virtual memory
- [ ] Mouse driver
- [ ] True graphical mode (VESA/VBE)
- [ ] Persistent file system (FAT12 on disk)
- [ ] Multitasking / process scheduler
- [ ] Sound driver (PC speaker beeps)
- [ ] More games (Tetris, Pong)

---

## Learning Resources Used

- [OSDev Wiki](https://wiki.osdev.org) — the bible of OS development
- *Writing a Simple OS from Scratch* — Nick Blundell (free PDF)
- [os-tutorial](https://github.com/cfenollosa/os-tutorial) — step by step GitHub guide
- Intel x86 Architecture Manual

---

## Fun Facts

- The bootloader started as just 512 bytes of Assembly
- Keyboard input didn't work for 2 hours (interrupt vs polling battle)
- Survived 47 "disk read errors" during early development
- The snake game runs with zero libraries — pure C and VGA memory writes
- The entire OS fits on a USB stick and boots in under 3 seconds
- No Linux. No Windows. Just you, the CPU, and 640KB of RAM

---

## License

This project is open source and free to learn from.
Built with ❤️ by Sakshyam Kharel — 2025

> *"Any sufficiently advanced tinkering is indistinguishable from an OS."*
