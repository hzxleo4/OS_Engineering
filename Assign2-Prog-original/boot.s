!	boot.s
!
! It then loads the system at 0x10000, using BIOS interrupts. Thereafter
! it disables all interrupts, changes to protected mode, and calls the 
! start of system. System then must RE-initialize the protected mode in
! it's own tables, and enable interrupts as needed.

BOOTSEG = 0x07c0
SYSSEG  = 0x1000			! system loaded at 0x10000 (65536).
SYSLEN  = 128				! sectors occupied. floopy disk -- Max: 2880. Max sector number - 128 (64K - 16-bit addr. bound) 

TASKSEG = 0x5000
TASKLEN = 1
TASKNUM = 2				! Starting from 0x5000:0000, each task is loaded sequetiallly with 0x10000 length


entry start
start:
	jmpi	go,#BOOTSEG
go:	mov	ax,cs
	mov	ds,ax
	mov	ss,ax
	mov	sp,#0x400		! arbitrary value >>512

! ok,now
load_system:
    	mov ax, #SYSSEG        ; ES = 0x1000, system load base
    	mov es, ax
    	xor bx, bx            ; offset = 0
    	mov si, #SYSLEN        ; total sectors to read
    	mov cx, #2             ; start at sector 2, track 0; cx: Bits 0-5 for Sector No. (1-18 for Floopy); 
			       ;   Bit 9-15 for Track No. (0-79); Bits 6-8 for highest bits of cylinder 
    	mov dx, #0             ; head=0, drive=0

; ----------------------------
; First track special case
; ----------------------------
first_track:
    	cmp si, #17
    	jbe read_last_first   ; if ≤17 sectors left, read them
    	mov ax, #0x200+17      ; BIOS int 13h, AH=2, AL=17 sectors
    	int 0x13
    	jc die
    	add bx, #17*512        ; advance buffer
    	sub si, #17            ; reduce remaining count

    	; move to next head/track
    	mov cx, #1             ; reset to sector 1
    	inc dh                 ; head=1
    	jmp normal_loop

read_last_first:
	cmp si, #0
	jbe task_load
    	mov ax, #0x200         ; function 2: read sectors
    	add ax, si            ; AL = remaining sectors
    	int 0x13
    	jc die
    	jmp task_load

; ----------------------------
; Normal loop for subsequent tracks
; ----------------------------
normal_loop:
    	cmp si, #18
    	jbe read_last         ; ≤18 → final read
    	mov ax, #0x200+18      ; read 18 sectors
    	int 0x13
    	jc die
    	add bx, #18*512
    	sub si, #18
	mov cl, #1          ; reset sector to 1
	inc dh             ; next head
	cmp dh, #2
	jb normal_loop     ; if head < 2, continue
	mov dh, #0          ; reset head
	inc ch             ; next cylinder
	jmp normal_loop

read_last:
    	mov ax, #0x200         ; function 2: read sectors
    	add ax, si            ; AL = remaining sectors
    	int 0x13
    	jc die

task_load:
	mov 	si,#TASKNUM	; the number of tasks
	cmp	si,#0
	jbe	ok_load
	mov	cx,#0x0800	; Cylinder 08, 
	mov	dx,#0x0000	; Head 0
	mov	ax,#TASKSEG	; 
	mov	es,ax
task_load_next:
	inc	cl		;Start from sector 1, each time load one sector
	xor	bx,bx
	mov	ax,#0x200+TASKLEN
	int 	0x13
    	jc die

	sub	si,#1
	cmp 	si,#0
	jbe	ok_load
	mov	ax, es
	add     ax, #0x1000
	mov	es, ax		;Leave space 0x10000 for next task
	jmp 	task_load_next

die:	jmp	die

! now we want to move to protected mode ...
ok_load:
	cli			! no interrupts allowed !
	lgdt	gdt_48		

! well, that went ok, I hope. Now we have to reprogram the interrupts :-(
! we put them right after the intel-reserved hardware interrupts, at
! int 0x20-0x2F. There they won't mess up anything. Sadly IBM really
! messed this up with the original PC, and they haven't been able to
! rectify it afterwards. Thus the bios puts interrupts at 0x08-0x0f,
! which is used for the internal hardware interrupts as well. We just
! have to reprogram the 8259's, and it isn't fun.

! these operation is moved to c code
	# mov	al,#0x11		! initialization sequence
	# out	#0x20,al		! send it to 8259A-1
	# out	#0xA0,al		! and to 8259A-2
	# mov	al,#0x20		! start of hardware int's (0x20)
	# out	#0x21,al
	# mov	al,#0x28		! start of hardware int's 2 (0x28)
	# out	#0xA1,al
	# mov	al,#0x04		! 8259-1 is master
	# out	#0x21,al
	# mov	al,#0x02		! 8259-2 is slave
	# out	#0xA1,al
	# mov	al,#0x01		! 8086 mode for both
	# out	#0x21,al
	# out	#0xA1,al
	# mov	al,#0xFF		! mask off all interrupts for now
	# out	#0x21,al
	# out	#0xA1,al

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

