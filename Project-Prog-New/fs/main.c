/*************************************************************************//**
 *****************************************************************************
 * @file   main.c
 * @brief  
 * @author Forrest Y. Yu
 * @date   2007
 *****************************************************************************
 *****************************************************************************/

#include "type.h"
#include "config.h"
#include "const.h"
#include "string.h"
#include "fs.h"
#include "task.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"
#include "stdio.h"

#include "hd.h"
PUBLIC void sys_init_fs();
PUBLIC int sys_open(char *pathname, int flags);
PUBLIC int sys_mkdir(char *pathname);
PUBLIC int sys_close(int fd);
PUBLIC int sys_read(int fd, void *buf, int count);
PUBLIC int sys_write(int fd, void *buf, int count);
PUBLIC int sys_rdwt(int io_type, int fd, void *buf, int count);
PUBLIC int sys_delete(char *pathname);
PUBLIC void sys_flush(void);

PRIVATE int write_block(int block_nr, void *buf, int bytes);
PRIVATE int read_block(int block_nr, void* buf, int bytes);
PRIVATE int write_through(int block_nr, void *buf, int bytes);
PRIVATE int read_through(int block_nr, void* buf, int bytes);
void cache_init(void);
static void lru_touch(struct cache_block* blk);
struct cache_block* cache_find(u32 block_nr);
void cache_add(u32 block_nr, u8* data, int dirty);
void cache_evict(void);
void cache_sync_all(void);
PRIVATE void mkfs();
PRIVATE struct inode * create_file(char * path, int flags);
int alloc_inode(void);
int alloc_block(void);
static int alloc_block_for_inode(struct inode *inode, int logical);
static int free_block(int block_nr);
static int free_inode(int inode_nr);
static int write_inode_to_disk(struct inode *inode);
static struct inode *get_inode_slot(void);
static int add_dir_entry(struct inode *dir, char *name, int inode_nr);
PUBLIC void put_inode(struct inode * pinode);
int strip_path(char *filename, const char *path, struct inode **dir_inode);
PUBLIC struct super_block *	get_super_block();
PRIVATE void read_super_block();
PUBLIC int search_file(char *path);
static struct inode * read_inode(int inode_nr, u32 inode_region_start, u32 inode_size);
PRIVATE int get_block_nr(struct inode *inode, int block_index);
PRIVATE int find_in_dir(struct inode *dir_inode, char *name, u32 dir_ent_size);
char *strtok_r(char *str, const char *delim, char **saveptr);
char *strrchr(const char *s, int c);
PUBLIC void dump_dir_recursive(int inode_nr, int depth, int max_depth);
PUBLIC void dump_fs(void);


/* Please complete the missing code in sys_mkdir() and sys_rdwt()*/

/**
 * Create a directory.
 * @param pathname The path of the directory to create.
 * @return 0 on success, -1 on failure.
 */
PUBLIC int sys_mkdir(char *pathname)
{
    struct super_block *super_b = get_super_block();
    char dirname[MAX_PATH];
    struct inode *parent_dir;
    int inode_nr;
    struct inode *new_dir;
    int block_nr;
    u8 *block_buf;
    /* 1. Check if the path is valid and the directory does not already exist */
    int existing_inode = search_file(pathname);
    if (existing_inode == -1) {
        sys_printx("\nmkdir: invalid path");
        return -1;
    }
    if (existing_inode > 0) {
        sys_printx("\nmkdir: directory already exists");
        return -1;
    }
    /* 2. Split path into directory name and parent directory inode */
    if (strip_path(dirname, pathname, &parent_dir) != 0) {
        sys_printx("\nmkdir: strip_path failed");
        return -1;
    }
    /* 3. Allocate a new inode number */
    inode_nr = alloc_inode();
    if (inode_nr < 0) {
        sys_printx("\nmkdir: failed to allocate inode");
        return -1;
    }
    /* 4. Get a free inode slot from the cache */
    new_dir = get_inode_slot();
    if (!new_dir) {
        sys_printx("\nmkdir: no free inode slots in cache");
        free_inode(inode_nr);
        return -1;
    }  
    /* 5. Allocate a data block for the directory content */
    block_nr = alloc_block();
    if (block_nr < 0) {
        sys_printx("\nmkdir: no free blocks");
        free_inode(inode_nr);
        return -1;
    }
    /* 6. Initialize the inode for the directory, specifically, inode->imode --> I_DIRECTORY (include/const.h) */
    memset(new_dir, 0, sizeof(struct inode));
    new_dir->i_mode = I_DIRECTORY;
    new_dir->i_size = DIR_ENTRY_SIZE * 2; /* Initially contains "." and ".." */
    new_dir->i_start_block = block_nr;
    new_dir->i_nr_blocks = 1;
    new_dir->i_num = inode_nr;
    new_dir->i_cnt = 1;
    // new_dir->i_direct[0] = block_nr;
    for (int i = 0; i < INODE_DIRECT_COUNT; i++)
        new_dir->i_direct[i] = (i == 0) ? block_nr : 0;
    new_dir->i_indirect = 0;
    /* 7. Write the inode to disk */
    if (write_inode_to_disk(new_dir) != 0) {
        sys_printx("\nmkdir: failed to write inode to disk");
        free_inode(inode_nr);
        free_block(block_nr);
        return -1;
    }
    /* 8. Initialize the directory data block with "." and ".." entries. 
          You can use kmalloc(super_b->block_size) in kernel/page.c to allocate a buffer.
          For example: block_buf = (u8 *)kmalloc(super_b->block_size); */
    block_buf = (u8 *)kmalloc(super_b->block_size);
    if (!block_buf) 
        return -1;
    memset(block_buf, 0, super_b->block_size);

    struct dir_entry *pde = (struct dir_entry *)block_buf;
    
    pde->inode_nr = inode_nr;
    strcpy(pde->name, ".");
    
    pde++;
    pde->inode_nr = parent_dir->i_num;
    strcpy(pde->name, "..");

    write_through(block_nr, block_buf, super_b->block_size);
    /* 9. Add the directory entry in the parent directory */
    if (add_dir_entry(parent_dir, dirname, inode_nr) != 0) {
        sys_printx("\nmkdir: add_dir_entry failed");
        free_inode(inode_nr);
        free_block(block_nr);
        return -1;
    }


    sys_printx("\nmkdir: directory created successfully");
    return 0;
}

/*-----------sys_rdwt----------------------------------------*
 *  This function handles reading from or writing to a file. *
 *-----------------------------------------------------------*/

PUBLIC int sys_rdwt(int io_type, int fd, void *buf, int count)
{
    /*1. Checks validity: It first ensures the file descriptor (fd) is valid, the file is open, and the mode allows reading/writing.*/
    if (fd < 0 || fd >= NR_FILES) {
        sys_printx("\nsys_rdwt: invalid fd\n");
        return -1;
    }
    if (current->filp[fd] == 0) {
        sys_printx("\nsys_rdwt: file not open\n"); 
        return -1;
    } 
    struct file_desc *f_desc = current->filp[fd];
    /* 检查读写权限 */
    if (!(current->filp[fd]->fd_mode & O_RDWR))
        return -1;
    /*2. Gets file info: Retrieves the file’s inode (metadata) and current position (pos).*/
    struct inode *pin = f_desc->fd_inode;
    int pos = f_desc->fd_pos;
    /*3. Determines limits:  For read: You can only read up to the file’s current size.    *
    /*   For write: You can extend the file, but only up to the number of allocated blocks.*
    /*   If more space is needed, new blocks are allocated dynamically.                    */
    if (io_type == READ) {
        if (pos >= pin->i_size)
            return 0;
        if (pos + count > pin->i_size)
            count = pin->i_size - pos;
    }
    if (io_type == WRITE) {
        int end_pos = pos + count;
        int needed_blocks =
            (end_pos + BLOCK_SIZE - 1) >> BLOCK_SIZE_SHIFT;
        for (int i = 0; i < needed_blocks; i++) {
            int phys = get_block_nr(pin, i);
            if (phys <= 0) {
                if (alloc_block_for_inode(pin, i) < 0) {
                    sys_printx("\nsys_rdwt: alloc block failed\n");
                    return -1;
                }
            }
        }
    }
    /*4. Block-level processing: 
         Files are stored in blocks. The function calculates which blocks need to be   *
         accessed based on the position and length. For each block:                    *
         (1) Loads the block into memory (using read_block()).                         *
         (2) Copies data between the user buffer and the block buffer (generated by    *  
             kmalloc()).                                                               *
         (3) For writing, saves the updated block back to disk (using write_block()).  */
    int bytes_done = 0;
    u8 *block_buf = (u8 *)kmalloc(BLOCK_SIZE);
    while (bytes_done < count) {
        int cur_pos = pos + bytes_done;
        int logical_block =
            cur_pos >> BLOCK_SIZE_SHIFT;
        int block_offset =
            cur_pos & (BLOCK_SIZE - 1);
        int bytes_left = count - bytes_done;
        int bytes_this_round =
            BLOCK_SIZE - block_offset;
        if (bytes_this_round > bytes_left)
            bytes_this_round = bytes_left;
        int phys_block =
            get_block_nr(pin, logical_block);
        /* READ hole -> 返回 0 */
        if (phys_block <= 0) {
            if (io_type == READ) {
                memset(block_buf, 0, BLOCK_SIZE);
            } else {
                sys_printx("\nsys_rdwt: invalid write block\n");
                return -1;
            }
        } else {
            /* 读取 block */
            if (read_block(phys_block,
                           block_buf,
                           BLOCK_SIZE) != 0) {
                sys_printx("\nsys_rdwt: read_block failed\n");
                return -1;
            }
        }
        /* READ */
        if (io_type == READ) {
            memcpy(
                (u8 *)buf + bytes_done,
                block_buf + block_offset,
                bytes_this_round
            );
        } else {
            /* WRITE */
            memcpy(
                block_buf + block_offset,
                (u8 *)buf + bytes_done,
                bytes_this_round
            );
            if (write_block(phys_block,
                            block_buf,
                            BLOCK_SIZE) != 0) {

                sys_printx("\nsys_rdwt: write_block failed\n");
                return -1;
            }
        }
        bytes_done += bytes_this_round;
    }
    /*5. Updates metadata:                                                                  *                                     
         (1) Advances the file position in memory (filp[fd]->fd_pos).                       *
         (2) If writing extends the file size, updates the inode and writes it back to disk.*/
    f_desc->fd_pos += bytes_done;
    if (io_type == WRITE) {
        if (f_desc->fd_pos > pin->i_size) {
            pin->i_size = f_desc->fd_pos;
            if (write_inode_to_disk(pin) != 0) {
                sys_printx("\nsys_rdwt: write inode failed\n");
                return -1;
            }
        }
    }
    return bytes_done;
}


