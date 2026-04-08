#ifndef	_PAGE_H_
#define	_PAGE_H_
#include "type.h"

// page table entry
typedef unsigned int pte_t;

extern pte_t pg_dir[1024];
extern pte_t pg0[1024];
extern pte_t pg1[1024];

extern pte_t pg_dir_tasks[4096];
extern pte_t pg_tasks[4096];

// task0 and task1 share the pg1

#define PAGE_OFFSET 0xC0000000
#define PROC_BASE 0x50000
#define PROC_LEN 0x10000


#define SELECTOR_KERNEL_DS 0x10  

#define PAGE_SIZE 0x1000  // 4KB
#define PHYS_PAGE_START 0x100000    // 开始于1MB
#define PHYS_MEM_END   0x1000000   // 假设物理内存结束于 16MB，可按需调整
#define TOTAL_PAGES    ((PHYS_MEM_END - PHYS_PAGE_START) >> 12)
#define BITMAP_SIZE    ((TOTAL_PAGES + 7) >> 3)



#define PAGE_MASK 0xFFFFF000  // 页地址掩码（取页起始地址）
#define PAGE_ALIGN(addr) (((addr) + PAGE_SIZE - 1) & PAGE_MASK)
#define FREE_PHYS_PAGE 0x100000
// 权限位定义（和你的set_task_paging中mask=0x07对应）
#define PAGE_PRESENT  0x01  // 存在位
#define PAGE_WRITE    0x02  // 可写位
#define PAGE_USER     0x04  // 用户态可访问位
#define PAGE_DEFAULT  (PAGE_PRESENT | PAGE_WRITE | PAGE_USER)  // 0x07

#define HEAP_START  0x3000
#define HEAP_LIMIT  0x9000

#define STACK_START  0xF000
#define STACK_END    0xFFFF

void set_task_paging(int pid);
void sys_print_task_paging(int pid);

#endif