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
#define _NR_fork                13
#define _NR_get_pid				14

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

#define fork() \
({\
int __result;\
__asm__ volatile ("int $0x80" \
	: "=a" (__result)\
	: "a"(_NR_fork));\
__result; \
})

#define get_pid()\
({\
int __result;\
__asm__ volatile ("int $0x80" \
	: "=a" (__result)\
	: "a"(_NR_get_pid));\
__result; \
})

//#define open(pathname, flags) \
__asm__ ("int $0x80" \
	: \
	: "a"(_NR_open), "b"(pathname), "c"(flags) )

//#define syscall_char_printer(my_char) \
__asm__ ("int $0x80" \
	: \
	: "a"(my_char))

//#define syscall_tty_routine() \
__asm__ ("int $0x81" \
	: \
	: )

//#define syscall_write(buf, len) \
__asm__ ("int $0x82" \
	: \
	: "a"(buf), "b"(len))

//#define syscall_printx(buf) \
__asm__ ("int $0x83" \
	: \
	: "a"(buf))

//#define syscall_sendrec(func, src_dest, p_msg) \
__asm__ ("int $0x84" \
	: \
	: "a"(func), "b"(src_dest), "c"(p_msg))

//#define syscall_int_printer(my_int) \
__asm__ ("int $0x85" \
	: \
	: "a"(my_int))

//#define syscall_harddisk(p_msg) \
__asm__ ("int $0x86" \
	: \
	: "a"(p_msg))

//#define syscall_fs(step, p_msg) \
__asm__ ("int $0x87" \
	: \
	: "a"(step), "b"(p_msg))

