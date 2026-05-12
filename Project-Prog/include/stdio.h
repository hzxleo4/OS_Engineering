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
/* 库函数 */
/*--------*/

/* lib/open.c */
//PUBLIC	void	open		(const char *pathname, int flags);

/* lib/close.c */
//PUBLIC	int	close		(int fd);



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
#define _NR_delete 				12
#define _NR_hd_routine			13
#define _NR_fsync				14
#define _NR_mkdir				15
#define _NR_print_dir_tree		16

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


#define open(pathname, flags)\
({\
int __result;\
__asm__ volatile ("int $0x80" \
	: "=a" (__result)\
	: "a"(_NR_open), "b"(pathname), "c"(flags)\
	: "memory", "cc");\
__result; \
})

#define close(fd)\
({\
int __result;\
__asm__ volatile ("int $0x80" \
	: "=a" (__result)\
	: "a"(_NR_close), "b"(fd)\
	: "memory", "cc");\
__result; \
})

#define read(fd, buf, count)\
({\
int __result;\
__asm__ volatile ("int $0x80" \
	: "=a" (__result)\
	: "a"(_NR_read), "b"(fd), "c"(buf), "d"(count)\
	: "memory", "cc");\
__result; \
})

#define write(fd, buf, count)\
({\
int __result;\
__asm__ volatile ("int $0x80" \
	: "=a" (__result)\
	: "a"(_NR_write), "b"(fd), "c"(buf), "d"(count)\
	: "memory", "cc");\
__result; \
})

#define delete(pathname)\
({\
int __result;\
__asm__ volatile ("int $0x80" \
	: "=a" (__result)\
	: "a"(_NR_delete), "b"(pathname)\
	: "memory", "cc");\
__result; \
})

#define hd_routine() \
__asm__ ("int $0x80" \
	: \
	: "a" (_NR_hd_routine))

#define fsync() \
__asm__ ("int $0x80" \
	: \
	: "a" (_NR_fsync))

#define mkdir(pathname)\
({\
int __result;\
__asm__ volatile ("int $0x80" \
	: "=a" (__result)\
	: "a"(_NR_mkdir), "b"(pathname)\
	: "memory", "cc");\
__result; \
})

#define print_dir_tree() \
__asm__ ("int $0x80" \
	: \
	: "a" (_NR_print_dir_tree))
