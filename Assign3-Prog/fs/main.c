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

PRIVATE	u8	fsbuf[SECTOR_SIZE];

PUBLIC void sys_init_fs();
PUBLIC int sys_open(char *pathname, int flags);
PUBLIC int sys_close(int fd);
PUBLIC int sys_read(int fd, void *buf, int count);
PUBLIC int sys_write(int fd, void *buf, int count);
PUBLIC int sys_rdwt(int io_type, int fd, void *buf, int count);


PRIVATE void mkfs();
PUBLIC int rw_sector();

PRIVATE struct inode * create_file(char * path, int flags);
PRIVATE int alloc_imap_bit(int dev);
PRIVATE int alloc_smap_bit(int dev, int nr_sects_to_alloc);
PRIVATE struct inode * new_inode(int dev, int inode_nr, int start_sect);
PRIVATE void new_dir_entry(struct inode * dir_inode, int inode_nr, char * filename);

PUBLIC int search_file(char * path);
PUBLIC int strip_path(char * filename, const char * pathname,
		      struct inode** ppinode);

PUBLIC struct inode *		get_inode(int dev, int num);
PUBLIC void			put_inode(struct inode * pinode);
PUBLIC void			sync_inode(struct inode * p);
PUBLIC struct super_block *	get_super_block(int dev);
PRIVATE void read_super_block(int dev);

#define RD_SECT(dev, sect_nr) rw_sector(DEV_READ, \
				       dev,				\
				       ((sect_nr) * SECTOR_SIZE),		\
				       SECTOR_SIZE, /* read one sector */ \
				       TASK_FS,				\
				       fsbuf);

#define WR_SECT(dev, sect_nr) rw_sector(DEV_WRITE, \
				       dev,				\
				       ((sect_nr) * SECTOR_SIZE),		\
				       SECTOR_SIZE, /* write one sector */ \
				       TASK_FS,				\
				       fsbuf);

/*****************************************************************************
 *                                rw_sector
 *****************************************************************************/
/**
 * 
 * @param io_type  DEV_READ or DEV_WRITE
 * @param dev      device nr
 * @param pos      Byte offset from/to where to r/w.
 * @param bytes    r/w count in bytes.
 * @param proc_nr  To whom the buffer belongs.
 * @param buf      r/w buffer.
 * 
 * @return Zero if success.
 *****************************************************************************/
PUBLIC int rw_sector(int io_type, int dev, u64 pos, int bytes, int proc_nr, void* buf)
{
	sys_hd_rdwt(io_type, MINOR(dev), pos, bytes, proc_nr, buf);

	return 0;
}


PUBLIC void sys_init_fs()
{
	int i;

	/* f_desc_table[] */
	for (i = 0; i < NR_FILE_DESC; i++)
		memset(&f_desc_table[i], 0, sizeof(struct file_desc));

	/* inode_table[] */
	for (i = 0; i < NR_INODE; i++)
		memset(&inode_table[i], 0, sizeof(struct inode));

	/* super_block[] */
	struct super_block * sb = super_block;
	for (; sb < &super_block[NR_SUPER_BLOCK]; sb++)
		sb->sb_dev = NO_DEV;

	sys_hd_open(MINOR(ROOT_DEV));

	/* make FS */
	mkfs();
	
	read_super_block(ROOT_DEV);


	sb = get_super_block(ROOT_DEV);

	root_inode = get_inode(ROOT_DEV, ROOT_INODE);
}

/*****************************************************************************
 *                                mkfs
 *****************************************************************************/
/**
 *  Make a available Orange'S FS in the disk. It will
 *          - Write a super block to sector 1.
 *          - Create three special files: dev_tty0, dev_tty1, dev_tty2
 *          - Create the inode map
 *          - Create the sector map
 *          - Create the inodes of the files
 *          - Create `/', the root directory
 *****************************************************************************/
