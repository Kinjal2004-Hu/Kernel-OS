
# 🧠 MyOS – A Simple Modular Operating System (C + GRUB + QEMU)

**MyOS** is a minimalist 32-bit x86 operating system built from scratch using **C**. This modular OS is Multiboot-compliant, loads via **GRUB**, and runs inside **QEMU** as a bootable ISO. It demonstrates key operating system concepts including text-mode video output, modular kernel design, and keyboard input.

---

## ✨ Features

- 🔧 **GRUB-based bootloader** (Multiboot-compliant)
- 🧱 **Modular Kernel Design** (separate modules for screen, keyboard, shell)
- 📺 **VGA Text Mode** (80x25) interface using direct video memory
- ⌨️ **Keyboard Input Handling** with an input buffer
- 🐚 **Shell Interface** with basic command parsing
- 💿 Bootable ISO image creation
- 🧪 Testable using QEMU emulator

---

## 📦 Requirements

Install the following packages (tested on Debian/Ubuntu):

```bash
sudo apt update
sudo apt install build-essential xorriso grub-pc-bin qemu-system-i386
```

> Optional (for cross-compilation):
```bash
sudo apt install gcc-multilib
```

---

## 📁 Project Structure

```
MyOS/
├── kernel/                # Modular C source files (kernel.c, screen.c, etc.)
├── iso/
│   └── boot/
│       ├── grub/
│       │   └── grub.cfg
│       └── kernel.bin
├── linker.ld              # Linker script for GRUB
├── build.sh               # Automated build script
├── myos.iso               # Output ISO (after build)
└── README.md              # This documentation
```

---

## 🔧 Step-by-Step Build Instructions

### 1. Clone and Navigate

```bash
git clone https://github.com/yourusername/MyOS.git
cd MyOS
```

### 2. Build the Kernel and ISO

```bash
chmod +x build.sh
./build.sh
```

The script will:
- Compile the kernel from `kernel.c` (and others)
- Link using `linker.ld`
- Create the ISO structure under `iso/boot`
- Generate a bootable ISO image using `grub-mkrescue`

### 3. Run in QEMU

```bash
qemu-system-i386 -cdrom myos.iso
```

You should see a welcome message and shell prompt.

---

## 📜 GRUB Configuration

Located at: `iso/boot/grub/grub.cfg`

```cfg
set timeout=0
set default=0

menuentry "MyOS" {
    multiboot /boot/kernel.bin
    boot
}
```

Ensure the path `/boot/kernel.bin` matches the actual location of the kernel file.

---

## 📄 Linker Script (`linker.ld`)

Ensures the kernel loads at the expected 1MB memory address:

```ld
ENTRY(kmain)
SECTIONS {
    . = 1M;
    .text : { *(.text) }
    .data : { *(.data) }
    .bss  : { *(.bss) }
}
```

---

## 🛠️ Build Script (`build.sh`)

```bash
#!/bin/bash

echo "Building MyOS..."

mkdir -p iso/boot/grub

i386-elf-gcc -ffreestanding -c kernel.c -o kernel.o
i386-elf-ld -T linker.ld -o kernel.bin kernel.o

cp kernel.bin iso/boot/
cp grub.cfg iso/boot/grub/

grub-mkrescue -o myos.iso iso
```

---

## 🚨 Common Errors & Fixes

| Error                                              | Cause & Solution                                             |
|---------------------------------------------------|--------------------------------------------------------------|
| `invalid file name 'iso/boot/kernel.bin'`         | `kernel.bin` not found at correct location                   |
| `you need to load the kernel first`               | GRUB config wrong path or missing multiboot header           |
| Blank screen on boot                              | No text written to VGA buffer or incorrect memory offset     |
| `grub-mkrescue: command not found`                | Install `grub-pc-bin` and `xorriso`                          |

---

## 🔮 Planned Features

- FAT-12 file system integration
- Shell history & command parsing enhancements
- Basic GUI using VESA BIOS Extensions
- Modular compilation system for kernel extensions

---

## 📖 References

- [Multiboot Specification](https://www.gnu.org/software/grub/manual/multiboot/multiboot.html)
- [GRUB Manual](https://www.gnu.org/software/grub/manual/)
- Inspired by [cfenollosa/os-tutorial](https://github.com/cfenollosa/os-tutorial)

---

## 🧾 License

This project is licensed under the [MIT License](LICENSE).

---

## 🤝 Contributing

Pull requests are welcome! For major changes, please open an issue first to discuss what you would like to change.

---

## 👨‍💻 Author

Developed by **Kinjal Ojha**  
[GitHub](https://github.com/Kinjal2004-Hu) | [LinkedIn]((https://www.linkedin.com/in/kinjal-ojha-1288b32b8/))

---
