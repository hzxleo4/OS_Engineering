# Makefile for the simple example kernel.
AS86	=as86 -0 -a
LD86	=ld86 -0
AS	=as
LD	=ld
ASM = nasm

LDFLAGS =-m elf_i386 -Ttext 0xC0010000 -e startup_32 -s -x -M  
CFLAGS = -I include/ -c -fno-pic -fno-stack-protector -fno-builtin
ASMFLAGS = -I include/ -f elf32

all:	clean Image

Image: boot/boot kernel/system kernel/mytask kernel/mytask1
	dd bs=32 if=boot/boot of=Image skip=1
	objcopy -O binary kernel/system boot/head
	cat boot/head >> Image
	objcopy -O binary kernel/mytask kernel/task0
	dd bs=512 if=kernel/task0 of=Image seek=288
	objcopy -O binary kernel/mytask1 kernel/task1
	dd bs=512 if=kernel/task1 of=Image seek=289


disk: Image
	dd bs=8192 if=Image of=/dev/fd0
	sync;sync;sync

kernel/mytask.o: kernel/mytask.c include/proto.h
	gcc $(CFLAGS) -o kernel/mytask.o kernel/mytask.c  # -fno-pic is used to reduce the size of the object file; remove it is ok

kernel/mytask: kernel/mytask.o
	$(LD) -m elf_i386 -Ttext 0 -e main -s -x -M kernel/mytask.o -o kernel/mytask > kernel/mytask.map

kernel/mytask1.o: kernel/mytask1.c include/proto.h
	gcc $(CFLAGS) -o kernel/mytask1.o kernel/mytask1.c  # -fno-pic is used to reduce the size of the object file; remove it is ok	

kernel/mytask1: kernel/mytask1.o
	$(LD) -m elf_i386 -Ttext 0 -e main -s -x -M kernel/mytask1.o -o kernel/mytask1 > kernel/mytask1.map


boot/head.o: boot/head.s
	$(AS) -o boot/head.o boot/head.s

kernel/main.o: kernel/main.c
	gcc $(CFLAGS) -o kernel/main.o kernel/main.c

kernel/kernel.o: kernel/kernel.c include/kernel.h include/type.h
	gcc $(CFLAGS) -o kernel/kernel.o kernel/kernel.c

kernel/page.o: kernel/page.c include/page.h
	gcc $(CFLAGS) -o kernel/page.o kernel/page.c

kernel/task.o: kernel/task.c include/task.h
	gcc $(CFLAGS) -o kernel/task.o kernel/task.c

kernel/keyboard.o: kernel/keyboard.c include/keyboard.h
	gcc $(CFLAGS) -o kernel/keyboard.o kernel/keyboard.c

kernel/tty.o: kernel/tty.c include/tty.h
	gcc $(CFLAGS) -o kernel/tty.o kernel/tty.c

kernel/console.o: kernel/console.c include/console.h
	gcc $(CFLAGS) -o kernel/console.o kernel/console.c

kernel/global.o: kernel/global.c include/global.h
	gcc $(CFLAGS) -o kernel/global.o kernel/global.c

lib/klibs.o: lib/klibs.s 
	$(AS) -o lib/klibs.o lib/klibs.s

lib/klib.o: lib/klib.c include/klib.h include/proto.h
	gcc $(CFLAGS) -o lib/klib.o lib/klib.c


kernel/system:	boot/head.o kernel/main.o kernel/kernel.o kernel/page.o kernel/task.o kernel/keyboard.o kernel/global.o lib/klibs.o lib/klib.o kernel/tty.o kernel/console.o
	$(LD) $(LDFLAGS) boot/head.o kernel/main.o kernel/kernel.o kernel/page.o kernel/task.o kernel/keyboard.o kernel/global.o lib/klibs.o lib/klib.o kernel/tty.o kernel/console.o -o kernel/system > kernel/System.map

boot/boot:	boot/boot.s
	$(AS86) -o boot/boot.o boot/boot.s
	$(LD86) -s -o boot/boot boot/boot.o



clean:
	rm -f Image kernel/System.map boot/boot boot/head kernel/*.o lib/*.o boot/*.o kernel/system kernel/task0 kernel/mytask kernel/mytask.map kernel/task1 kernel/mytask1 kernel/mytask1.map
