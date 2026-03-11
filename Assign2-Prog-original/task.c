#include "task.h"
#include "kernel.h"

// #define __pa(x) ((unsigned long)(x) - PAGE_OFFSET)

/*
 * Volatile isn't enough to prevent the compiler from reordering the
 * read/write functions for the control registers and messing everything up.
 * A memory clobber would solve the problem, but would prevent reordering of
 * all loads stores around it, which can hurt performance. Solution is to
 * use a variable and mimic reads and writes to it to enforce serialization
 */
static unsigned long __force_order;


static inline void load_cr3(pte_t pgdir)
{
	// write_cr3(__pa(pgdir));
	asm volatile("mov %0,%%cr3": : "r" (pgdir), "m" (__force_order));
}



/*TASK * __switch_to(TASK *prev_p, TASK *next_p)
{
	
	 * Reload esp0.
	 
	tss0.esp0 = next_p->sp0;

	return prev_p;
}*/

TASK * __switch_to()
{
	/*
	 * Reload esp0.
	 */
	register TASK* prev asm("eax");
	register TASK* next asm("edx");

	tss0.esp0 = next->sp0;

	return prev;
}



#define switch_to(prev, next, last)					\
do {									\
	/*								\
	 * Context-switching clobbers all registers, so we clobber	\
	 * them explicitly, via unused output variables.		\
	 * (EAX and EBP is not listed because EBP is saved/restored	\
	 * explicitly for wchan access and EAX is the return value of	\
	 * __switch_to())						\
	 */								\
	unsigned long ebx, ecx, edx, esi, edi;				\
									\
	asm volatile("pushfl\n\t"		/* save    flags */	\
		     "pushl %%ebp\n\t"		/* save    EBP   */	\
		     "movl %%esp,%[prev_sp]\n\t"	/* save    ESP   */ \
		     "movl %[next_sp],%%esp\n\t"	/* restore ESP   */ \
		     "movl $1f,%[prev_ip]\n\t"	/* save    EIP   */	\
		     "pushl %[next_ip]\n\t"	/* restore EIP   */	\
		     "jmp __switch_to\n"	/* regparm call  */	\
		     "1:\t"						\
		     "popl %%ebp\n\t"		/* restore EBP   */	\
		     "popfl\n"			/* restore flags */	\
									\
		     /* output parameters */				\
		     : [prev_sp] "=m" (prev->sp),		\
		       [prev_ip] "=m" (prev->ip),		\
		       "=a" (last),					\
									\
		       /* clobbered output registers: */		\
		       "=b" (ebx), "=c" (ecx), "=d" (edx),		\
		       "=S" (esi), "=D" (edi)				\
		       							\
		       /* input parameters: */				\
		     : [next_sp]  "m" (next->sp),		\
		       [next_ip]  "m" (next->ip),		\
		       							\
		       /* regparm parameters for __switch_to(): */	\
		       [prev]     "a" (prev),				\
		       [next]     "d" (next));				\
} while (0)

void context_switch(TASK* prev, TASK* next) {
	// change page change
	load_cr3(next->pg_dir);

	switch_to(prev, next, prev);

}


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

	context_switch(prev, next);

}

