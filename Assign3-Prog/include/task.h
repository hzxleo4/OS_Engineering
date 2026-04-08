#ifndef	_TASK_H_
#define	_TASK_H_
#include "type.h"
#include "page.h"
#include "const.h"

typedef struct task_struct
{
	u32 pid;
	char name[16];		   /* name of the process */
	int p_parent; /**< pid of parent process */
	int exit_status; /**< for parent */

	// address space (page directory)
	pte_t pg_dir;
	
	/* CPU-specific state of this task */
	u32 sp;
	u32 sp0;
	u32 ip;
	
	int nr_tty;
	int  p_flags;              /**
				    * process flags.
				    * A proc is runnable if p_flags==0
				    */

	MESSAGE * p_msg;
	int p_recvfrom;
	int p_sendto;
	int has_int_msg;           /**
				    * nonzero if an INTERRUPT occurred when
				    * the task is not ready to deal with it.
				    */
	struct task_struct * q_sending;   /**
				    * queue of procs sending messages to
				    * this proc
				    */
	struct task_struct * next_sending;/**
				    * next proc in the sending
				    * queue (q_sending)
				    */
	
	struct file_desc * filp[NR_FILES];

	u32 heap_start;   // 堆区域的起始虚拟地址
    u32 heap_brk;     // 当前堆的结束地址
    u32 heap_limit;   // 堆的最大允许地址

	u32 stack_start;	
	u32 stack_end; 		
} TASK, PROC; // TASK = PROC

TASK tasks[2];
TASK* current;
TASK* prev;
TASK* next;

/* Number of tasks*/
#define NR_TASKS	2
#define NR_PROCS	2
#define FIRST_PROC		tasks[0]
#define LAST_PROC		tasks[NR_PROCS - 1]

#endif


