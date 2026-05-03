/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            proto.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#include "const.h"
#include "task.h"
#include "page.h"


/* klib.asm */
PUBLIC void	out_byte(u16 port, u8 value);
PUBLIC u8	in_byte(u16 port);
PUBLIC void	disp_str(char * info);
PUBLIC void disable_int();
PUBLIC void enable_int();
PUBLIC void port_read(u16 port, void* buf, int n);
PUBLIC void port_write(u16 port, void* buf, int n);
void enable_irq(int irq);

/* keyboard.c */
PUBLIC void init_keyboard();
PUBLIC void keyboard_read();

/* main.c */
//PUBLIC void put_irq_handler(int irq, irq_handler handler);
PUBLIC void spurious_irq(int irq);
PUBLIC void panic(const char *fmt, ...);

/* klib.c */
PUBLIC void	delay(int time);
PUBLIC void disp_int(int input);

/* printf.c */
PUBLIC  int     printf(const char *fmt, ...);
#define	printl	printf

/* vsprintf.c */
PUBLIC  int     vsprintf(char *buf, const char *fmt, va_list args);
PUBLIC	int	sprintf(char *buf, const char *fmt, ...);

/* proc.c */
PUBLIC	void*	va2la(int pid, void* va);
PUBLIC 	int 	proc2pid(TASK* p);
PUBLIC	void	reset_msg(MESSAGE* p);
PUBLIC	void	dump_msg(const char * title, MESSAGE* m);
PUBLIC	void	dump_proc(TASK * p);
//PUBLIC	int	send_recv(int function, int src_dest, MESSAGE* msg);
PUBLIC void inform_int(int task_nr);
PUBLIC int sys_sendrec(int function, int src_dest, MESSAGE* m);
PUBLIC int sys_get_pid();


/* task.c */
PUBLIC void schedule_new();

/* hd.c */
PUBLIC void sys_init_hd();
PUBLIC void sys_hd_rdwt(int io_type, int dev, u64 pos, int bytes, int proc_nr, void* buf);
PUBLIC void sys_hd_open	(int device);
PUBLIC void	sys_hd_close(int device);
PUBLIC void sys_hd_ioctl(int dev, int request, void* buf);
//PUBLIC void sys_hd_routine(MESSAGE *m);

/* tty.c */
PUBLIC int sys_printx(char* s);
PUBLIC int sys_tty_write(char* s, int len);
PUBLIC void sys_write_char_routine();
PUBLIC void sys_tty_routine();

/*klib.c*/
PUBLIC void sys_write_int_routine(int input);

/*fs/main.c*/
PUBLIC void sys_init_fs();
PUBLIC int sys_open(char *pathname, int flags);
PUBLIC int sys_close(int fd);
PUBLIC int sys_read(int fd, void *buf, int count);
PUBLIC int sys_write(int fd, void *buf, int count);


/*misc.c*/
PUBLIC int memcmp(const void * s1, const void *s2, int n);
PUBLIC int strcmp(const char * s1, const char *s2);
PUBLIC char * strcat(char * s1, const char *s2);

/*mm/main.c*/
PUBLIC int sys_fork();