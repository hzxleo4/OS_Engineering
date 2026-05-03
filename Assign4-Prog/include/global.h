/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* EXTERN is defined as extern except in global.c */
#ifdef	GLOBAL_VARIABLES_HERE
#undef	EXTERN
#define	EXTERN
#endif

#include "tty.h"
#include "console.h"
#include "fs.h"
#include "task.h"

EXTERN	int		disp_pos;
//EXTERN	u32		k_reenter;
EXTERN	int		nr_current_console;

extern	irq_handler	irq_table[];
extern	TTY		tty_table[];
extern  CONSOLE         console_table[];
//extern	struct dev_drv_map	dd_map[];

/* FS */
EXTERN	struct file_desc	f_desc_table[NR_FILE_DESC];
EXTERN	struct inode		inode_table[NR_INODE];
EXTERN	struct super_block	super_block[NR_SUPER_BLOCK];

EXTERN	TASK *		pcaller;
EXTERN	struct inode *		root_inode;

extern u32 current_esp_in_syscall;