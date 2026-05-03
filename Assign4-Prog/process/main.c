#include "type.h"
#include "config.h"
#include "const.h"
//#include "protect.h"
#include "string.h"
#include "fs.h"
#include "task.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"
#include "stdio.h"
#include "page.h"

#include "hd.h"

PUBLIC int do_fork(u32 parent_esp_addr);
PUBLIC int sys_fork();

PUBLIC int sys_fork()
{
    return do_fork(current_esp_in_syscall);
}

PUBLIC void fs_cpy(TASK* dst, TASK* src)
{
	int i;
	for (i = 0; i < NR_FILES; i++) {
		if (src->filp[i]) {
			dst->filp[i] = src->filp[i];
		}
	}
}

PUBLIC void proc_cpy(TASK* dst, TASK* src){
	strcpy(dst->name, src->name);
	dst->p_parent = src->p_parent;
	dst->exit_status = src->exit_status;
	dst->pg_dir = src->pg_dir;
	dst->sp = src->sp;
	dst->sp0 = src->sp0;
	dst->ip = src->ip;
	dst->nr_tty = src->nr_tty;
	dst->p_msg = src->p_msg;
	dst->p_recvfrom = src->p_recvfrom;
	dst->p_sendto = src->p_sendto;
	dst->has_int_msg = src->has_int_msg;
	dst->q_sending = src->q_sending;
	dst->next_sending = src->next_sending;
	//dst->filp = src->filp;
	fs_cpy(dst, src);
}

PUBLIC int do_fork(u32 parent_esp_addr)
{
	TASK* t = tasks;
	int i;
	for (i = 0; i < NR_TASKS; i++, t++){
		if (t->p_flags == FREE_SLOT){
			break;
		}
	}
	int child_pid = i;
	
	if (t != &tasks[child_pid]){
		sys_printx("ERROR!\n");
		return -1;
	}

	if (i == NR_TASKS){
		sys_printx("NO FREE SLOT\n");
		return -1;
	}

	/* duplicate the process table */
	int pid = current->pid;
	sys_printx("\nCurrent pid is ");
	sys_write_int_routine(pid);

	pte_t child_pd_dir = t->pg_dir;
	u32 child_sp0 = t->sp0;
	//u32 child_sp = t->sp;
	//u32 child_ip = t->ip;


	
	proc_cpy(t, &tasks[pid]);
	t->pid = child_pid;
	
	t->pg_dir = child_pd_dir;
	t->sp0 = child_sp0;
	//t->sp = child_sp;
	//t->ip = child_ip;
	t->p_parent = pid;
	t->p_flags = WAITING;
	//sys_printx("\nParent pid is ");
	//sys_write_int_routine(t->p_parent);

	//Allocate memory
	set_task_paging(child_pid);
	//sys_print_task_paging(child_pid);
	// child is a copy of the parent
	phys_copy(    
		(void*)(PROC_BASE + PAGE_OFFSET + child_pid * PROC_LEN),  // 目标地址
    	(void*)(PROC_BASE + PAGE_OFFSET + pid * PROC_LEN),        // 源地址
    	PROC_LEN                                       // 复制长度
	);

	/*
	 * Copy the active syscall kernel-stack frame from parent stack to child stack
	 * and remap child's esp to the same offset in its own kernel stack.
	 */
	u32 parent_sp0 = tasks[pid].sp0;
	u32 used_stack_bytes = parent_sp0 - parent_esp_addr;
	u32 child_esp = t->sp0 - used_stack_bytes;
	phys_copy((void*)child_esp, (void*)parent_esp_addr, used_stack_bytes);
	t->sp = child_esp;
	
	extern u32 child_fork_ret;
	t->ip = (u32)&child_fork_ret;

	t->p_flags = READY;
	return child_pid;
}