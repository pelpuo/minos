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

# ← monitor: build the monitor exactly the same way
$CC $CFLAGS -Wl,-Tuser.ld -Wl,-Map=monitor.map \
    -o monitor.elf monitor.c user.c common.c
$OBJCOPY --set-section-flags .bss=alloc,contents -O binary monitor.elf monitor.bin
$OBJCOPY -Ibinary -Oelf64-littleriscv monitor.bin monitor.bin.o

xxd -i monitor.bin > monitor_bin.h

# ← myapp: build the user app exactly the same way
$CC $CFLAGS -Wl,-Tuser.ld -Wl,-Map=myapp.map \
    -o myapp.elf myapp.c user.c common.c
$OBJCOPY --set-section-flags .bss=alloc,contents -O binary myapp.elf myapp.bin
$OBJCOPY -Ibinary -Oelf64-littleriscv myapp.bin myapp.bin.o

# generate a C array and length constant
xxd -i myapp.bin > myapp_bin.h

# ← myapp: build the user app exactly the same way
$CC $CFLAGS -Wl,-Tuser.ld -Wl,-Map=mal.map \
    -o mal.elf mal.c user.c common.c
$OBJCOPY --set-section-flags .bss=alloc,contents -O binary mal.elf mal.bin
$OBJCOPY -Ibinary -Oelf64-littleriscv mal.bin mal.bin.o

# generate a C array and length constant
xxd -i mal.bin > mal_bin.h


# Build the kernel
$CC $CFLAGS -mcmodel=medany -fuse-ld=lld -Wl,-Tkernel.ld -Wl,-Map=kernel.map -o kernel.elf \
    kernel.c common.c monitor.bin.o shell.bin.o

# Check symbols in the ELF (useful for debugging addresses)
riscv64-unknown-elf-objdump -t kernel.elf | grep __

(cd disk && tar --format=ustar --owner=0 --group=0 -cf ../disk.tar hello.txt meow.txt)

# $QEMU -machine virt -bios default -nographic -monitor none -serial stdio --no-reboot \
$QEMU -machine virt -bios default -nographic -serial mon:stdio --no-reboot \
    -d unimp,guest_errors,int,cpu_reset -D qemu.log \
    -drive id=drive0,file=disk.tar,format=raw,if=none \
    -device virtio-blk-device,drive=drive0,bus=virtio-mmio-bus.0 \
    -kernel kernel.elf