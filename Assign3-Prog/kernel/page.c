#include "page.h"
#include "type.h"
#include "kernel.h"
#include "global.h"
#include "string.h"

static u8 phys_page_bitmap[BITMAP_SIZE] = {0};   // 位图，每个位对应一个页面

// 辅助函数：设置第 index 位（占用）
static void set_bit(u32 index) {
    phys_page_bitmap[index / 8] |= (1 << (index % 8));
}

// 辅助函数：清除第 index 位（空闲）
static void clear_bit(u32 index) {
    phys_page_bitmap[index / 8] &= ~(1 << (index % 8));
}

// 辅助函数：测试第 index 位是否被占用
static int test_bit(u32 index) {
    return (phys_page_bitmap[index / 8] >> (index % 8)) & 1;
}

/**
 * 分配一个物理页
 * @return 分配到的物理页起始地址，若没有空闲页则返回 0
 */
u32 alloc_phys_page() {
    for (u32 i = 0; i < TOTAL_PAGES; i++) {
        if (!test_bit(i)) {
            set_bit(i);
            u32 page = PHYS_PAGE_START + i * PAGE_SIZE;
            // 清零页面（假设虚拟地址 = 物理地址 + PAGE_OFFSET）
            memset((void*)(page + PAGE_OFFSET), 0, PAGE_SIZE);
            return page;
        }
    }
    return 0;   // 无可用内存
}

/**
 * 释放一个物理页
 * @param page 要释放的物理页起始地址
 */
void free(u32 page) {
    // 检查地址是否在管理范围内
    if (page < PHYS_PAGE_START || page >= PHYS_MEM_END) {
        return;   // 无效地址，可改为 panic 或其他错误处理
    }
    u32 index = (page - PHYS_PAGE_START) / PAGE_SIZE;
    if (test_bit(index)) {
        clear_bit(index);
    }
}

// 从虚拟地址获取当前任务的页目录项（根据 pid）
static inline u32* get_pgd_entry(u32 vaddr, int pid) {
    u32 idx = (vaddr >> 22) & 0x3FF;               // 页目录索引（高10位）
    return &pg_dir_tasks[pid * 1024 + idx];        // 每个任务占用 1024 个页目录项
}

// 获取页表项（根据 pid）
static inline u32* get_pte_entry(u32 vaddr, u32* pgd, int pid) {
    if (!(*pgd & PAGE_PRESENT)) {
        u32 pt_phys = (u32)pg_tasks - PAGE_OFFSET + ((u32)pgd - (u32)pg_dir_tasks);
        *pgd = pt_phys | PAGE_DEFAULT;
    }
    u32* pg_table = (u32*)((*pgd & PAGE_MASK) + PAGE_OFFSET);  // 页表线性地址
    u32 pte_idx = (vaddr >> 12) & 0x3FF;                       // 页表索引（中间10位）
    return &pg_table[pte_idx];
}

void *sys_brk(u32 new_brk) {
    TASK *task = current;
    u32 old_brk = task->heap_brk;

    // 如果参数为 0，只返回当前堆顶
    if (new_brk == 0) {
        return (void*)task->heap_brk;
    }

    // 仅允许在当前任务的合法堆区间内调整 brk
    if (new_brk < task->heap_start || new_brk > task->heap_limit) {
        return (void*)-1;
    }

    task->heap_brk = new_brk;
    return (void*)old_brk;
}

