/*************************************************************************//**
 *****************************************************************************
 * @file   include/sys/fs.h
 * @brief  Header file for File System.
 * @author Forrest Yu
 * @date   2008
 *****************************************************************************
 *****************************************************************************/

#ifndef	_ORANGES_FS_H_
#define	_ORANGES_FS_H_

/**
 * @struct dev_drv_map fs.h "include/sys/fs.h"
 * @brief  The Device_nr.\ - Driver_nr.\ MAP.
 *
 *  \dot
 *  digraph DD_MAP {
 *    graph[rankdir=LR];
 *    node [shape=record, fontname=Helvetica];
 *    b [ label="Device Nr."];
 *    c [ label="Driver (the task)"];
 *    b -> c [ label="DD_MAP", fontcolor=blue, URL="\ref DD_MAP", arrowhead="open", style="dashed" ];
 *  }
 *  \enddot
 */
struct dev_drv_map {
	int driver_nr; /**< The proc nr.\ of the device driver. */
};

/**
 * @def   MAGIC_V1
 * @brief Magic number of FS v1.0
 */
#define	MAGIC_V1	0x111

struct super_block {
	u32	magic;		  /**< Magic number */
	u32	nr_inodes;	  /**< How many inodes */
	u32	nr_blocks;	  /**< How many blocks (including bit maps) */
	u32	nr_imap_blocks;	  /**< How many inode-map blocks */
	u32	nr_bmap_blocks;	  /**< How many block-map blocks */
	u32	first_data_block;	  /**< Number of the 1st data block */
	u32	nr_inode_blocks;   /**< How many inode blocks */
	u32	root_inode;       /**< Inode nr of root directory */
	u32	inode_size;       /**< INODE_SIZE */
	u32 block_size;            /* 块大小（字节） */
	u32	inode_isize_off;  /**< Offset of `struct inode::i_size' */
	u32	inode_start_off;  /**< Offset of `struct inode::i_start_sect' */
	u32	dir_ent_size;     /**< DIR_ENTRY_SIZE */
	u32	dir_ent_inode_off;/**< Offset of `struct dir_entry::inode_nr' */
	u32	dir_ent_fname_off;/**< Offset of `struct dir_entry::name' */
	u8	_unused[4];	/**< Stuff for alignment */

	/*
	 * the following item(s) are only present in memory
	 */
	int	sb_dev; 	/**< the super block's home device */
};

/**
 * @def   SUPER_BLK_MAGIC_V1
 * @brief Magic number of super block, version 1.
 * @attention It must correspond with boot/include/load.h::SB_MAGIC_V1
 */
#define	SUPER_BLK_MAGIC_V1		0x111
/**
 * @def   SUPER_BLOCK_SIZE
 * @brief The size of super block \b in \b the \b device.
 *
 * Note that this is the size of the struct in the device, \b NOT in memory.
 * The size in memory is larger because of some more members.
 */
#define	SUPER_BLOCK_SIZE	64
/* 块大小定义（4KB） */
#define BLOCK_SIZE         4096
#define BLOCK_SIZE_SHIFT   12
#define BITS_PER_BLOCK      (BLOCK_SIZE * 8) 
#define SECTORS_PER_BLOCK   (BLOCK_SIZE / SECTOR_SIZE)   // = 8
#define MAX_PATH_LEN        256

/* 缓存配置 */
#define CACHE_BLOCKS       (1 * 1024 * 1024 / BLOCK_SIZE)   /* 256 个块 */
#define INODE_DIRECT_COUNT 10

/**
 * @struct inode
 * @brief  i-node
 * \b NOTE: Remember to change INODE_SIZE if the members are changed
 */
struct inode {
	u32	i_mode;		/**< Accsess mode. Unused currently */
	u32	i_size;		/**< File size */
	u32	i_start_block;	/**< The first block of the data */
	u32	i_nr_blocks;	/**< How many blocks the file occupies */
	u32 i_direct[INODE_DIRECT_COUNT];   /* 直接块指针 */
	u32 i_indirect;            /* 间接块指针 */
	u8	_unused[4];	/**< Stuff for alignment */

	/* the following items are only present in memory */
	//int	i_dev;
	int	i_cnt;		/**< How many procs share this inode  */
	int	i_num;		/**< inode nr.  */
};

/**
 * @def   INODE_SIZE
 * @brief The size of i-node stored \b in \b the \b device.
 *
 * Note that this is the size of the struct in the device, \b NOT in memory.
 * The size in memory is larger because of some more members.
 */
#define	INODE_SIZE	64

/**
 * @def   MAX_FILENAME_LEN
 * @brief Max len of a filename
 * @see   dir_entry
 */
#define	MAX_FILENAME_LEN	12

/**
 * @struct dir_entry
 * @brief  Directory Entry
 */
struct dir_entry {
	int		inode_nr;		/**< inode nr. */
	char	name[MAX_FILENAME_LEN];	/**< Filename */
};

/**
 * @def   DIR_ENTRY_SIZE
 * @brief The size of directory entry in the device.
 *
 * It is as same as the size in memory.
 */
#define	DIR_ENTRY_SIZE			sizeof(struct dir_entry)
#define DIR_ENTRY_SIZE_SHIFT 	4


/**
 * @struct file_desc
 * @brief  File Descriptor
 */
struct file_desc {
	int		fd_mode;	/**< R or W */
	int		fd_pos;		/**< Current position for R/W. */
	struct inode*	fd_inode;	/**< Ptr to the i-node */
};


/**
 * Since all invocations of `rw_sector()' in FS look similar (most of the
 * params are the same), we use this macro to make code more readable.
 *
 * Before I wrote this macro, I found almost every rw_sector invocation
 * line matchs this emacs-style regex:
 * `rw_sector(\([-a-zA-Z0-9_>\ \*()+.]+,\)\{3\}\ *SECTOR_SIZE,\ *TASK_FS,\ *fsbuf)'
 */
//#define RD_SECT(dev,sect_nr) rw_sector(DEV_READ, \
				       dev,				\
				       (sect_nr) * SECTOR_SIZE,		\
				       SECTOR_SIZE, /* read one sector */ \
				       TASK_FS,				\
				       fsbuf);
//#define WR_SECT(dev,sect_nr) rw_sector(DEV_WRITE, \
				       dev,				\
				       (sect_nr) * SECTOR_SIZE,		\
				       SECTOR_SIZE, /* write one sector */ \
				       TASK_FS,				\
				       fsbuf);

/* 缓存块结构 */
struct cache_block {
    u32 block_nr;          /* 块号，-1 表示空闲 */
    u8 data[BLOCK_SIZE];   /* 数据 */
    int dirty;             /* 脏标志 */
    u32 access_time;       /* 访问时间（用于 LRU） */
    struct cache_block *prev, *next;  /* LRU 链表指针 */
};

/* 全局变量 */
static struct cache_block* cache;           /* 缓存数组，通过 kmalloc 分配 */
static struct cache_block* lru_head;        /* LRU 链表头 */
static struct cache_block* lru_tail;        /* LRU 链表尾 */
static int cache_initialized = 0;
static u32 cache_time_counter = 0;          /* 简单的时间计数器 */
/* 全局标志，防止递归 */
static int in_cache_write = 0;

PRIVATE	u8	fsbuf[4096];
PRIVATE	u8	fsbuf2[4096];
	
#endif /* _ORANGES_FS_H_ */
