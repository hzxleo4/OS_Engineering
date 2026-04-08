#include "task.h"
#include "kernel.h"

#define my_switch_to(prev, next) \
do { \
    asm volatile ( \
        "pushl %%ebp\n\t"                /* 保存 ebp */ \
        "pushl %%edi\n\t"                /* 保存 edi */ \
        "pushl %%esi\n\t"                /* 保存 esi */ \
        "pushl %%ebx\n\t"                /* 保存 ebx */ \
        "pushl %%ecx\n\t"                /* 保存 ecx */ \
        "pushl %%edx\n\t"                /* 保存 edx */ \
        "movl %%esp, %[prev_sp]\n\t"     /* 保存 esp */ \
        "movl $1f, %[prev_ip]\n\t"       /* 保存 eip */ \
        "movl %[next_sp], %%esp\n\t"     /* 切换到新栈 */ \
        "jmp *%[next_ip]\n\t"            /* 跳转到新进程的 eip */ \
        "1:\n\t"                         /* 返回点 */ \
        "popl %%edx\n\t"                 /* 恢复 edx */ \
        "popl %%ecx\n\t"                 /* 恢复 ecx */ \
        "popl %%ebx\n\t"                 /* 恢复 ebx */ \
        "popl %%esi\n\t"                 /* 恢复 esi */ \
        "popl %%edi\n\t"                 /* 恢复 edi */ \
        "popl %%ebp\n\t"                 /* 恢复 ebp */ \
        : [prev_sp] "=m" (prev->sp), \
          [prev_ip] "=m" (prev->ip) \
        : [next_sp] "m" (next->sp), \
          [next_ip] "m" (next->ip) \
        : "memory" \
    ); \
} while (0)

void schedule_new() {
	int next_pid = (current->pid + 1) % NR_TASKS;   // 计算下一个任务的 PID
    next = &tasks[next_pid];

	if (next->p_flags == 0){
		prev = current;
		current = next;
		// change page table
		asm volatile("mov %0,%%cr3": : "r" (next->pg_dir));

		// Reload esp0.
		tss0.esp0 = next->sp0;
		my_switch_to(prev, next);
	}
}