/*****************************************************************************
 *                                init_fs
 *****************************************************************************/
/**
 * <Ring 0> Do some preparation.
 * 
 *****************************************************************************/
PUBLIC void sys_init_fs()
{
	int i;
    
	/* f_desc_table[] */
	for (i = 0; i < NR_FILE_DESC; i++)
		memset(&f_desc_table[i], 0, sizeof(struct file_desc));

	/* inode_table[] */
	for (i = 0; i < NR_INODE; i++)
		memset(&inode_table[i], 0, sizeof(struct inode));
        
	cache_init();

	/* make FS */
	mkfs();
	
	read_super_block();
    struct super_block * super_b;
	super_b = get_super_block();

	root_inode = read_inode(super_b->root_inode, super_b->first_data_block - super_b->nr_inode_blocks, super_b->inode_size);
    
    //dump_fs();
}


/**
 * 打开文件系统调用
 * @param pathname 文件路径
 * @param flags    打开标志（O_RDONLY, O_WRONLY, O_RDWR, O_CREAT等）
 * @return 成功返回文件描述符，失败返回-1
 */
PUBLIC int sys_open(char *pathname, int flags)
{
    int fd = -1;
    int inode_nr;
    struct inode *pin = NULL;
    int i;
    struct super_block * super_b = get_super_block();

    /* 1. 获取文件inode号（-1: 路径无效，0: 文件不存在，>0: 存在） */
    inode_nr = search_file(pathname);
    if (inode_nr == -1) {
        /* 路径无效（中间目录不存在） */
        sys_printx("\nsearch_file: invalid path\n");
        return -1;
    }

    /* 2. 在进程文件描述符表中找空闲槽位 */
    for (i = 0; i < NR_FILES; i++) {
        if (current->filp[i] == 0) {
            fd = i;
            break;
        }
    }
    if (fd < 0 || i >= NR_FILES) {
        sys_printx("\nfilp[] is full, the fd is ");
        sys_write_int_routine(fd);
        sys_printx("\nthe pid is ");
        sys_write_int_routine(proc2pid(current));
        return -1;
    }

    /* 3. 在全局文件描述符表中找空闲槽位 */
    for (i = 0; i < NR_FILE_DESC; i++)
        if (f_desc_table[i].fd_inode == 0)
            break;
    if (i >= NR_FILE_DESC) {
        sys_printx("\nf_desc_table[] is full");
        return -1;
    }

    /* 4. 根据标志位决定是创建还是打开已有文件 */
    if (flags & O_CREAT) {
        if (inode_nr > 0) {
            sys_printx("file exists.\n");
            return -1;
        } else {
            sys_printx("\nfile does not exist, create it");
            pin = create_file(pathname, flags);
        }
    } else {
        if (inode_nr <= 0) {
            sys_printx("\nfile not found.\n");
            return -1;
        }
        /* 计算inode区域起始块号 */
        u32 inode_region_start = super_b->first_data_block - super_b->nr_inode_blocks;
		pin = read_inode(inode_nr, inode_region_start, super_b->inode_size);
        if (pin == NULL) {
            sys_printx("read_inode failed\n");
            return -1;
        }
    }

    /* 5. 关联进程文件描述符与全局文件描述符表，并初始化 */
    if (pin) {
        current->filp[fd] = &f_desc_table[i];
        f_desc_table[i].fd_inode = pin;
        f_desc_table[i].fd_mode = flags;
        f_desc_table[i].fd_pos = 0;
    } else {
        return -1;
    }

    return fd;
}

/*****************************************************************************
 *                                do_close
 *****************************************************************************/
/**
 * Handle the message CLOSE.
 * 
 * @return Zero if success.
 *****************************************************************************/
PUBLIC int sys_close(int fd)
{
	put_inode(current->filp[fd]->fd_inode);
	current->filp[fd]->fd_inode = 0;
	current->filp[fd] = 0;
	return 0;
}



PUBLIC int sys_read(int fd, void *buf, int count)
{
	int ret = sys_rdwt(READ, fd, buf, count);
	return ret;
}

PUBLIC int sys_write(int fd, void *buf, int count)
{
	int ret = sys_rdwt(WRITE, fd, buf, count);
	return ret;
}



/* 删除一个文件 */
PUBLIC int sys_delete(char *pathname) {
    char filename[MAX_PATH];
    struct inode *dir_inode;
    int inode_nr;
    struct inode* file_inode;
    u32 region_start;
    int ret;
    struct super_block * super_b = get_super_block();

    /* 1. 获取文件 inode 号（确认文件存在） */
    inode_nr = search_file(pathname);
    if (inode_nr <= 0) {
        sys_printx("sys_delete: file not found\n");
        return -1;
    }

    /* 2. 分离路径，得到文件名和父目录 inode */
    if (strip_path(filename, pathname, &dir_inode) != 0) {
        sys_printx("sys_delete: strip_path failed\n");
        return -1;
    }

    /* 3. 在父目录中删除该目录项 */
    /* 需要遍历父目录的所有块，找到对应项并清除 inode_nr */
    int dir_ent_size = super_b->dir_ent_size;
    int block_size = super_b->block_size;
    int entries_per_block = block_size >> DIR_ENTRY_SIZE_SHIFT;
    int num_blocks = (dir_inode->i_size + block_size - 1) >> BLOCK_SIZE_SHIFT;
    int found = 0;

    for (int block_idx = 0; block_idx < num_blocks; block_idx++) {
        int phys_block = get_block_nr(dir_inode, block_idx);
        if (phys_block < 0) continue;

        u8*block_buf = (u8*) kmalloc(BLOCK_SIZE);
        if (read_through(phys_block, block_buf, block_size) != 0) {
            continue;
        }

        for (int entry_idx = 0; entry_idx < entries_per_block; entry_idx++) {
            struct dir_entry *entry = (struct dir_entry *)(block_buf + entry_idx * dir_ent_size);
            if (entry->inode_nr == inode_nr && strcmp(entry->name, filename) == 0) {
                /* 找到，清除 inode_nr */
                entry->inode_nr = 0;
                if (write_through(phys_block, block_buf, block_size) != 0) {
                    sys_printx("sys_delete: write block failed\n");
                    return -1;
                }
                found = 1;
                break;
            }
        }
        if (found) break;
    }

    if (!found) {
        sys_printx("sys_delete: directory entry not found\n");
        return -1;
    }

    /* 4. 读取文件 inode 信息（从磁盘或缓存） */
    region_start = super_b->first_data_block - super_b->nr_inode_blocks;
	file_inode = read_inode(inode_nr, region_start, super_b->inode_size);
    if (file_inode == NULL) {
        sys_printx("sys_delete: read inode failed\n");
        /* 目录项已删除，但 inode 未释放，应尽量回滚？简单返回失败 */
        return -1;
    }

    /* 5. 检查文件类型，如果是目录且非空，应拒绝删除（简化：如果是目录直接拒绝） */
    if ((file_inode->i_mode & I_TYPE_MASK) == I_DIRECTORY) {
        /* 简单实现：不允许删除目录 */
        sys_printx("sys_delete: cannot delete directory\n");
        return -1;
    }

    /* 6. 释放文件占用的所有数据块 */
    int nr_blocks = file_inode->i_nr_blocks;
    for (int i = 0; i < nr_blocks; i++) {
        int phys_block = get_block_nr(file_inode, i);
        if (phys_block > 0) {
            free_block(phys_block);
        }
    }

    /* 7. 释放 inode 号 */
    free_inode(inode_nr);

    /* 8. 从 inode 缓存中移除（减少引用计数，如果有进程打开则还需要处理） */
    /* 这里简单处理：找到缓存中的条目，如果 i_cnt > 1，说明有进程打开，我们只清除目录项，延迟释放 */
    /* 但为了简单，我们直接清除缓存条目，但如果有进程打开，会导致后续访问出错。实际应该根据引用计数 */
    /* 我们实现一个简单的处理：如果引用计数 > 1，则不清除，仅标记 inode 为“已删除”状态（比如 i_mode 特殊标记） */
    /* 但这里为了完整性，我们假设删除时没有进程打开，直接将其引用计数置0 */
    for (int i = 0; i < NR_INODE; i++) {
        if (inode_table[i].i_num == inode_nr && inode_table[i].i_cnt > 0) {
            inode_table[i].i_cnt = 0;  /* 释放槽位 */
            break;
        }
    }

    /* 9. 如果有必要，将父目录的 inode 写回（虽然大小没变，但目录项被修改） */
    /* 目录 inode 的修改已在写块时记录，但可能需要更新其修改时间等，这里省略 */
    /* 可选：同步父目录 inode 到磁盘（如果其内存副本是脏的） */
    if (write_inode_to_disk(dir_inode) != 0) {
        /* 写回失败不致命，但记录 */
        sys_printx("sys_delete: warning, failed to sync parent inode\n");
    }

    return 0;
}

