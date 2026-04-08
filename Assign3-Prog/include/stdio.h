/* EXTERN */
#define	EXTERN	extern	/* EXTERN is defined as extern except in global.c */

/* string */
//#define	STR_DEFAULT_LEN	1024

#define	O_CREAT		1
#define	O_RDWR		2

#define READ		1
#define WRITE		2

#define SEEK_SET	1
#define SEEK_CUR	2
#define SEEK_END	3

#define	MAX_PATH	128

/*--------*/
/* system calls */
/*--------*/
#define _NR_write_char_routine 	0
#define _NR_tty_routine			1
#define _NR_tty_write			2
#define _NR_printx				3
#define _NR_sendrec				4
#define _NR_write_int_routine	5
#define _NR_hd_open				6
#define _NR_init_fs				7
#define _NR_open 				8
#define _NR_close 				9
#define _NR_read 				10
#define _NR_write 				11
#define _NR_print_task_paging 	12
#define _NR_brk					13

#define char_printer(my_char) \
__asm__ ("int $0x80" \
	: \
	: "a" (_NR_write_char_routine), "b"(my_char))

#define tty_routine() \
__asm__ ("int $0x80" \
	: \
	: "a" (_NR_tty_routine))

#define tty_write(buf, len) \
__asm__ ("int $0x80" \
	: \
	: "a"(_NR_tty_write), "b"(buf), "c"(len))

#define printx(buf) \
__asm__ ("int $0x80" \
	: \
	: "a"(_NR_printx), "b"(buf))

#define sendrec(func, src_dest, p_msg) \
__asm__ ("int $0x80" \
	: \
	: "a"(_NR_sendrec), "b"(func), "c"(src_dest), "d"(p_msg))

#define int_printer(my_int) \
__asm__ ("int $0x80" \
	: \
	: "a"(_NR_write_int_routine), "b"(my_int))

#define hd_open(dev) \
__asm__ ("int $0x80" \
	: \
	: "a"(_NR_hd_open), "b"(dev))

#define init_fs() \
__asm__ ("int $0x80" \
	: \
	: "a"(_NR_init_fs) )

#define print_task_paging(pid) \
__asm__ ("int $0x80" \
	: \
	: "a"(_NR_print_task_paging), "b"(pid) )

#define print_task_paging(pid) \
__asm__ ("int $0x80" \
	: \
	: "a"(_NR_print_task_paging), "b"(pid) )

#define brk(addr) \
({ \
	void* __res; \
	__asm__ volatile ("int $0x80" \
		: "=a" (__res) \
		: "a" (_NR_brk), "b" (addr)); \
	__res; \
})
