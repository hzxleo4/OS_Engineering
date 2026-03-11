#include "task.h"
#include "kernel.h"



#define my_switch_to(prev, next)					\
do {									\
	asm volatile("movl %%esp,%[prev_sp]\n\t"	/* save    ESP   */ \
			 "movl $1f,%[prev_ip]\n\t"	/* save    EIP   */	\
		     "movl %[next_sp],%%esp\n\t"	/* restore ESP   */ \
		     "jmp %[next_ip]\n"	/* jump  */	\
		     "1:\t"						\
									\
		     /* output parameters */				\
		     : [prev_sp] "=m" (prev->sp),		\
		       [prev_ip] "=m" (prev->ip)		\
		       /* input parameters: */				\
		     : [next_sp]  "m" (next->sp),		\
		       [next_ip]  "m" (next->ip));				\
} while (0)


void schedule_new() {
	if (current->pid == 0) {
		next = &tasks[1];
	} else if (current->pid == 1) {
		next = &tasks[0];
	} else {
		next = &tasks[0];
	}
	prev = current;
	current = next;

	// change page table
	asm volatile("mov %0,%%cr3": : "r" (next->pg_dir));

	// Reload esp0.
	tss0.esp0 = next->sp0;

	// save SP and IP for prev, restore SP and IP for next, set tss.sp0
	my_switch_to(prev, next);
}

