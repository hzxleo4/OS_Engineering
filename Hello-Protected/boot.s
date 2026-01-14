!	boot.s
!
! It then loads the system at 0x10000, using BIOS interrupts. Thereafter
! it disables all interrupts, changes to protected mode, and calls the 

BOOTSEG = 0x07c0
SYSSEG  = 0x2000			! system loaded at 0x10000 (65536).
SYSLEN  = 1				! sectors occupied.

entry start
start:
	jmpi	go,#BOOTSEG
go:	mov	ax,cs
	mov	ds,ax
	mov	es,ax

	# mov ah,#0x0f
	# int 0x10

	mov	ah,#0x03			! Read cursor pos
	xor	bh,bh				! Character display attribute: page 0
	int	0x10 				! The BIOS interrupt call 0x10, function 0x03
	
	mov	cx,#24				! Number of characters
	mov	bx,#0x0006			! Character display attribute: page 0, attribute 7 (normal)
	mov	bp,#msg1			! Point to a string
	mov	ax,#0x1301			! Write string, move cursor
	int	0x10 				! The BIOS interrupt call 0x10, function 0x13, subfunc 01

! ok, now
load_system:
	mov	dx,#0x0000
	mov	cx,#0x0002
	mov	ax,#SYSSEG
	mov	es,ax
	xor	bx,bx
	mov	ax,#0x200+SYSLEN
	int 	0x13
	jnc	ok_load
die:	jmp	die

! now we want to move to protected mode ...
ok_load:
	cli			! no interrupts allowed
	lgdt	gdt_48		! load gdt with whatever appropriate

	mov	ax,#0x0001	! protected mode (PE) bit
	lmsw	ax		! This is it!
	jmpi	0,8		! jmp offset 0 of segment 8 (cs)

msg1:
	.byte 13,10				! carriage return and line feed (13, 10) characters
	.ascii "Loading: Hello ..."
	.byte 13,10,13,10

gdt:	.word	0,0,0,0		! dummy

	.word	0x07FF		! 0x08 8Mb - limit=2047 (2048*4096=8Mb)
	.word	0x0000		! base address=0x10000
	.word	0x9A02		! code read/exec
	.word	0x00C0		! granularity=4096, 386

	.word	0x07FF		! 0x10 8Mb - limit=2047 (2048*4096=8Mb)
	.word	0x0000		! base address=0x10000
	.word	0x9202		! data read/write
	.word	0x00C0		! granularity=4096, 386

	.word   0x0002		! screen 0x18 - for display
	.word   0x2900
	.word   0x9200
	.word   0x00c0

gdt_48: .word	0x7ff		! gdt limit=2048, 256 GDT entries
	.word	0x7c00+gdt,0	! gdt base = 07xxx
.org 510
	.word   0xAA55