/**
 * 系统调用：刷新所有文件系统缓存到磁盘
 * 包括块缓存和 inode 缓存
 */
PUBLIC void sys_flush(void)
{
    /* 1. 同步所有脏块缓存到磁盘 */
    cache_sync_all();

    /* 2. 遍历 inode 缓存表，将使用中的 inode 写回磁盘 */
    for (int i = 0; i < NR_INODE; i++) {
        struct inode *inode = &inode_table[i];
        if (inode->i_cnt > 0) {      /* 仅当 inode 被使用时才需要写回 */
            if (write_inode_to_disk(inode) != 0) {
                /* 如果某个 inode 写回失败，记录错误但继续处理其他 */
                sys_printx("\nsys_flush: failed to write inode:");
				sys_write_int_routine(inode->i_num);
            }
        }
    }
}

PRIVATE int write_block(int block_nr, void *buf, int bytes)
{
    /* 参数检查：字节数必须为正且是 4KB 的倍数 */
    if (bytes <= 0 || (bytes & 0xFFF) != 0) {
        sys_printx("\nwrite_block: bytes must be positive multiple of 4096\n");
        return -1;
    }

    /* 确保缓存已初始化 */
    if (!cache_initialized) {
        cache_init();
    }

    /* 简化处理：假设每次只写一个完整的块（4KB） */
    if (bytes != BLOCK_SIZE) {
        sys_printx("write_block: bytes must be exactly BLOCK_SIZE for caching\n");
        return -1;
    }

    /* 将数据加入缓存（标记为脏，可能触发淘汰写回） */
    cache_add(block_nr, (u8 *)buf, 1);

    return 0;
}

PRIVATE int read_block(int block_nr, void* buf, int bytes) {
    if (bytes <= 0 || (bytes & 0xFFF) != 0) {
        sys_printx("\nread_block: bytes must be positive multiple of 4096\n");
        return -1;
    }
    if (!cache_initialized) {
        cache_init();
    }
    // 查找缓存
    struct cache_block *cblk = cache_find(block_nr);
    if (cblk) {
        memcpy(buf, cblk->data, bytes);
        return 0;
    }
    // 缓存未命中，需要从磁盘读取
    // 使用异步请求队列，但需要同步等待完成
    // 我们使用一个临时缓冲区接收数据，因为 buf 可能是用户空间地址
    if (bytes > BLOCK_SIZE) return -1;
    u8*block_buf = (u8*) kmalloc(BLOCK_SIZE);
	if (read_through(block_nr, block_buf, bytes) != 0) {
		return -1;
	}

    // 复制到用户缓冲区
    memcpy(buf, block_buf, bytes);
    // 将数据加入缓存（干净块）
    cache_add(block_nr, (u8*)block_buf, 0);
    return 0;
}

PRIVATE int write_through(int block_nr, void* buf, int bytes) {
    // 参数检查：字节数必须为正且是 4KB 的倍数
    if (bytes <= 0 || (bytes & 0xFFF) != 0) {  // 4KB = 4096 = 0x1000
        sys_printx("\nwrite_through: bytes must be positive multiple of 4096\n");
        return -1;
    }

    // 构造请求
    struct hd_request req;
    req.pid = current->pid;               // 当前进程的 PID
    req.io_type = DEV_WRITE;          // 写入操作
    req.block_nr = block_nr;
    req.bytes = bytes;
    req.buf = buf;

    // 插入队列
    enqueue_hd_request(&req);
	
	block(current);
    
	return 0;   // 成功
}

PRIVATE int read_through(int block_nr, void* buf, int bytes) {
    // 参数检查：字节数必须为正且是 4KB 的倍数
    if (bytes <= 0 || (bytes & 0xFFF) != 0) {
        sys_printx("\nread_through: bytes must be positive multiple of 4096\n");
        return -1;
    }

    // 构造请求
    struct hd_request req;
    req.pid = current->pid;
    req.io_type = DEV_READ;      // 读操作
    req.block_nr = block_nr;
    req.bytes = bytes;
    req.buf = buf;

    // 插入队列
    enqueue_hd_request(&req);
	
	block(current);
    
	return 0;
}

/* 初始化缓存（在 mkfs 中调用） */
void cache_init(void) {
    if (cache_initialized) return;
    /* 分配 1MB 内存作为缓存块的数据区 */
    u32 base = kmalloc(CACHE_BLOCKS * sizeof(struct cache_block));
    cache = (struct cache_block*)base;
   
    memset(cache, 0, CACHE_BLOCKS * sizeof(struct cache_block));
    for (int i = 0; i < CACHE_BLOCKS; i++) {
        cache[i].block_nr = -1;
        cache[i].dirty = 0;
        /* 初始化 LRU 链表 */
        if (i > 0) cache[i].prev = &cache[i-1];
        if (i < CACHE_BLOCKS-1) cache[i].next = &cache[i+1];
    }
    lru_head = &cache[0];
    lru_tail = &cache[CACHE_BLOCKS-1];
    lru_head->prev = NULL;
    lru_tail->next = NULL;
    cache_initialized = 1;
}

/* 在 LRU 链表中移动块到头部（最近使用） */
static void lru_touch(struct cache_block* blk) {
    if (blk == lru_head) return;
    /* 从原位置移除 */
    if (blk->prev) blk->prev->next = blk->next;
    if (blk->next) blk->next->prev = blk->prev;
    if (blk == lru_tail) lru_tail = blk->prev;
    /* 插入头部 */
    blk->next = lru_head;
    blk->prev = NULL;
    if (lru_head) lru_head->prev = blk;
    lru_head = blk;
    if (!lru_tail) lru_tail = blk;
}

/* 查找缓存块，命中则更新 LRU */
struct cache_block* cache_find(u32 block_nr) {
    for (int i = 0; i < CACHE_BLOCKS; i++) {
        if (cache[i].block_nr == block_nr) {
            lru_touch(&cache[i]);
            cache[i].access_time = ++cache_time_counter;
            return &cache[i];
        }
    }
    return NULL;
}

/* 添加或更新缓存块（可能触发淘汰） */
void cache_add(u32 block_nr, u8* data, int dirty) {
    struct cache_block* blk = cache_find(block_nr);
    if (blk) {
        /* 已存在，更新数据 */
        memcpy(blk->data, data, BLOCK_SIZE);
        blk->dirty |= dirty;
        return;
    }
    /* 找空闲块（block_nr == -1） */
    for (int i = 0; i < CACHE_BLOCKS; i++) {
        if (cache[i].block_nr == -1) {
            blk = &cache[i];
            goto found;
        }
    }
    /* 缓存满，淘汰 LRU 尾块 */
    blk = lru_tail;
    if (blk->dirty) {
        /* 写回脏块 */
		write_through(blk->block_nr, blk->data, BLOCK_SIZE);
        blk->dirty = 0;
    }
    blk->block_nr = -1;
    /* 重新插入 LRU 头部（实际会移动） */
    lru_touch(blk);  /* 将 blk 移动到头部，但 blk 还未设置新 block_nr，暂时无影响 */
found:
    blk->block_nr = block_nr;
    memcpy(blk->data, data, BLOCK_SIZE);
    blk->dirty = dirty;
    lru_touch(blk);
    blk->access_time = ++cache_time_counter;
}

