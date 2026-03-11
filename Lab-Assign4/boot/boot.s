!	boot.s
!
! It then loads the system at 0x10000, using BIOS interrupts. Thereafter
! it disables all interrupts, changes to protected mode, and calls the 
! start of system. System then must RE-initialize the protected mode in
! its own tables, and enable interrupts as needed.

BOOTSEG = 0x07c0
SYSSEG  = 0x1000			! system loaded at 0x10000 (65536).
SYSLEN  = 72				! sectors occupied.  max value is 72, why?


TASKSEG = 0x5000
TASKLEN = 18
TASKSEG1 = 0x6000
TASKLEN = 18

entry start
start:
	jmpi	go,#BOOTSEG
go:	mov	ax,cs
	mov	ds,ax
	mov	ss,ax
	mov	sp,#0x400		! arbitrary value >>512

! ok,now
load_system:
	mov	dx,#0x0000
	mov	cx,#0x0002
	mov	ax,#SYSSEG
	mov	es,ax
	xor	bx,bx
	mov	ax,#0x200+SYSLEN
	int 	0x13

	mov	dx,#0x0000
	mov	cx,#0x0202
	mov	ax,#0x1900
	mov	es,ax
	xor	bx,bx
	mov	ax,#0x200+56	! why cannot work when using a value > 56?
	int 	0x13

	mov	dx,#0x0100
	mov	cx,#0x0304
	mov	ax,#0x2000
	mov	es,ax
	xor	bx,bx
	mov	ax,#0x200+72
	int 	0x13

	mov	dx,#0x0100
	mov	cx,#0x0504
	mov	ax,#0x2900
	mov	es,ax
	xor	bx,bx
	mov	ax,#0x200+56
	int 	0x13


	!task0
	mov	dx,#0x0000
	mov	cx,#0x0801
	mov	ax,#TASKSEG
	mov	es,ax
	xor	bx,bx
	mov	ax,#0x200+TASKLEN
	int 	0x13

	!task1
	mov	dx,#0x0000
	mov	cx,#0x0802
	mov	ax,#TASKSEG1
	mov	es,ax
	xor	bx,bx
	mov	ax,#0x200+TASKLEN
	int 	0x13

	jnc	ok_load
die:	jmp	die

! now we want to move to protected mode ...
ok_load:
	cli			! no interrupts allowed !
	lgdt	gdt_48		


	mov	ax,#0x0001	! protected mode (PE) bit
	lmsw	ax		! This is it!
	jmpi	0x10000,8		! jmp offset 0x10000 of segment 8 (cs)

gdt:	.word	0,0,0,0		! dummy

	.word	0x07FF		! 8Mb - limit=2047 (2048*4096=8Mb)
	.word	0x0000		! base address=0x00000
	.word	0x9A00		! code read/exec
	.word	0x00C0		! granularity=4096, 386

	.word	0x07FF		! 8Mb - limit=2047 (2048*4096=8Mb)
	.word	0x0000		! base address=0x00000
	.word	0x9200		! data read/write
	.word	0x00C0		! granularity=4096, 386

	.word   0x0002
	.word   0x8000
	.word   0x920b
	.word   0x00c0

gdt_48: .word	0x7ff		! gdt limit=2048, 256 GDT entries
	.word	0x7c00+gdt,0	! gdt base = 07xxx
.org 510
	.word   0xAA55