PRIVATE void mkfs()
{
	int i, j;

	int bits_per_sect = SECTOR_SIZE * 8; /* 8 bits per byte */

	/* get the geometry of ROOTDEV */
	struct part_info geo;
	sys_hd_ioctl(MINOR(ROOT_DEV), DIOCTL_GET_GEO, &geo);
	
	sys_printx("dev size: ");
	sys_write_int_routine(geo.size);
	sys_printx(" sectors\n");
	
	/************************/
	/*      super block     */
	/************************/
	struct super_block sb;
	sb.magic	  = MAGIC_V1;
	sb.nr_inodes	  = bits_per_sect;
	sb.nr_inode_sects = sb.nr_inodes * INODE_SIZE / SECTOR_SIZE;
	sb.nr_sects	  = geo.size; /* partition size in sector */
	sb.nr_imap_sects  = 1;
	sb.nr_smap_sects  = sb.nr_sects / bits_per_sect + 1;
	sb.n_1st_sect	  = 1 + 1 +   /* boot sector & super block */
		sb.nr_imap_sects + sb.nr_smap_sects + sb.nr_inode_sects;
	sb.root_inode	  = ROOT_INODE;
	sb.inode_size	  = INODE_SIZE;
	struct inode x;
	sb.inode_isize_off= (int)&x.i_size - (int)&x;
	sb.inode_start_off= (int)&x.i_start_sect - (int)&x;
	sb.dir_ent_size	  = DIR_ENTRY_SIZE;
	struct dir_entry de;
	sb.dir_ent_inode_off = (int)&de.inode_nr - (int)&de;
	sb.dir_ent_fname_off = (int)&de.name - (int)&de;
	
	memset(fsbuf, 0x90, SECTOR_SIZE);
	memcpy(fsbuf, &sb, SUPER_BLOCK_SIZE);

	/* write the super block */
	WR_SECT(ROOT_DEV, 1);

	/************************/
	/*       inode map      */
	/************************/
	memset(fsbuf, 0, SECTOR_SIZE);
	for (i = 0; i < (NR_CONSOLES + 2); i++)
		fsbuf[0] |= 1 << i;

	//assert(fsbuf[0] == 0x1F);/* 0001 1111 : 
	//			  *    | ||||
	//			  *    | |||`--- bit 0 : reserved
	//			  *    | ||`---- bit 1 : the first inode,
	//			  *    | ||              which indicates `/'
	//			  *    | |`----- bit 2 : /dev_tty0
	//			  *    | `------ bit 3 : /dev_tty1
	//			  *    `-------- bit 4 : /dev_tty2
	//			  */
	WR_SECT(ROOT_DEV, 2);

	/************************/
	/*      secter map      */
	/************************/
	memset(fsbuf, 0, SECTOR_SIZE);
	int nr_sects = NR_DEFAULT_FILE_SECTS + 1;
	/*             ~~~~~~~~~~~~~~~~~~~|~   |
	 *                                |    `--- bit 0 is reserved
	 *                                `-------- for `/'
	 */
	for (i = 0; i < nr_sects >> 3; i++)
	{
		fsbuf[i] = 0xFF;
	}

	for (j = 0; j < nr_sects % 8; j++)
		fsbuf[i] |= (1 << j);

	
	
	WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects);

	/* zeromemory the rest sector-map */
	memset(fsbuf, 0, SECTOR_SIZE);
	for (i = 1; i < sb.nr_smap_sects; i++)
		WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + i);

	/************************/
	/*       inodes         */
	/************************/
	/* inode of `/' */
	memset(fsbuf, 0, SECTOR_SIZE);
	struct inode * pi = (struct inode*)fsbuf;
	pi->i_mode = I_DIRECTORY;
	pi->i_size = DIR_ENTRY_SIZE * 4; /* 4 files:
					  * `.',
					  * `dev_tty0', `dev_tty1', `dev_tty2',
					  */
	pi->i_start_sect = sb.n_1st_sect;
	pi->i_nr_sects = NR_DEFAULT_FILE_SECTS;
	/* inode of `/dev_tty0~2' */
	for (i = 0; i < NR_CONSOLES; i++) {
		pi = (struct inode*)(fsbuf + (INODE_SIZE * (i + 1)));
		pi->i_mode = I_CHAR_SPECIAL;
		pi->i_size = 0;
		pi->i_start_sect = MAKE_DEV(DEV_CHAR_TTY, i);
		pi->i_nr_sects = 0;
	}
	WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + sb.nr_smap_sects);
	
	/************************/
	/*          `/'         */
	/************************/
	memset(fsbuf, 0, SECTOR_SIZE);
	struct dir_entry * pde = (struct dir_entry *)fsbuf;

	pde->inode_nr = 1;
	strcpy(pde->name, ".");

	/* dir entries of `/dev_tty0~2' */
	for (i = 0; i < NR_CONSOLES; i++) {
		pde++;
		pde->inode_nr = i + 2; /* dev_tty0's inode_nr is 2 */
		strcpy(pde->name, "dev_tty");
	}
	WR_SECT(ROOT_DEV, sb.n_1st_sect);
	return;
}