/* 淘汰一个脏块并写回（用于 flush） */
void cache_evict(void) {
    struct cache_block* blk = lru_tail;
    if (blk && blk->dirty && blk->block_nr != -1) {
        write_through(blk->block_nr, blk->data, BLOCK_SIZE);
		blk->dirty = 0;
    }
}

/* 同步所有脏块到硬盘 */
void cache_sync_all(void) {
    for (int i = 0; i < CACHE_BLOCKS; i++) {
        if (cache[i].dirty && cache[i].block_nr != -1) {
            write_through(cache[i].block_nr, cache[i].data, BLOCK_SIZE);
			cache[i].dirty = 0;
        }
    }
}

/*****************************************************************************
 *                                mkfs
 *****************************************************************************/
/**
 * <Ring 1> Make a available Orange'S FS in the disk. It will
 *          - Write a super block to sector 1.
 *          - Create three special files: dev_tty0, dev_tty1, dev_tty2
 *          - Create the inode map
 *          - Create the sector map
 *          - Create the inodes of the files
 *          - Create `/', the root directory
 *****************************************************************************/
PRIVATE void mkfs()
{
    sys_printx("\nmkfs");
	int i, j;
	/* get the geometry of ROOTDEV */
	struct part_info geo;

	sys_hd_ioctl(MINOR(ROOT_DEV), DIOCTL_GET_GEO, &geo);
	
	//sys_printx("\ndev size: ");
	//sys_write_int_routine(geo.size);
	//sys_printx(" sectors\n");
	//return; 
	
	/************************/
	/*      super block     */
	/************************/
	struct super_block s_b;
	s_b.magic	  = MAGIC_V1;
	s_b.nr_inodes	  = BITS_PER_BLOCK;
	s_b.block_size = BLOCK_SIZE;
	s_b.nr_inode_blocks = (s_b.nr_inodes * INODE_SIZE) >> BLOCK_SIZE_SHIFT;
	s_b.nr_blocks	  = geo.size >> 3; // div SECTORS_PER_BLOCK
	s_b.nr_imap_blocks  = 1;
	s_b.nr_bmap_blocks  = (s_b.nr_blocks >> 15) + 1; // div BITS_PER_BLOCK
	s_b.first_data_block	  = 1 + 1 +   /* boot block & super block */
		s_b.nr_imap_blocks + s_b.nr_bmap_blocks + s_b.nr_inode_blocks;
	s_b.root_inode	  = ROOT_INODE;
	s_b.inode_size	  = INODE_SIZE;


	struct inode x;
	s_b.inode_isize_off= (int)&x.i_size - (int)&x;
	s_b.inode_start_off= (int)&x.i_start_block - (int)&x;
	
	s_b.dir_ent_size	  = DIR_ENTRY_SIZE;
	struct dir_entry de;
	s_b.dir_ent_inode_off = (int)&de.inode_nr - (int)&de;
	s_b.dir_ent_fname_off = (int)&de.name - (int)&de;
    
    u8*block_buf = (u8*) kmalloc(BLOCK_SIZE);
    memset(block_buf, 0x90, BLOCK_SIZE);
	memcpy(block_buf, &s_b, SUPER_BLOCK_SIZE);

	//return;
	/* write the super block */
	write_through(1, block_buf, BLOCK_SIZE);

	/************************/
	/*       inode map      */
	/************************/
    block_buf = (u8*) kmalloc(BLOCK_SIZE);
	memset(block_buf, 0, BLOCK_SIZE);
	for (i = 0; i < 2; i++)
		block_buf[0] |= 1 << i; // bit 0 : reserved, bit 1 : the first inode "/"
	//WR_SECT(ROOT_DEV, 2);
	write_through(2, block_buf, BLOCK_SIZE);
	//return;

	/************************/
	/*      block map      */
	/************************/
    block_buf = (u8*) kmalloc(BLOCK_SIZE);
	memset(block_buf, 0, BLOCK_SIZE);
	//return; 
	int nr_blocks = 1 + 1;
	/*             		~~~~~~~~~~~~~~~~~~~|~   |
	 *                  	               |    `--- bit 0 is reserved
	 *                       	         `-------- for `/'
	 */
	for (j = 0; j < nr_blocks; j++)
		block_buf[i] |= (1 << j);
	write_through(2 + s_b.nr_imap_blocks, block_buf, BLOCK_SIZE);

	/* zeromemory the rest block-map */
    block_buf = (u8*) kmalloc(BLOCK_SIZE);
	memset(block_buf, 0, BLOCK_SIZE);
	for (i = 1; i < s_b.nr_bmap_blocks; i++){
		write_through(2 + s_b.nr_imap_blocks + i, block_buf, BLOCK_SIZE);
	}

	/************************/
	/*       inodes         */
	/************************/
	/* inode of `/' */
    block_buf = (u8*) kmalloc(BLOCK_SIZE);
	memset(block_buf, 0, BLOCK_SIZE);
	struct inode * pi = (struct inode*)block_buf;
	pi->i_mode = I_DIRECTORY;
	pi->i_size = DIR_ENTRY_SIZE * 2; /* . and .. */
	pi->i_start_block = s_b.first_data_block;
	pi->i_nr_blocks = 1;
	pi->i_direct[0] = s_b.first_data_block;
	for (int i = 1; i < INODE_DIRECT_COUNT; i++) {
    	pi->i_direct[i] = 0;           /* 其余直接块清零 */
	}
	pi->i_indirect = 0;
	write_through(2 + s_b.nr_imap_blocks + s_b.nr_bmap_blocks, block_buf, BLOCK_SIZE);
	//return; 
	
	/************************/
	/*          `/'         */
	/************************/
    block_buf = (u8*) kmalloc(BLOCK_SIZE);
	memset(block_buf, 0, BLOCK_SIZE);
	struct dir_entry * pde = (struct dir_entry *)block_buf;
	pde->inode_nr = 1;
	strcpy(pde->name, ".");
	pde++;
	pde->inode_nr = 1;
	strcpy(pde->name, "..");
	write_through(s_b.first_data_block, block_buf, BLOCK_SIZE);
	return;
}


/*****************************************************************************
 *                                create_file
 *****************************************************************************/
/**
 * Create a file and return it's inode ptr.
 *
 * @param[in] path   The full path of the new file
 * @param[in] flags  Attributes of the new file (e.g., O_CREAT, mode bits)
 *
 * @return           Ptr to i-node of the new file if successful, otherwise NULL.
 *
 * @see open()
 * @see do_open()
 *****************************************************************************/
PRIVATE struct inode *create_file(char *path, int flags)
{
    //sys_printx("\ncreate_file is called\n");

    /* 1. Split path into filename and parent directory inode */
    char filename[MAX_PATH];
    struct inode *dir_inode;
    if (strip_path(filename, path, &dir_inode) != 0) {
        sys_printx("strip_path failed\n");
        return NULL;
    }

    //sys_printx("\nfilename is ");
    //sys_printx(filename);
    //sys_printx("\ndir_inode size is ");
    //sys_write_int_routine(dir_inode->i_size);

    /* 2. Allocate a new inode number from the inode bitmap */
    int inode_nr = alloc_inode();
    //sys_printx("\ninode_nr is ");
    //sys_write_int_routine(inode_nr);
    if (inode_nr < 0) {
        sys_printx("alloc_inode failed\n");
        return NULL;
    }

    /* 3. Allocate data blocks for the file (optional, here we allocate 0 blocks) */
    /*    For a new file, we may not allocate any data blocks until write. */
    /*    The original code allocated NR_DEFAULT_FILE_SECTS; we keep it simple. */

    /* 4. Get an inode slot from the inode cache */
    struct inode *newino = get_inode_slot();   /* assume this finds a free slot */
    if (!newino) {
        sys_printx("get_inode_slot failed\n");
        /* Free the allocated inode number? */
        return NULL;
    }

    /* 5. Initialize the inode */
    memset(newino, 0, sizeof(struct inode));
    newino->i_mode = I_REGULAR | (flags & 0777);  /* set file type and permissions */
    newino->i_size = 0;
    newino->i_start_block = 0;          /* no blocks allocated yet */
    newino->i_nr_blocks = 0;
    for (int i = 0; i < INODE_DIRECT_COUNT; i++)
        newino->i_direct[i] = 0;
    newino->i_indirect = 0;
    newino->i_cnt = 1;                  /* initial reference count */
    newino->i_num = inode_nr;

    //sys_printx("\ni_mode is ");
    //sys_write_int_routine(newino->i_mode);

    /* 6. Write the inode back to disk */
    if (write_inode_to_disk(newino) != 0) {
        sys_printx("write_inode_to_disk failed\n");
        /* free the allocated inode number? */
        return NULL;
    }

    /* 7. Add directory entry in the parent directory */
    if (add_dir_entry(dir_inode, filename, inode_nr) != 0) {
        sys_printx("add_dir_entry failed\n");
        /* Remove the inode? */
        return NULL;
    }

    return newino;
}

