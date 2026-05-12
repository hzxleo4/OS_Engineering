#include "task.h"
#include "kernel.h"



//#define my_switch_to(prev, next)					\
do {									\
	asm volatile("movl %%esp,%[prev_sp]\n\t"	/* save    ESP   */ \
			 "movl $1f,%[prev_ip]\n\t"	/* save    EIP   */	\
		     "movl %[next_sp],%%esp\n\t"	/* restore ESP   */ \
		     "jmp *%[next_ip]\n"	/* jump  */	\
		     "1:\t"						\
									\
		     /* output parameters */				\
		     : [prev_sp] "=m" (prev->sp),		\
		       [prev_ip] "=m" (prev->ip)		\
		       /* input parameters: */				\
		     : [next_sp]  "m" (next->sp),		\
		       [next_ip]  "m" (next->ip));				\
} while (0)

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
    int i;
    // 从当前任务的下一个开始环形遍历整个任务数组
    for (i = (current->pid + 1) % NR_TASKS; i != current->pid; i = (i + 1) % NR_TASKS) {
        if (tasks[i].p_flags == READY) {
            next = &tasks[i];
            break;
        }
    }
    // 若没有找到其他就绪任务，则继续运行当前任务
    if (i == current->pid) {
        return;
    }
    // 执行上下文切换
    prev = current;
    current = next;
    // 切换页表
    asm volatile("mov %0,%%cr3": : "r" (next->pg_dir));
    // 重载 esp0
    tss0.esp0 = next->sp0;
    // 保存前一个任务的栈和指令指针，恢复下一个任务的状态
    my_switch_to(prev, next);
}

/**
 * 重新启动当前进程，使其从新的 IP 和 SP 开始执行。
 * @param new_ip  新的指令指针地址
 * @param new_sp  新的栈指针地址
 */
void restart_current_proc(u32 new_ip, u32 new_sp) {
    /*
     * 使用内联汇编设置新的栈指针（ESP）并跳转到新的指令指针（EIP）。
     * 注意：此操作不会保存任何当前上下文，调用后原执行流终止。
     * 新栈必须已正确初始化，新 IP 处的代码应能正常执行（例如拥有有效的返回地址等）。
     */
    asm volatile (
        "movl %0, %%esp\n\t"   /* 将新的 SP 加载到 ESP 寄存器 */
		"pushl $0x17\n\t"
		"pushl $0x10000\n\t"
		"pushfl\n\t"
		"pushl $0x0f\n\t"
		"pushl $0x0\n\t"
        "jmp *%1\n\t"          /* 间接跳转到新的 IP 地址 */
        : : "r" (new_sp), "r" (new_ip)  /* 输入操作数 */
        : "memory"            /* 告诉编译器内存可能被修改 */
    );

    /* 永远不会执行到此处（上述跳转已结束当前函数） */
    while (1);
}

//void sys_getticks(){
//
//	//printf("<Ticks:%d>", msg.RETVAL);
//	disp_int(msg.RETVAL);
//}