PUBLIC int sys_open(char *pathname, int flags)
{
	int fd = -1;		/* return value */
	
	/* get parameters from the message */
	int name_len = strlen(pathname);	/* length of filename */

	/* find a free slot in PROCESS::filp[] */
	int i;
	for (i = 0; i < 64; i++) {
		if (pcaller->filp[i] == 0) {
			fd = i;
			break;
		}
	}
	if ((fd < 0) || (i >= NR_FILES)){
		sys_printx("filp[] is full, the fd is ");
		sys_write_int_routine(fd);
		sys_printx("\nthe pid is ");
		sys_write_int_routine(proc2pid(pcaller));
	}
		

	/* find a free slot in f_desc_table[] */
	for (i = 0; i < NR_FILE_DESC; i++)
		if (f_desc_table[i].fd_inode == 0)
			break;
	if (i >= NR_FILE_DESC)
		sys_printx("f_desc_table[] is full");

	int inode_nr = search_file(pathname);

	struct inode * pin = 0;
	
	if (flags & O_CREAT) {
		sys_printx("flag is O_CREAT\n");
		if (inode_nr) {
			sys_printx("file exists.\n");
			return -1;
		}
		else {
			sys_printx("fils does not exist, create it\n");
			pin = create_file(pathname, flags);
		}
	}
	else {
		sys_printx("flag is O_RDWR\n");
		char filename[MAX_PATH];
		struct inode * dir_inode;
		if (strip_path(filename, pathname, &dir_inode) != 0)
			return -1;
		pin = get_inode(dir_inode->i_dev, inode_nr);
	}

	if (pin) {
		sys_printx("\nfile is created\n");
		/* connects proc with file_descriptor */
		pcaller->filp[fd] = &f_desc_table[i];

		/* connects file_descriptor with inode */
		f_desc_table[i].fd_inode = pin;

		f_desc_table[i].fd_mode = flags;
		/* f_desc_table[i].fd_cnt = 1; */
		f_desc_table[i].fd_pos = 0;

		int imode = pin->i_mode & I_TYPE_MASK;

		if (imode == I_CHAR_SPECIAL) {
			int dev = pin->i_start_sect;
			sys_hd_open(MINOR(dev));
		}
		else if (imode == I_DIRECTORY) {
		}
		else {
		}
	}
	else {
		return -1;
	}

	return fd;
}


PUBLIC int sys_close(int fd)
{
	put_inode(pcaller->filp[fd]->fd_inode);
	pcaller->filp[fd]->fd_inode = 0;
	pcaller->filp[fd] = 0;

	return 0;
}


PUBLIC int sys_read(int fd, void *buf, int count)
{
	sys_rdwt(READ, fd, buf, count);
}

PUBLIC int sys_write(int fd, void *buf, int count)
{
	sys_rdwt(WRITE, fd, buf, count);
}