/**
 * 从全局inode位图中分配一个空闲inode号
 * @return 成功返回inode号（从1开始），失败返回-1
 */
int alloc_inode(void)
{
    struct super_block * super_b = get_super_block();
    int inode_count = super_b->nr_inodes;
    /* inode位图起始块号：超级块之后紧接着inode位图 */
    int bitmap_start = SUPER_BLOCK_NR + 1;
    int bitmap_blocks = super_b->nr_imap_blocks;
    int inode_nr = -1;

    for (int block_idx = 0; block_idx < bitmap_blocks; block_idx++) {
        int block_nr = bitmap_start + block_idx;
        u8*block_buf = (u8*) kmalloc(BLOCK_SIZE);
        if (read_through(block_nr, block_buf, BLOCK_SIZE) != 0) {
            return -1;
        }

        /* 遍历该位图块的每一个字节 */
        for (int byte = 0; byte < BLOCK_SIZE; byte++) {
            /* 如果当前字节全满（0xFF），则跳过 */
            if (block_buf[byte] == 0xFF) {
                continue;
            }

            /* 查找该字节中的空闲位 */
            for (int bit = 0; bit < 8; bit++) {
                if (!(block_buf[byte] & (1 << bit))) {
                    /* 计算全局位索引（从0开始） */
                    int global_bit = block_idx * (BLOCK_SIZE * 8) + byte * 8 + bit;
                    if (global_bit >= inode_count) {
                        break;  /* 超出可用inode范围 */
                    }
                    inode_nr = global_bit + 1;  /* inode号从1开始 */

                    /* 设置该位为1 */
                    block_buf[byte] |= (1 << bit);
                    if (write_through(block_nr, block_buf, BLOCK_SIZE) != 0) {
                        return -1;
                    }
                    return inode_nr;
                }
            }
        }
    }

    /* 没有空闲inode */
    return -1;
}

/**
 * 从全局块位图中分配一个空闲数据块
 * @return 成功返回物理块号（从0开始），失败返回-1
 */
int alloc_block(void)
{
    /* 块位图起始块号：超级块(1) + inode位图块数 + 1? 这里假设inode位图在超级块后 */
    struct super_block * super_b = get_super_block();
    int bitmap_start = SUPER_BLOCK_NR + 1 + super_b->nr_imap_blocks;
    int bitmap_blocks = super_b->nr_bmap_blocks;

    for (int block_idx = 0; block_idx < bitmap_blocks; block_idx++) {
        int block_nr = bitmap_start + block_idx;
        u8*block_buf = (u8*) kmalloc(BLOCK_SIZE);
        if (read_through(block_nr, block_buf, BLOCK_SIZE) != 0)
            return -1;

        for (int byte = 0; byte < BLOCK_SIZE; byte++) {
            if (block_buf[byte] == 0xFF)
                continue;
            for (int bit = 0; bit < 8; bit++) {
                if (!(block_buf[byte] & (1 << bit))) {
                    int global_bit = block_idx * (BLOCK_SIZE * 8) + byte * 8 + bit;
                    if (global_bit >= super_b->nr_blocks)
                        break;  /* 超出总块数，该位图块剩余位无效 */

                    int alloc_block_nr = global_bit;
                    /* 确保分配的块是数据块区域 */
                    if (alloc_block_nr <= super_b->first_data_block)
                        continue;   /* 跳过系统保留块（超级块、位图、inode表等） */

                    /* 设置位图 */
                    block_buf[byte] |= (1 << bit);
                    if (write_through(block_nr, block_buf, BLOCK_SIZE) != 0)
                        return -1;

                    return alloc_block_nr;
                }
            }
        }
    }
    return -1;  /* 无空闲块 */
}

static int alloc_block_for_inode(struct inode *inode, int logical)
{
    struct super_block *super_b = get_super_block();
    int block_size = super_b->block_size;
    int ptrs_per_block = block_size / sizeof(u32);  /* 每个间接块可存储的指针数 */

    /* 分配一个新的物理块 */
    int new_block = alloc_block();
    if (new_block < 0) return -1;

    /* 直接块 */
    if (logical < INODE_DIRECT_COUNT) {
        inode->i_direct[logical] = new_block;
        inode->i_nr_blocks++;
        return new_block;
    }

    /* 间接块 */
    int indirect_index = logical - INODE_DIRECT_COUNT;
    if (indirect_index >= ptrs_per_block) {
        /* 超出间接块范围，暂不支持二级间接块 */
        sys_printx("alloc_block_for_inode: file too large, indirect block overflow\n");
        return -1;
    }

    /* 确保间接块已分配 */
    if (inode->i_indirect == 0) {
        int indirect_block = alloc_block();
        if (indirect_block < 0) {
            sys_printx("alloc_block_for_inode: alloc indirect block failed\n");
            return -1;
        }
        inode->i_indirect = indirect_block;
        /* 初始化间接块全零 */
        char *zero_buf = (char*)kmalloc(block_size);
        if (!zero_buf) return -1;
        memset(zero_buf, 0, block_size);
        if (write_through(indirect_block, zero_buf, block_size) != 0) {
            return -1;
        }
    }

    /* 读取间接块 */
    u32 *indirect = (u32*)kmalloc(block_size);
    if (!indirect) return -1;
    if (read_through(inode->i_indirect, indirect, block_size) != 0) {
        return -1;
    }

    /* 设置对应指针 */
    indirect[indirect_index] = new_block;
    if (write_through(inode->i_indirect, indirect, block_size) != 0) {
        return -1;
    }

    inode->i_nr_blocks++;
    return new_block;
}


/* 辅助函数：释放一个数据块 */
static int free_block(int block_nr) {
    /* 块位图起始块号：超级块(1) + inode位图块数 + 1? 根据之前 alloc_block 的实现 */
    struct super_block * super_b = get_super_block();
    int bitmap_start = SUPER_BLOCK_NR + 1 + super_b->nr_imap_blocks;
    int bitmap_blocks = super_b->nr_bmap_blocks;

    /* 计算位图块索引和位偏移 */
    int global_bit = block_nr;   /* 直接使用块号作为位索引（从0开始） */
    int bitmap_block_idx = global_bit >> 15;
    int offset = global_bit % (BLOCK_SIZE * 8);
    int byte = offset >> 3;
    int bit = offset % 8;

    if (bitmap_block_idx >= bitmap_blocks) {
        sys_printx("free_block: block_nr out of range\n");
        return -1;
    }

    int block_nr_bitmap = bitmap_start + bitmap_block_idx;
    u8*block_buf = (u8*) kmalloc(BLOCK_SIZE);
    if (read_through(block_nr_bitmap, block_buf, BLOCK_SIZE) != 0) {
        sys_printx("free_block: read bitmap failed\n");
        return -1;
    }

    /* 清除位 */
    block_buf[byte] &= ~(1 << bit);
    if (write_through(block_nr_bitmap, block_buf, BLOCK_SIZE) != 0) {
        sys_printx("free_block: write bitmap failed\n");
        return -1;
    }
    return 0;
}

/* 辅助函数：释放一个 inode 号 */
static int free_inode(int inode_nr) {
    struct super_block * super_b = get_super_block();
    int bitmap_start = SUPER_BLOCK_NR + 1;
    int bitmap_blocks = super_b->nr_imap_blocks;
    int global_bit = inode_nr - 1;   /* inode 号从1开始，位索引从0开始 */
    int bitmap_block_idx = global_bit >> 15;
    int offset = global_bit % (BLOCK_SIZE * 8);
    int byte = offset >> 3;
    int bit = offset % 8;

    if (bitmap_block_idx >= bitmap_blocks) {
        sys_printx("free_inode: inode_nr out of range\n");
        return -1;
    }

    int block_nr = bitmap_start + bitmap_block_idx;
    u8*block_buf = (u8*) kmalloc(BLOCK_SIZE);
    if (read_through(block_nr, block_buf, BLOCK_SIZE) != 0) {
        sys_printx("free_inode: read bitmap failed\n");
        return -1;
    }

    block_buf[byte] &= ~(1 << bit);
    if (write_through(block_nr, block_buf, BLOCK_SIZE) != 0) {
        sys_printx("free_inode: write bitmap failed\n");
        return -1;
    }
    return 0;
}

/**
 * 将内存中的 inode 写回磁盘。
 * 根据 inode 号计算出其在 inode 区域中的位置，然后调用 write_through 写入。
 */
