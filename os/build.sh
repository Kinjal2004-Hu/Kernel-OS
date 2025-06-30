#!/bin/bash

echo "Building Enhanced MyOS..."

# Clean previous builds
rm -f *.bin *.o os.img

echo "Step 1: Building bootloader..."
nasm -f bin boot.asm -o boot.bin
if [ $? -ne 0 ]; then
    echo "Bootloader build failed!"
    exit 1
fi

echo "Step 2: Building kernel entry..."
nasm -f elf32 kernel_entry.asm -o kernel_entry.o
if [ $? -ne 0 ]; then
    echo "Kernel entry build failed!"
    exit 1
fi

echo "Step 3: Compiling enhanced C kernel..."
gcc -m32 -c kernel.c -o kernel.o \
    -ffreestanding \
    -fno-stack-protector \
    -fno-pic \
    -nostdlib \
    -Wall \
    -Wextra \
    -O2

if [ $? -ne 0 ]; then
    echo "Kernel compilation failed!"
    exit 1
fi

echo "Step 4: Linking kernel (ELF)..."
ld -m elf_i386 -T linker.ld -nostdlib \
   kernel_entry.o kernel.o -o kernel.elf
if [ $? -ne 0 ]; then
    echo "Kernel linking failed!"
    exit 1
fi

# optional: create flat binary for os.img
objcopy -O binary kernel.elf kernel.bin

echo "Step 5: Checking kernel size (flat image)..."
KERNEL_SIZE=$(wc -c < kernel.bin)
MAX_SIZE=$((20 * 512))  # 20 sectors max
echo "Kernel size: $KERNEL_SIZE bytes (max: $MAX_SIZE bytes)"

echo "Step 6: Creating floppy image (os.img)..."
dd if=/dev/zero of=os.img bs=512 count=2880 2>/dev/null
dd if=boot.bin of=os.img bs=512 count=1 conv=notrunc 2>/dev/null
dd if=kernel.bin of=os.img bs=512 seek=1 conv=notrunc 2>/dev/null

echo "Step 7: Creating bootable ISO with GRUB..."
ISO_DIR=iso
mkdir -p "$ISO_DIR/boot/grub"

cp kernel.elf "$ISO_DIR/boot/"
cp hello.txt "$ISO_DIR/boot/"
cp basic.txt "$ISO_DIR/boot/"
cp disk.img "$ISO_DIR/boot/"
cp grub/grub.cfg "$ISO_DIR/boot/grub/"

echo "Formatting disk.img as FAT-12..."
mformat -i disk.img ::

echo "Copying files into disk.img..."
mcopy -i disk.img hello.txt ::

grub-mkrescue -o myos.iso "$ISO_DIR" > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "GRUB ISO creation failed!"
    exit 1
fi

echo "Bootable ISO created: myos.iso"
echo "Test it using:"
echo "  qemu-system-i386 -cdrom myos.iso"
qemu-system-i386 -cdrom myos.iso