PUBLIC int sys_rdwt(int io_type, int fd, void *buf, int count)
{
	sys_printx("sys_rdwt is called, buf is \n");
	sys_printx(buf);
	
	int len = count;	/**< r/w bytes */

	if (!(pcaller->filp[fd]->fd_mode & O_RDWR))
		return -1;

	int pos = pcaller->filp[fd]->fd_pos;

	struct inode * pin = pcaller->filp[fd]->fd_inode;

	int imode = pin->i_mode & I_TYPE_MASK;

	if (imode == I_CHAR_SPECIAL) {
		int t = io_type == READ ? DEV_READ : DEV_WRITE;
		
		int dev = pin->i_start_sect;
		sys_hd_rdwt(t, dev, pos, len, TASK_HD, buf);
		return len;
	}
	else {
		int pos_end;
		if (io_type == READ)
		{
			if (pos + len < pin->i_size) {
				pos_end = pos + len;
			} else {
				pos_end = pin->i_size;
			}
		}
		else		/* WRITE */
		{
			if (pos + len < pin->i_nr_sects * SECTOR_SIZE) {
				pos_end = pos + len;
			} else {
				pos_end = pin->i_nr_sects * SECTOR_SIZE;
			}
			
		}

		int off = pos % SECTOR_SIZE;
		int rw_sect_min=pin->i_start_sect+(pos>>SECTOR_SIZE_SHIFT);
		int rw_sect_max=pin->i_start_sect+(pos_end>>SECTOR_SIZE_SHIFT);
		int chunk;
		if (rw_sect_max - rw_sect_min + 1 < SECTOR_SIZE >> SECTOR_SIZE_SHIFT) {
			chunk = rw_sect_max - rw_sect_min + 1; 
		} else {
			chunk = SECTOR_SIZE >> SECTOR_SIZE_SHIFT;
		}

		int bytes_rw = 0;
		int bytes_left = len;
		int i;
		for (i = rw_sect_min; i <= rw_sect_max; i += chunk) {
			/* read/write this amount of bytes every time */
			int bytes;
			if (bytes_left < chunk * SECTOR_SIZE - off) {
				bytes = bytes_left; 
			} else {
				bytes = chunk * SECTOR_SIZE - off;
			}

			rw_sector(DEV_READ,
				  pin->i_dev,
				  i * SECTOR_SIZE,
				  chunk * SECTOR_SIZE,
				  TASK_FS,
				  fsbuf);

			if (io_type == READ) {
				phys_copy(buf + bytes_rw,
					  fsbuf + off,
					  bytes);
			}
			else {	/* WRITE */
				phys_copy(fsbuf + off,
					  buf + bytes_rw,
					  bytes);

				rw_sector(DEV_WRITE,
					  pin->i_dev,
					  i * SECTOR_SIZE,
					  chunk * SECTOR_SIZE,
					  TASK_FS,
					  fsbuf);
			}
			off = 0;
			bytes_rw += bytes;
			pcaller->filp[fd]->fd_pos += bytes;
			bytes_left -= bytes;
		}

		if (pcaller->filp[fd]->fd_pos > pin->i_size) {
			/* update inode::size */
			pin->i_size = pcaller->filp[fd]->fd_pos;

			/* write the updated i-node back to disk */
			sync_inode(pin);
		}

		return bytes_rw;
	}
}



PRIVATE struct inode * create_file(char * path, int flags)
{
	char filename[MAX_PATH];
	struct inode * dir_inode;
	sys_printx("create_file is called");

	if (strip_path(filename, path, &dir_inode) != 0)
		return 0;

	int inode_nr = alloc_imap_bit(dir_inode->i_dev);

	int free_sect_nr = alloc_smap_bit(dir_inode->i_dev,
					  NR_DEFAULT_FILE_SECTS);

	struct inode *newino = new_inode(dir_inode->i_dev, inode_nr,
					 free_sect_nr);

	new_dir_entry(dir_inode, newino->i_num, filename);

	return newino;
}

PRIVATE int alloc_imap_bit(int dev)
{
	int inode_nr = 0;
	int i, j, k;

	int imap_blk0_nr = 1 + 1; /* 1 boot sector & 1 super block */
	struct super_block * sb = get_super_block(dev);

	for (i = 0; i < sb->nr_imap_sects; i++) {
		RD_SECT(dev, imap_blk0_nr + i);

		for (j = 0; j < SECTOR_SIZE; j++) {
			/* skip `11111111' bytes */
			if (fsbuf[j] == 0xFF)
				continue;

			/* skip `1' bits */
			for (k = 0; ((fsbuf[j] >> k) & 1) != 0; k++) {}

			/* i: sector index; j: byte index; k: bit index */
			inode_nr = (i * SECTOR_SIZE + j) * 8 + k;
			fsbuf[j] |= (1 << k);

			/* write the bit to imap */
			WR_SECT(dev, imap_blk0_nr + i);
			break;
		}

		return inode_nr;
	}

	/* no free bit in imap */
	sys_printx("inode-map is probably full.\n");

	return 0;
}