static int write_inode_to_disk(struct inode *inode)
{
    /* 计算 inode 区域起始块号和偏移，然后写入 */
    struct super_block * super_b = get_super_block();
    u32 inode_region_start = super_b->first_data_block - super_b->nr_inode_blocks;
    int idx = inode->i_num - 1;
    int block_offset = idx * super_b->inode_size;
    int block_nr = inode_region_start + (block_offset >> BLOCK_SIZE_SHIFT);
    int offset = block_offset % BLOCK_SIZE;
    //sys_printx("\noffset is");
    //sys_write_int_routine(offset);

    /* 先读取整个块，因为可能只修改部分数据 */
    u8*block_buf = (u8*) kmalloc(BLOCK_SIZE);
    if (read_through(block_nr, block_buf, BLOCK_SIZE) != 0)
        return -1;
    memcpy(block_buf + offset, inode, super_b->inode_size);
    if (write_through(block_nr, block_buf, BLOCK_SIZE) != 0)
        return -1;
    return 0;
}

/**
 * 从 inode 缓存中获取一个空闲槽位。
 * 遍历 inode_table，找到一个 i_cnt == 0 的槽位。
 */
static struct inode *get_inode_slot(void)
{
    for (int i = 0; i < NR_INODE; i++) {
        if (inode_table[i].i_cnt == 0) {
            return &inode_table[i];
        }
    }
    return NULL;
}

/**
 * 在目录中添加一个新的目录项
 * @param dir       目录的 inode
 * @param name      文件名（不能包含路径分隔符）
 * @param inode_nr  文件的 inode 号
 * @return 0 成功，-1 失败
 */
static int add_dir_entry(struct inode *dir, char *name, int inode_nr)
{
    struct super_block * super_b = get_super_block();
    int dir_ent_size = super_b->dir_ent_size;
    int block_size = super_b->block_size;
    int entries_per_block = block_size >> DIR_ENTRY_SIZE_SHIFT;

    /* 检查文件名长度 */
    int name_len = strlen(name);
    if (name_len >= MAX_FILENAME_LEN) {
        sys_printx("filename too long\n");
        return -1;
    }

    /* 遍历目录的所有数据块，查找空闲目录项 */
    int num_blocks = (dir->i_size + block_size - 1) >> BLOCK_SIZE_SHIFT;
    for (int block_idx = 0; block_idx < num_blocks; block_idx++) {
        int phys_block = get_block_nr(dir, block_idx);
        if (phys_block < 0) {
            sys_printx("get_block_nr failed\n");
            return -1;
        }
        u8*block_buf = (u8*) kmalloc(BLOCK_SIZE);
        if (read_through(phys_block, block_buf, block_size) != 0) {
            sys_printx("read_block failed in add_dir_entry\n");
            return -1;
        }

        /* 遍历块内的所有目录项 */
        for (int entry_idx = 0; entry_idx < entries_per_block; entry_idx++) {
            struct dir_entry *entry = (struct dir_entry *)(block_buf + entry_idx * dir_ent_size);
            //sys_printx("\nexisting dir entry ");
            //sys_printx(entry->name);
            //sys_printx("  ");
            //sys_write_int_routine(entry->inode_nr);
            if (entry->inode_nr == 0) {
                /* 找到空闲项，写入新条目 */
                entry->inode_nr = inode_nr;
                strcpy(entry->name, name);
                entry->name[MAX_FILENAME_LEN - 1] = '\0';
                //sys_printx("\nadd new dir entry ");
                //sys_printx(entry->name);
                //sys_printx("  ");
                //sys_write_int_routine(entry->inode_nr);
                if (write_through(phys_block, block_buf, block_size) != 0) {
                    sys_printx("write_through failed in add_dir_entry\n");
                    return -1;
                }
                dir->i_size += DIR_ENTRY_SIZE;
                if (write_inode_to_disk(dir) != 0) {
                    sys_printx("\nwrite_inode_to_disk failed in add_dir_entry\n");
                    return -1;
                }
                return 0;
            }
        }
    }

    /* 没有空闲项，需要扩展目录，分配新块 */
    int new_block = alloc_block();
    if (new_block < 0) {
        sys_printx("alloc_block failed for directory\n");
        return -1;
    }

    /* 将新块添加到目录 inode 中 */
    int new_block_idx = num_blocks;  /* 新的逻辑块索引 */
    /* 根据块索引设置 inode 的指针（直接/间接） */
    if (new_block_idx < INODE_DIRECT_COUNT) {
        dir->i_direct[new_block_idx] = new_block;
    } else if (new_block_idx < INODE_DIRECT_COUNT + (block_size >> 2) /* div sizeof(u32)*/) {
        /* 使用间接块 */
        if (dir->i_indirect == 0) {
            /* 需要分配间接块 */
            int indirect_block = alloc_block();
            if (indirect_block < 0) {
                /* 回滚已分配的数据块？ */
                /* 简单起见，释放 new_block 并返回失败 */
                /* 注意：这里没有释放位图的函数，需实现 */
                return -1;
            }
            dir->i_indirect = indirect_block;
            /* 初始化间接块为零 */
            u8*block_buf2 = (u8*) kmalloc(BLOCK_SIZE);
            memset(block_buf2, 0, block_size);
            if (write_through(indirect_block, block_buf2, block_size) != 0) {
                return -1;
            }
        }
        u8*block_buf3 = (u8*) kmalloc(BLOCK_SIZE);
        /* 读取间接块 */
        if (read_through(dir->i_indirect, block_buf3, block_size) != 0) {
            return -1;
        }
        u32 *indirect = (u32 *)block_buf3;
        int index_in_indirect = new_block_idx - INODE_DIRECT_COUNT;
        indirect[index_in_indirect] = new_block;
        if (write_through(dir->i_indirect, block_buf3, block_size) != 0) {
            return -1;
        }
    } else {
        /* 不支持二级间接块 */
        sys_printx("directory too large, indirect block not supported\n");
        return -1;
    }

    dir->i_nr_blocks++;   /* 增加块计数 */
    dir->i_size += block_size;

    u8*block_buf4 = (u8*) kmalloc(BLOCK_SIZE);
    /* 初始化新块的所有目录项为0 */
    memset(block_buf4, 0, block_size);
    if (write_through(new_block, block_buf4, block_size) != 0) {
        /* 回滚 inode 更改？简化处理，返回失败 */
        return -1;
    }

    /* 在新块的第一个目录项中写入新条目 */
    u8*block_buf5 = (u8*) kmalloc(BLOCK_SIZE);
    struct dir_entry *new_entry = (struct dir_entry *)block_buf5;
    new_entry->inode_nr = inode_nr;
    strcpy(new_entry->name, name);
    new_entry->name[MAX_FILENAME_LEN - 1] = '\0';
    if (write_through(new_block, block_buf5, block_size) != 0) {
        return -1;
    }

    /* 更新目录 inode 到磁盘 */
    if (write_inode_to_disk(dir) != 0) {
        sys_printx("write_inode_to_disk failed for directory\n");
        return -1;
    }

    return 0;
}

/*****************************************************************************
 *                                read_super_block
 *****************************************************************************/
/**
 * <Ring 1> Read super block from the given device then write it into a free
 *          super_block[] slot.
 * 
 *****************************************************************************/
PRIVATE void read_super_block()
{
	u8*block_buf = (u8*) kmalloc(BLOCK_SIZE);
    int ret = read_through(SUPER_BLOCK_NR, block_buf, BLOCK_SIZE);
	memcpy(sb, block_buf, SUPER_BLOCK_SIZE);
}

/*****************************************************************************
 *                                get_super_block
 *****************************************************************************/
/**
 * <Ring 1> Get the super block from super_block[].
 * 
 * 
 * @return Super block ptr.
 *****************************************************************************/
PUBLIC struct super_block * get_super_block()
{
	struct super_block * super_b = sb;
    return super_b;
}

PUBLIC int search_file(char *path) {
    char path_copy[MAX_PATH_LEN];
    char *token, *saveptr;
    int cur_inode;
    struct inode *dir_inode;
    struct super_block *super_b = get_super_block();
    u32 inode_region_start = super_b->first_data_block - super_b->nr_inode_blocks;

    cur_inode = super_b->root_inode;

    strcpy(path_copy, path);
    path_copy[MAX_PATH_LEN - 1] = '\0';
    //sys_printx("\npath_copy is ");
    //sys_printx(path_copy);

    char *p = path_copy;
    if (*p == '/') p++;
    if (*p == '\0') return cur_inode;   // 根目录

    token = strtok_r(p, "/", &saveptr);
    //sys_printx("\ntoken is ");
    //sys_printx(token);

    while (token != NULL) {
        dir_inode = read_inode(cur_inode, inode_region_start, super_b->inode_size);
        if (dir_inode == NULL) return -1;   // 中间目录无法读取，路径无效

        int next_inode = find_in_dir(dir_inode, token, super_b->dir_ent_size);
        if (next_inode == -1) {
            // 如果当前 token 是路径的最后一个分量，说明文件不存在，返回 0
            char *next_token = strtok_r(NULL, "/", &saveptr);
            if (next_token == NULL) return 0;   // 文件不存在
            else return -1;                     // 中间目录缺失
        }
        cur_inode = next_inode;
        token = strtok_r(NULL, "/", &saveptr);
        //sys_printx("\ntoken is ");
        //sys_printx(token);
    }
    return cur_inode;
}