// pagefault_handler(线性地址, 错误码)
void pagefault_handler(u32 err_addr, u32 err_code) {
    // 处理逻辑：判断缺页原因（页面不存在/权限不足/写只读页等）
    // 例如：打印缺页地址、错误码，或尝试修复页表

	// 强制恢复内核数据段，避免段寄存器指向LDT
    __asm__ __volatile__(
        "movw %0, %%ds\n"
        "movw %0, %%es\n"
        "movw %0, %%fs\n"
        "movw %0, %%gs\n"
        "movw %0, %%ss\n"
        : : "r"(SELECTOR_KERNEL_DS) : "memory"
    );

    TASK *task = current;
    if (!((err_addr >= task->heap_start && err_addr <= task->heap_brk) || (err_addr >= task->stack_start && err_addr <= task->stack_end))) {
        sys_printx("\n[Fault!] Illegal page fault at ");
        sys_write_int_routine(err_addr);
        __asm__ __volatile__("hlt");
    }

    sys_printx("\n===== Page Fault Triggered =====");
	sys_printx("\nPage Fault at address: ");
	sys_write_int_routine(err_addr); 
	sys_printx("\nError Code: "); 
	sys_write_int_routine(err_code);

	// 分配物理页
	u32 phys_page = alloc_phys_page();
    sys_printx("\nAllocate physical page:");
	sys_write_int_routine(phys_page);

	// 获取虚拟地址对应的页表项
    u32 vaddr_aligned = err_addr & PAGE_MASK;  // 对齐到页起始地址
    u32* pgd = get_pgd_entry(vaddr_aligned, task->pid);
    u32* pte = get_pte_entry(vaddr_aligned, pgd, task->pid);

    // 建立页表映射（虚拟→物理）
    *pte = (phys_page & PAGE_MASK) | PAGE_DEFAULT;  // 设置权限：存在+可写+用户态

    // 刷新TLB（避免CPU缓存旧页表）
    __asm__ __volatile__("invlpg (%0)" : : "r"(vaddr_aligned) : "memory");

    // 打印成功信息
    sys_printx("\nPage mapped: VA=");
	sys_write_int_routine(vaddr_aligned); 
	sys_printx("-> PA=");
	sys_write_int_routine(phys_page);
    sys_printx("\n===== Page Fault Resolved =====\n");
	
	//__asm__ __volatile__("hlt");
}

void set_task_paging(int pid){
	unsigned int task_base = 0x50000;
	unsigned int task_size = 0x10000;
	unsigned int mask = 0x07;
	unsigned int index = pid * 1024;

	//pte_t init_page_entry = task_base + task_size * pid + mask;
	pte_t init_page_entry = task_base + mask;
	for (int i = 0; i < pid; i++) {
		init_page_entry = init_page_entry + task_size;
	}
	
	for (int i = 0; i <= 16; i++) {
		// only allocate page 0, 1, 2 for code and data; page 15 for stack.
		if (i < 3 || i > 14){
			pg_tasks[index + i] = init_page_entry;
		}else{
			pg_tasks[index + i] = 0;
		}
        init_page_entry = init_page_entry + 0x1000;
	}

	//pg_dir_tasks[index] = (pte_t) pg_tasks + index * 4 + mask - PAGE_OFFSET;
	pte_t tmp = (pte_t) pg_tasks + mask - PAGE_OFFSET;
	for (int i = 0; i < pid; i++) {
		tmp = tmp + 0x1000;
	}
	pg_dir_tasks[index] = tmp;
	
	pg_dir_tasks[index + 768] = (pte_t) pg1 + mask - PAGE_OFFSET;
}

void sys_print_task_paging(int pid) {
	unsigned int task_base = 0x50000;
	unsigned int task_size = 0x10000;
	unsigned int mask = 0x07;
	unsigned int index = pid * 1024;

	sys_printx("print page mappings for pid [");
	sys_write_int_routine(pid);
	sys_printx("]\n");

	
	for (int i = 0; i < 16; i++) {
		sys_printx("page[");
		sys_write_int_routine(i);
		sys_printx("] is [");
		sys_write_int_routine(pg_tasks[index + i]);
		sys_printx("]\n");
	}

	sys_printx("page table entry [");
	sys_write_int_routine(0);
	sys_printx("] is [");
	sys_write_int_routine(pg_dir_tasks[index]);
	sys_printx("]\n");

	sys_printx("page table entry[");
	sys_write_int_routine(768);
	sys_printx("] is [");
	sys_write_int_routine(pg_dir_tasks[index + 768]);
	sys_printx("]\n");
}