PRIVATE int alloc_smap_bit(int dev, int nr_sects_to_alloc)
{
	/* int nr_sects_to_alloc = NR_DEFAULT_FILE_SECTS; */

	int i; /* sector index */
	int j; /* byte index */
	int k; /* bit index */

	struct super_block * sb = get_super_block(dev);

	int smap_blk0_nr = 1 + 1 + sb->nr_imap_sects;
	int free_sect_nr = 0;

	for (i = 0; i < sb->nr_smap_sects; i++) { /* smap_blk0_nr + i :
						     current sect nr. */
		RD_SECT(dev, smap_blk0_nr + i);

		/* byte offset in current sect */
		for (j = 0; j < SECTOR_SIZE && nr_sects_to_alloc > 0; j++) {
			k = 0;
			if (!free_sect_nr) {
				/* loop until a free bit is found */
				if (fsbuf[j] == 0xFF) continue;
				for (; ((fsbuf[j] >> k) & 1) != 0; k++) {}
				free_sect_nr = (i * SECTOR_SIZE + j) * 8 +
					k - 1 + sb->n_1st_sect;
			}

			for (; k < 8; k++) { /* repeat till enough bits are set */
				fsbuf[j] |= (1 << k);
				if (--nr_sects_to_alloc == 0)
					break;
			}
		}

		if (free_sect_nr) /* free bit found, write the bits to smap */
			WR_SECT(dev, smap_blk0_nr + i);

		if (nr_sects_to_alloc == 0)
			break;
	}
	return free_sect_nr;
}


PRIVATE struct inode * new_inode(int dev, int inode_nr, int start_sect)
{
	struct inode * new_inode = get_inode(dev, inode_nr);

	new_inode->i_mode = I_REGULAR;
	new_inode->i_size = 0;
	new_inode->i_start_sect = start_sect;
	new_inode->i_nr_sects = NR_DEFAULT_FILE_SECTS;

	new_inode->i_dev = dev;
	new_inode->i_cnt = 1;
	new_inode->i_num = inode_nr;

	/* write to the inode array */
	sync_inode(new_inode);

	return new_inode;
}

PRIVATE void new_dir_entry(struct inode *dir_inode,int inode_nr,char *filename)
{
	/* write the dir_entry */
	int dir_blk0_nr = dir_inode->i_start_sect;
	int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE) / SECTOR_SIZE;
	int nr_dir_entries =
		dir_inode->i_size / DIR_ENTRY_SIZE; /**
						     * including unused slots
						     * (the file has been
						     * deleted but the slot
						     * is still there)
						     */
	int m = 0;
	struct dir_entry * pde;
	struct dir_entry * new_de = 0;

	int i, j;
	for (i = 0; i < nr_dir_blks; i++) {
		RD_SECT(dir_inode->i_dev, dir_blk0_nr + i);

		pde = (struct dir_entry *)fsbuf;
		for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,pde++) {
			if (++m > nr_dir_entries)
				break;

			if (pde->inode_nr == 0) { /* it's a free slot */
				new_de = pde;
				break;
			}
		}
		if (m > nr_dir_entries ||/* all entries have been iterated or */
		    new_de)              /* free slot is found */
			break;
	}
	if (!new_de) { /* reached the end of the dir */
		new_de = pde;
		dir_inode->i_size += DIR_ENTRY_SIZE;
	}
	new_de->inode_nr = inode_nr;
	strcpy(new_de->name, filename);

	/* write dir block -- ROOT dir block */
	WR_SECT(dir_inode->i_dev, dir_blk0_nr + i);

	/* update dir inode */
	sync_inode(dir_inode);
}


PUBLIC int search_file(char * path)
{
	int i, j;

	char filename[MAX_PATH];
	memset(filename, 0, MAX_FILENAME_LEN);
	struct inode * dir_inode;
	if (strip_path(filename, path, &dir_inode) != 0)
		return 0;

	if (filename[0] == 0)	/* path: "/" */
		return dir_inode->i_num;

	/**
	 * Search the dir for the file.
	 */
	int dir_blk0_nr = dir_inode->i_start_sect;
	int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE - 1) >> 9;
	int nr_dir_entries = dir_inode->i_size >> 4; /**
					       * including unused slots
					       * (the file has been deleted
					       * but the slot is still there)
					       */
	sys_printx("\n");
	sys_write_int_routine(nr_dir_entries);
	sys_printx("\n");
	//return 0;

	int m = 0;
	struct dir_entry * pde;
	for (i = 0; i < nr_dir_blks; i++) {
		RD_SECT(dir_inode->i_dev, dir_blk0_nr + i);
		pde = (struct dir_entry *)fsbuf;
		for (j = 0; j < SECTOR_SIZE >> 4; j++,pde++) {
			sys_printx("\n");
			sys_write_int_routine(j);
			sys_printx("\n");

			if (memcmp(filename, pde->name, MAX_FILENAME_LEN) == 0)
				return pde->inode_nr;
			if (++m > nr_dir_entries)
				break;
		}
		if (m > nr_dir_entries) /* all entries have been iterated */
			break;
	}

	/* file not found */
	return 0;
}