static struct inode *read_inode(int inode_nr, u32 inode_region_start, u32 inode_size)
{
    if (inode_nr < 1)
        return NULL;

    /* 1. 在缓存中查找已有的 inode */
    for (int i = 0; i < NR_INODE; i++) {
        if (inode_table[i].i_num == inode_nr && inode_table[i].i_cnt > 0) {
            inode_table[i].i_cnt++;  /* 增加引用计数 */
            return &inode_table[i];
        }
    }

    /* 2. 未找到，寻找空闲槽位 */
    int free_slot = -1;
    for (int i = 0; i < NR_INODE; i++) {
        if (inode_table[i].i_cnt == 0) {
            free_slot = i;
            break;
        }
    }
    if (free_slot == -1) {
        sys_printx("inode cache full\n");
        return NULL;
    }

    /* 3. 从磁盘读取 inode 到空闲槽位 */
    int idx = inode_nr - 1;                         /* 从 0 开始的索引 */
    int block_offset = idx * inode_size;            /* 在 inode 区域内的字节偏移 */
    int block_nr = inode_region_start + (block_offset >> BLOCK_SIZE_SHIFT);
    int offset = block_offset % BLOCK_SIZE;
    u8*block_buf = (u8*) kmalloc(BLOCK_SIZE);
    if (read_through(block_nr, block_buf, BLOCK_SIZE) != 0)
        return NULL;

    struct inode *new_inode = &inode_table[free_slot];
    memcpy(new_inode, block_buf + offset, inode_size);
    new_inode->i_num = inode_nr;
    new_inode->i_cnt = 1;                           /* 初始引用计数为 1 */

    return new_inode;
}

PUBLIC void put_inode(struct inode * pinode)
{
	pinode->i_cnt--;
}

/**
 * 获取文件第block_index个数据块的块号（支持直接块和间接块）
 * @param inode        inode指针
 * @param block_index  块索引（从0开始）
 * @return 块号，失败返回-1
 */
static int get_block_nr(struct inode *inode, int block_index) {
    if (block_index < INODE_DIRECT_COUNT) {
        return inode->i_direct[block_index];
    }

    /* 处理间接块 */
    int indirect_index = block_index - INODE_DIRECT_COUNT;
    int pointers_per_block = BLOCK_SIZE >> 2;
    if (indirect_index < pointers_per_block) {
        if (inode->i_indirect == 0) return -1;   /* 间接块未分配 */
        u8*block_buf = (u8*) kmalloc(BLOCK_SIZE);
        if (read_through(inode->i_indirect, block_buf, BLOCK_SIZE) != 0) {
            return -1;
        }
        u32 *ptr = (u32 *)block_buf;
        return ptr[indirect_index];
    }

    /* 暂不支持二级间接块 */
    return -1;
}

/**
 * 在目录inode中查找指定名称的文件
 * @param dir_inode    目录的inode
 * @param name         要查找的文件名
 * @param dir_ent_size 目录项大小
 * @return 找到则返回文件的inode号，否则返回-1
 */
static int find_in_dir(struct inode *dir_inode, char *name,
                       u32 dir_ent_size) {
    u32 size = dir_inode->i_size;
    int num_blocks = (size + BLOCK_SIZE - 1) >> BLOCK_SIZE_SHIFT;
    int total_entries = size >> DIR_ENTRY_SIZE_SHIFT;
	int entry_index = 0;

    for (int i = 0; i < num_blocks; i++) {
        int block_nr = get_block_nr(dir_inode, i);
        if (block_nr < 0) return -1;
        u8*block_buf = (u8*) kmalloc(BLOCK_SIZE);
        if (read_through(block_nr, block_buf, BLOCK_SIZE) != 0) {
            return -1;
        }

        int entries_in_block = BLOCK_SIZE >> 4;
        for (int j = 0; j < entries_in_block && entry_index < total_entries;
             j++, entry_index++) {
            struct dir_entry *entry = (struct dir_entry *)(block_buf + j * dir_ent_size);
            /* 跳过空目录项（inode_nr == 0） */
            if (entry->inode_nr == 0) continue;
            //sys_printx("\nentry->name is");
            //sys_printx(entry->name);
            if (strcmp(entry->name, name) == 0) {
                return entry->inode_nr;
            }
        }
    }
    return -1;
}

/**
 * 从完整路径中分离出目录路径和文件名，并返回目录的inode指针
 * @param filename  输出缓冲区，存放文件名
 * @param path      输入完整路径
 * @param dir_inode 输出参数，指向目录的inode指针
 * @return 0 成功，-1 失败（如路径无效、目录不存在等）
 */
int strip_path(char *filename, const char *path, struct inode **dir_inode)
{
    char path_copy[MAX_PATH];
    char *last_slash;
    char *dir_path;
    int dir_inode_nr;
    int inode_region_start;
    struct inode *tmp_inode;
    int i;
    struct super_block * super_b = get_super_block();

    /* 检查参数 */
    if (filename == NULL || path == NULL || dir_inode == NULL)
        return -1;

    /* 复制路径，避免修改原字符串 */
    strcpy(path_copy, path);
    path_copy[MAX_PATH - 1] = '\0';

    /* 查找最后一个 '/' */
    last_slash = strrchr(path_copy, '/');
    if (last_slash == NULL) {
        /* 没有 '/'，说明路径就是文件名，目录为根目录 */
        strcpy(filename, path_copy);
        filename[MAX_FILENAME_LEN - 1] = '\0';
        dir_path = NULL;  /* 根目录 */
    } else {
        /* 分割出目录路径和文件名 */
        *last_slash = '\0';   /* 将路径分成两部分 */
        if (last_slash == path_copy) {
            /* 路径以 '/' 开头且是唯一 '/'，例如 "/file" */
            dir_path = "/";   /* 根目录 */
        } else {
            dir_path = path_copy;   /* 目录路径 */
        }
        strcpy(filename, last_slash + 1);
        filename[MAX_FILENAME_LEN - 1] = '\0';
    }

    /* 获取目录的 inode 号 */
    if (dir_path == NULL || strcmp(dir_path, "/") == 0) {
        /* 根目录 */
        dir_inode_nr = super_b->root_inode;
    } else {
        /* 使用 search_file 查找目录的 inode 号 */
        dir_inode_nr = search_file(dir_path);
        if (dir_inode_nr < 0) {
            sys_printx("strip_path: directory not found\n");
            return -1;
        }
    }

    /* 读取目录的 inode 到缓存 */
    inode_region_start = super_b->first_data_block - super_b->nr_inode_blocks;
    for (i = 0; i < NR_INODE; i++) {
        if (inode_table[i].i_num == dir_inode_nr && inode_table[i].i_cnt > 0) {
            *dir_inode = &inode_table[i];
            (*dir_inode)->i_cnt++;
            return 0;
        }
    }
    /* 不在缓存中，需要从磁盘读取 */
    for (i = 0; i < NR_INODE; i++) {
        if (inode_table[i].i_cnt == 0) {
			tmp_inode = &inode_table[i];
			tmp_inode = read_inode(dir_inode_nr, inode_region_start, super_b->inode_size);
            if (tmp_inode == NULL) {
                return -1;
            }
            *dir_inode = tmp_inode;
            return 0;
        }
    }
    /* 缓存满 */
    sys_printx("strip_path: inode cache full\n");
    return -1;
}

/**
 * 字符串分割函数
 * @param str     要分割的字符串，首次调用时传入，后续调用传入NULL
 * @param delim   分隔符集合（字符串）
 * @param saveptr 指向保存状态的指针
 * @return 下一个token，无token时返回NULL
 */
char *strtok_r(char *str, const char *delim, char **saveptr) {
    char *token_start;
    const char *d;

    /* 参数检查 */
    if (delim == NULL || saveptr == NULL)
        return NULL;

    /* 如果str非空，则开始新的解析；否则使用保存的位置 */
    if (str != NULL)
        *saveptr = str;
    else if (*saveptr == NULL)
        return NULL;   /* 已经解析完毕 */

    /* 跳过开头的分隔符 */
    while (**saveptr) {
        int is_delim = 0;
        for (d = delim; *d; d++) {
            if (**saveptr == *d) {
                is_delim = 1;
                break;
            }
        }
        if (!is_delim)
            break;
        (*saveptr)++;
    }

    /* 如果已经到达字符串末尾，则没有token */
    if (**saveptr == '\0') {
        *saveptr = NULL;
        return NULL;
    }

    /* 标记token起始位置 */
    token_start = *saveptr;

    /* 扫描直到遇到分隔符或结尾 */
    while (**saveptr) {
        int is_delim = 0;
        for (d = delim; *d; d++) {
            if (**saveptr == *d) {
                is_delim = 1;
                break;
            }
        }
        if (is_delim) {
            **saveptr = '\0';     /* 替换分隔符为字符串结束符 */
            (*saveptr)++;         /* 移动指针到下一个字符 */
            return token_start;
        }
        (*saveptr)++;
    }

    /* 到达结尾，没有遇到分隔符 */
    *saveptr = NULL;
    return token_start;
}

