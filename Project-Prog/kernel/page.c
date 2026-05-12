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
 * 分配多个连续的物理页
 * @param size 需要分配的字节数
 * @return 分配到的第一个物理页起始地址，若失败则返回 0
 */
u32 alloc_phys_pages(u32 size) {
    if (size == 0) return 0;

    u32 num_pages = (size + PAGE_SIZE - 1) >> 12;  // 除以PAGE_SIZE,向上取整
	if (num_pages > TOTAL_PAGES) return 0;

	/* 查找一段连续空闲页 */
	for (u32 start = 0; start + num_pages <= TOTAL_PAGES; start++) {
		int ok = 1;
		for (u32 i = 0; i < num_pages; i++) {
			if (test_bit(start + i)) {
				ok = 0;
				start += i; /* 小优化：直接跳过已占用页 */
				break;
			}
		}
		if (!ok) continue;

		/* 标记并清零 */
		for (u32 i = 0; i < num_pages; i++) {
			set_bit(start + i);
			u32 page = PHYS_PAGE_START + (start + i) * PAGE_SIZE;
			memset((void*)(page + PAGE_OFFSET), 0, PAGE_SIZE);
		}

		return PHYS_PAGE_START + start * PAGE_SIZE;
    }

	return 0;
}

/**
 * 分配多个物理页
 * @param size 需要分配的字节数
 * @return 分配到的第一个物理页虚拟地址，若失败则返回 0
 */
u32 kmalloc(int size) {
	u32 page = alloc_phys_pages(size);
	if (page == 0) {
		return 0;
	}
	u32 page_virtual = page + PAGE_OFFSET;
	return (page_virtual);
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

void set_task_paging(int pid){
	//unsigned int task_base = 0x400000;
	//unsigned int task_size = 0x400000;
	unsigned int task_base = 0x50000;
	unsigned int task_size = 0x10000;
	unsigned int mask = 0x07;
	unsigned int index = pid * 1024;

	//pte_t init_page_entry = task_base + task_size * pid + mask;
	pte_t init_page_entry = task_base + mask;
	for (int i = 0; i < pid; i++) {
		init_page_entry = init_page_entry + task_size;
	}
	
	for (int i = 0; i < 16; i++) {
		// only allocate page 0 and page 15
		if (i == 0 || i == 15){
			pg_tasks[index + i] = init_page_entry;
			init_page_entry = init_page_entry + 0x1000;
		}else{
			pg_tasks[index + i] = 0;
		}
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


//void set_page_directory(pte_t *pg_dir, void* user_entry_value, void* kernel_entry_value) {
	
//	pg_dir[0] = (pte_t)user_entry_value+0x07-PAGE_OFFSET;
//	pg_dir[768] = (pte_t)kernel_entry_value+0x07-PAGE_OFFSET;
//}