PUBLIC int strip_path(char * filename, const char * pathname,
		      struct inode** ppinode)
{
	const char * s = pathname;
	char * t = filename;

	if (s == 0)
		return -1;

	if (*s == '/')
		s++;

	while (*s) {		/* check each character */
		if (*s == '/')
			return -1;
		*t++ = *s++;
		/* if filename is too long, just truncate it */
		if (t - filename >= MAX_FILENAME_LEN)
			break;
	}
	*t = 0;

	*ppinode = root_inode;

	return 0;
}


PRIVATE void read_super_block(int dev)
{
	int i;
	sys_hd_rdwt(DEV_READ, MINOR(dev), SECTOR_SIZE * 1, SECTOR_SIZE, TASK_FS, fsbuf);


	/* find a free slot in super_block[] */
	for (i = 0; i < NR_SUPER_BLOCK; i++)
		if (super_block[i].sb_dev == NO_DEV)
			break;
	if (i == NR_SUPER_BLOCK)
		//panic("super_block slots used up");
		sys_printx("super_block slots used up");
	struct super_block * psb = (struct super_block *)fsbuf;

	super_block[i] = *psb;
	super_block[i].sb_dev = dev;
}


PUBLIC struct super_block * get_super_block(int dev)
{
	struct super_block * sb = super_block;
	for (; sb < &super_block[NR_SUPER_BLOCK]; sb++)
		if (sb->sb_dev == dev)
			return sb;
	sys_printx("super block of devie not found.\n");

	return 0;
}


PUBLIC struct inode * get_inode(int dev, int num)
{
	if (num == 0)
		return 0;
	struct inode * p;
	struct inode * q = 0;
	for (p = &inode_table[0]; p < &inode_table[NR_INODE]; p++) {
		if (p->i_cnt) {	/* not a free slot */
			if ((p->i_dev == dev) && (p->i_num == num)) {
				/* this is the inode we want */
				p->i_cnt++;
				return p;
			}
		}
		else {		/* a free slot */
			if (!q) /* q hasn't been assigned yet */
				q = p; /* q <- the 1st free slot */
		}
	}

	if (!q)
		sys_printx("the inode table is full");

	q->i_dev = dev;
	q->i_num = num;
	q->i_cnt = 1;

	struct super_block * sb = get_super_block(dev);
	
	int blk_nr;
	if (sb != 0) {
		blk_nr = 1 + 1 + sb->nr_imap_sects + sb->nr_smap_sects + ((num - 1) >> 4);
	}
	else {
		sys_printx("super block does not exist.\n");
		return 0; 
	}

	RD_SECT(dev, blk_nr);
	struct inode * pinode = (struct inode*) ((u8*)fsbuf + ((num - 1 ) % (SECTOR_SIZE / INODE_SIZE)) * INODE_SIZE);
	q->i_mode = pinode->i_mode;
	q->i_size = pinode->i_size;
	q->i_start_sect = pinode->i_start_sect;
	q->i_nr_sects = pinode->i_nr_sects;
	return q;
}


PUBLIC void put_inode(struct inode * pinode)
{
	pinode->i_cnt--;
}

PUBLIC void sync_inode(struct inode * p)
{
	struct inode * pinode;
	struct super_block * sb = get_super_block(p->i_dev);
	int blk_nr = 1 + 1 + sb->nr_imap_sects + sb->nr_smap_sects + ((p->i_num - 1) >> 4);
	RD_SECT(p->i_dev, blk_nr);
	pinode = (struct inode*)((u8*)fsbuf +
				 (((p->i_num - 1) % (SECTOR_SIZE / INODE_SIZE))
				  * INODE_SIZE));
	pinode->i_mode = p->i_mode;
	pinode->i_size = p->i_size;
	pinode->i_start_sect = p->i_start_sect;
	pinode->i_nr_sects = p->i_nr_sects;
	WR_SECT(p->i_dev, blk_nr);
}

PUBLIC int fs_fork(int pid)
{
	int i;
	TASK* child = &tasks[pid];
	for (i = 0; i < NR_FILES; i++) {
		if (child->filp[i]) {
			child->filp[i]->fd_cnt++;
			child->filp[i]->fd_inode->i_cnt++;
		}
	}
	return 0;
}