char *strrchr(const char *s, int c) {
    char ch = (char)c;
    char *last = NULL;
    while (*s) {
        if (*s == ch)
            last = (char *)s;
        s++;
    }
    return last;
}


/*****************************************************************************
 *                                dump_fs
 *****************************************************************************/
/**
 * 打印当前文件系统的所有信息（超级块、位图、inode表、目录树）
 * 用于调试和查看文件系统状态。
 */
PUBLIC void dump_fs(void)
{
    struct super_block * super_b = get_super_block();
    int i, j;
    u32 total_inodes = super_b->nr_inodes;
    u32 total_blocks = super_b->nr_blocks;
    u32 used_inodes = 0;
    u32 used_blocks = 0;

    sys_printx("\n========== File System Dump ==========\n");

    /* 1. 超级块信息 */
    sys_printx("Super Block:\n");
    sys_printx("  magic:"); sys_write_int_routine(super_b->magic); sys_printx("\n");
    sys_printx("  nr_inodes: "); sys_write_int_routine(super_b->nr_inodes); sys_printx("\n");
    sys_printx("  nr_blocks: "); sys_write_int_routine(super_b->nr_blocks); sys_printx("\n");
    sys_printx("  nr_imap_blocks: "); sys_write_int_routine(super_b->nr_imap_blocks); sys_printx("\n");
    sys_printx("  nr_bmap_blocks: "); sys_write_int_routine(super_b->nr_bmap_blocks); sys_printx("\n");
    sys_printx("  first_data_block: "); sys_write_int_routine(super_b->first_data_block); sys_printx("\n");
    sys_printx("  root_inode: "); sys_write_int_routine(super_b->root_inode); sys_printx("\n");
    sys_printx("  block_size: "); sys_write_int_routine(super_b->block_size); sys_printx("\n");
    sys_printx("  inode_size: "); sys_write_int_routine(super_b->inode_size); sys_printx("\n");
    sys_printx("  dir_ent_size: "); sys_write_int_routine(super_b->dir_ent_size); sys_printx("\n");

    /* 2. 统计 inode 位图使用情况 */
    int bitmap_start = SUPER_BLOCK_NR + 1;
    int bitmap_blocks = super_b->nr_imap_blocks;
    sys_printx("\nInode bitmap:\n");
    for (i = 0; i < bitmap_blocks; i++) {
        int block_nr = bitmap_start + i;
        u8*block_buf = (u8*) kmalloc(BLOCK_SIZE);
        if (read_through(block_nr, block_buf, BLOCK_SIZE) != 0) {
            sys_printx("  failed to read inode bitmap block ");
            sys_write_int_routine(block_nr); sys_printx("\n");
            continue;
        }
        for (j = 0; j < BLOCK_SIZE; j++) {
            u8 byte = block_buf[j];
            if (byte == 0) continue;
            for (int bit = 0; bit < 8; bit++) {
                if (byte & (1 << bit)) {
                    int global_bit = i * (BLOCK_SIZE * 8) + j * 8 + bit;
                    if (global_bit < total_inodes) {
                        used_inodes++;
                    }
                }
            }
        }
    }
    sys_printx("  used inodes: "); sys_write_int_routine(used_inodes);
    sys_printx(" / "); sys_write_int_routine(total_inodes); sys_printx("\n");

    /* 3. 统计块位图使用情况 */
    bitmap_start = SUPER_BLOCK_NR + 1 + super_b->nr_imap_blocks;
    bitmap_blocks = super_b->nr_bmap_blocks;
    sys_printx("\nBlock bitmap (data blocks only):\n");
    for (i = 0; i < bitmap_blocks; i++) {
        int block_nr = bitmap_start + i;
        u8*block_buf = (u8*) kmalloc(BLOCK_SIZE);
        if (read_through(block_nr, block_buf, BLOCK_SIZE) != 0) {
            sys_printx("  failed to read block bitmap block ");
            sys_write_int_routine(block_nr); sys_printx("\n");
            continue;
        }
        for (j = 0; j < BLOCK_SIZE; j++) {
            u8 byte = block_buf[j];
            if (byte == 0) continue;
            for (int bit = 0; bit < 8; bit++) {
                if (byte & (1 << bit)) {
                    int global_bit = i * (BLOCK_SIZE * 8) + j * 8 + bit;
                    if (global_bit >= super_b->first_data_block && global_bit < total_blocks) {
                        used_blocks++;
                    }
                }
            }
        }
    }
    sys_printx("  used data blocks: "); sys_write_int_routine(used_blocks);
    sys_printx(" / "); sys_write_int_routine(total_blocks - super_b->first_data_block);
    sys_printx(" (excluding reserved)\n");

    /* 4. 打印内存中的 inode 缓存内容 */
    sys_printx("\nInode cache (in memory):\n");
    for (i = 0; i < NR_INODE; i++) {
        struct inode *ino = &inode_table[i];
        if (ino->i_cnt == 0) continue;
        sys_printx("  slot "); sys_write_int_routine(i);
        sys_printx(": inode_nr="); sys_write_int_routine(ino->i_num);
        sys_printx(" cnt="); sys_write_int_routine(ino->i_cnt);
        sys_printx(" mode="); sys_write_int_routine(ino->i_mode);
        sys_printx(" size="); sys_write_int_routine(ino->i_size);
        sys_printx(" nr_blocks="); sys_write_int_routine(ino->i_nr_blocks);
        sys_printx("\n");
    }
}

/**
 * 递归打印目录内容（每个条目一行）
 * @param inode_nr 当前目录的 inode 号
 * @param depth    当前深度（用于缩进）
 */
static void print_dir_tree_recursive(int inode_nr, int depth)
{
    struct super_block *sb = get_super_block();
    u32 inode_region_start = sb->first_data_block - sb->nr_inode_blocks;
    struct inode *dir_ino = read_inode(inode_nr, inode_region_start, sb->inode_size);
    if (dir_ino == NULL) {
        for (int i = 0; i < depth; i++) sys_printx("  ");
        sys_printx("|- [ERROR] cannot read inode ");
        sys_write_int_routine(inode_nr);
        sys_printx("\n");
        return;
    }
    if ((dir_ino->i_mode & I_TYPE_MASK) != I_DIRECTORY) {
        for (int i = 0; i < depth; i++) sys_printx("  ");
        sys_printx("|- [ERROR] not a directory\n");
        return;
    }

    int dir_ent_size = sb->dir_ent_size;
    int block_size = sb->block_size;
    int entries_per_block = block_size / dir_ent_size;
    int num_blocks = (dir_ino->i_size + block_size - 1) / block_size;

    for (int blk = 0; blk < num_blocks; blk++) {
        int phys_block = get_block_nr(dir_ino, blk);
        if (phys_block < 0) continue;
        u8 *block_buf = (u8 *)kmalloc(block_size);
        if (block_buf == NULL) continue;
        if (read_through(phys_block, block_buf, block_size) != 0) {
            continue;
        }

        for (int ent = 0; ent < entries_per_block; ent++) {
            struct dir_entry *de = (struct dir_entry *)(block_buf + ent * dir_ent_size);
            if (de->inode_nr == 0) continue;

            /* 打印缩进 */
            if (strcmp(de->name, ".") != 0 && strcmp(de->name, "..") != 0) {
                sys_printx("\n");
                for (int i = 0; i < depth; i++) sys_printx("  ");
                sys_printx("|--");
                sys_printx(de->name);
            }

            struct inode *child_ino = read_inode(de->inode_nr, inode_region_start, sb->inode_size);
            if (child_ino == NULL) {
                sys_printx(" (inode missing)\n");
                continue;
            }

            int type = child_ino->i_mode & I_TYPE_MASK;
            if (type == I_DIRECTORY) {
                //sys_printx("\n");
                /* 递归进入子目录，跳过 "." 和 ".." */
                if (strcmp(de->name, ".") != 0 && strcmp(de->name, "..") != 0) {
                    print_dir_tree_recursive(de->inode_nr, depth + 1);
                }
            } else if (type == I_REGULAR) {
                sys_printx("(file, size=");
                sys_write_int_routine(child_ino->i_size);
                sys_printx(")");
            } else if (type == I_CHAR_SPECIAL) {
                sys_printx(" (char device)\n");
            } else {
                sys_printx(" (unknown)\n");
            }
        }
    }
}

/**
 * 从根目录开始打印整个目录树
 */
PUBLIC void sys_print_dir_tree(void)
{
    struct super_block *sb = get_super_block();
    sys_printx("\n========== Directory Tree ==========\n");
    sys_printx("/");
    print_dir_tree_recursive(sb->root_inode, 0);
    sys_printx("\n====================================\n");
}