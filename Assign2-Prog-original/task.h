#ifndef	_TASK_H_
#define	_TASK_H_
#include "type.h"
#include "page.h"

typedef struct task_struct
{
	u32 pid;
	// address space (page directory)
	pte_t pg_dir;
	
	/* CPU-specific state of this task */
	u32 sp;
	u32 sp0;
	u32 ip;
} TASK;

TASK tasks[2];
TASK* current;
TASK* prev;
TASK* next;

#endif


