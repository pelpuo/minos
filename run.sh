#!/bin/bash
set -xue

OBJCOPY=/usr/local/bin/llvm-objcopy

# QEMU file path
QEMU=qemu-system-riscv64

# Path to clang and compiler flags
CC=clang  # Ubuntu users: use CC=clang
CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=riscv64-unknown-elf -fno-stack-protector -ffreestanding -nostdlib -mno-relax"

# Build the shell (application)
$CC $CFLAGS -Wl,-Tuser.ld -Wl,-Map=shell.map -o shell.elf shell.c user.c common.c
$OBJCOPY --set-section-flags .bss=alloc,contents -O binary shell.elf shell.bin
$OBJCOPY -Ibinary -Oelf64-littleriscv shell.bin shell.bin.o

# Build the kernel
$CC $CFLAGS -mcmodel=medany -fuse-ld=lld -Wl,-Tkernel.ld -Wl,-Map=kernel.map -o kernel.elf \
    kernel.c common.c shell.bin.o

# Check symbols in the ELF (useful for debugging addresses)
riscv64-unknown-elf-objdump -t kernel.elf | grep __

# Start QEMU
$QEMU -machine virt -bios default -nographic -serial mon:stdio --no-reboot \
    -kernel kernel.elf