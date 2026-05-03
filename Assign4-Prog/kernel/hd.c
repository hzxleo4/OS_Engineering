/*************************************************************************//**
 *****************************************************************************
 * @file   hd.c
 * @brief  HD driver.
 * @author Forrest Y. Yu
 * @date   2005~2008
 *****************************************************************************
 *****************************************************************************/

#include "type.h"
#include "const.h"
#include "string.h"
#include "fs.h"
#include "task.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"
#include "hd.h"

PUBLIC void 	sys_hd_open			(int device);
PUBLIC void		sys_hd_close		(int device);
PUBLIC void		sys_hd_rdwt			(int io_type, int dev, u64 pos, int bytes, int proc_nr, void* buf);
PUBLIC void 	sys_hd_ioctl		(int dev, int request, void* buf);

PUBLIC void		sys_init_hd			();
PRIVATE void	hd_cmd_out		(struct hd_cmd* cmd);
PRIVATE int		waitfor			(int mask, int val, int timeout);
PRIVATE void	interrupt_wait		();
PRIVATE	void	hd_identify		(int drive);
PRIVATE	void	hd_identify_continue		();
PRIVATE void	print_identify_info	(u16* hdinfo);

PRIVATE void	partition		(int device, int style);
PRIVATE void	print_hdinfo		(struct hd_info * hdi);


PRIVATE	u8	hd_status;
PRIVATE	u8	hdbuf[SECTOR_SIZE];

PRIVATE	struct hd_info	hd_info[1];

//#define	DRV_OF_DEV(dev) (dev <= MAX_PRIM ? \
			 dev / NR_PRIM_PER_DRIVE : \
			 (dev - MINOR_hd1a) / NR_SUB_PER_DRIVE)

//PRIVATE int ctn_flag = 0;

/*****************************************************************************
 *                                task_hd
 *****************************************************************************/
/**
 * Main loop of HD driver.
 * 
 *****************************************************************************/
//PUBLIC void sys_hd_routine(MESSAGE m)
//{
	//MESSAGE msg;
	//sys_sendrec(RECEIVE, ANY, &msg);
	
	//sys_printx("arrive the hd routine, type is: ");
	//sys_write_int_routine(msg.type);
	//sys_printx("\n");

	//TASK* p = current; 
	//int caller = proc2pid(p);
	//MESSAGE* mla = (MESSAGE*)va2la(caller, m);
	//sys_printx("Arrive the hd routine, type is: ");
	//sys_write_int_routine(mla->type);
	//sys_printx(", DEVICE is: ");
	//sys_write_int_routine(mla->DEVICE);
	//sys_printx("\n");

//	switch (mla->type) {
//		case DEV_OPEN:
			//hd_identify(0);
			//hd_identify_continue(0);
			//sys_printx("hd_open is called\n");
//			hd_open(mla->DEVICE);
//			break;
//		case DEV_CLOSE:
			//sys_printx("hd_close is called\n");
//			hd_close(mla->DEVICE);
//			break;
//		case DEV_READ:
//		case DEV_WRITE:
			//sys_printx("hd_rdwt is called\n");
//			hd_rdwt(mla);
//			break;			
//		case DEV_IOCTL:
			//sys_printx("hd_ioctl is called\n");
//			hd_ioctl(mla);
//			break;
		//case CONTINUE:
		//	hd_identify_continue(0);

//		default:
//			break;
//	}
//}

/*****************************************************************************
 *                                init_hd
 *****************************************************************************/
/**
 * <Ring 1> Check hard drive, set IRQ handler, enable IRQ and initialize data
 *          structures.
 *****************************************************************************/
PUBLIC void sys_init_hd()
{
	/* Get the number of drives from the BIOS data area */
	u8 * pNrDrives = (u8*)(0x475);
	//printl("NrDrives:%d.\n", *pNrDrives);
	//assert(*pNrDrives);

	//put_irq_handler(AT_WINI_IRQ, hd_handler);
	enable_irq(CASCADE_IRQ);
	enable_irq(AT_WINI_IRQ);
	int i = 0;
	for (i = 0; i < (sizeof(hd_info) / sizeof(hd_info[0])); i++)
		memset(&hd_info[i], 0, sizeof(hd_info[0]));
	hd_info[0].open_cnt = 0;
}

/*****************************************************************************
 *                                hd_open
 *****************************************************************************/
/**
 * <Ring 1> This routine handles DEV_OPEN message. It identify the drive
 * of the given device and read the partition table of the drive if it
 * has not been read.
 * 
 * @param device The device to be opened.
 *****************************************************************************/
PUBLIC void sys_hd_open(int device)
{
	//int drive = DRV_OF_DEV(device);
	//assert(drive == 0);	/* only one drive */

	//hd_identify(drive);
	
	sys_printx("\nsys_hd_open is called\n");
	int drive = TASK_HD;
	hd_identify(drive);

	if (hd_info[drive].open_cnt++ == 0) {
		partition(drive * (NR_PART_PER_DRIVE + 1), P_PRIMARY);
		print_hdinfo(&hd_info[drive]);
	}
}


/*****************************************************************************
 *                                hd_close
 *****************************************************************************/
/**
 * <Ring 1> This routine handles DEV_CLOSE message. 
 * 
 * @param device The device to be opened.
 *****************************************************************************/
PUBLIC void sys_hd_close(int device)
{
	//int drive = DRV_OF_DEV(device);
	//assert(drive == 0);	/* only one drive */

	int drive = TASK_HD;
	hd_info[drive].open_cnt--;
}

/*****************************************************************************
 *                                hd_rdwt
 *****************************************************************************/
/**
 * <Ring 1> This routine handles DEV_READ and DEV_WRITE message.
 * 
 * @param p Message ptr.
 *****************************************************************************/
PUBLIC void sys_hd_rdwt(int io_type, int dev, u64 pos, int bytes, int proc_nr, void* buf)
{
	//int drive = DRV_OF_DEV(p->DEVICE);
	int drive = TASK_HD;

	//u64 pos = p->POSITION;
	//sys_printx("Arrive the hd_rdwt, POSITION is: ");
	//sys_write_int_routine(pos);
	//sys_printx("\n");
	//return; 

	//assert((pos >> SECTOR_SIZE_SHIFT) < (1 << 31));

	/**
	 * We only allow to R/W from a SECTOR boundary:
	 */
	//assert((pos & 0x1FF) == 0);

	u32 sect_nr = (u32)(pos >> SECTOR_SIZE_SHIFT); /* pos / SECTOR_SIZE */
	int logidx = (dev - MINOR_hd1a) % NR_SUB_PER_DRIVE;
	sect_nr += dev < MAX_PRIM ?
		hd_info[drive].primary[dev].base :
		hd_info[drive].logical[logidx].base;


	struct hd_cmd cmd;
	cmd.features	= 0;
	//return; 
	//sys_printx("CNT is");
	//sys_write_int_routine(p->CNT);
	//sys_write_int_routine((p->CNT + SECTOR_SIZE - 1) >> 9);
	//return; 
	cmd.count	= (bytes + SECTOR_SIZE - 1) >> 9;
	cmd.lba_low	= sect_nr & 0xFF;
	cmd.lba_mid	= (sect_nr >>  8) & 0xFF;
	cmd.lba_high	= (sect_nr >> 16) & 0xFF;
	cmd.device	= MAKE_DEVICE_REG(1, drive, (sect_nr >> 24) & 0xF);
	cmd.command	= (io_type == DEV_READ) ? ATA_READ : ATA_WRITE;
	hd_cmd_out(&cmd);
	

	int bytes_left = bytes;
	void * la = buf;
	//sys_printx("\nbytes_left is");
	//sys_write_int_routine(bytes_left);
	//sys_printx("\nSECTOR_SIZE is");
	//sys_write_int_routine(SECTOR_SIZE);
	//sys_printx("\nmin is");
	//int sector_size = SECTOR_SIZE; 
	//int bytes = (bytes_left < sector_size) ? bytes_left : sector_size;
	//sys_write_int_routine(bytes);
	//return; 

	//sys_printx("bytes is");
	//bytes = min(SECTOR_SIZE, bytes_left);
	//sys_write_int_routine(bytes);
	//return; 

	while (bytes_left) {
		int bytes;
		if (bytes_left < SECTOR_SIZE) {
			bytes = bytes_left;
		} else {
			bytes = SECTOR_SIZE; 
		}
		//int bytes = min(SECTOR_SIZE, bytes_left);
		if (io_type == DEV_READ) {
			//sys_printx("read is called\n");
			interrupt_wait();
			port_read(REG_DATA, hdbuf, SECTOR_SIZE);
			phys_copy(la, hdbuf, bytes);
		}
		else {
			if (!waitfor(STATUS_DRQ, STATUS_DRQ, HD_TIMEOUT))
				sys_printx("hd writing error\n");
			//sys_printx("write is called\n");
			port_write(REG_DATA, la, bytes);
			interrupt_wait();
		}
		bytes_left -= SECTOR_SIZE;
		la += SECTOR_SIZE;
	}
}															


/*****************************************************************************
 *                                hd_ioctl
 *****************************************************************************/
/**
 * <Ring 1> This routine handles the DEV_IOCTL message.
 * 
 * @param p  Ptr to the MESSAGE.
 *****************************************************************************/
PUBLIC void sys_hd_ioctl(int dev, int request, void* buf)
{
	int device = dev;
	//int drive = DRV_OF_DEV(device);
	int drive = TASK_HD; 

	struct hd_info * hdi = &hd_info[drive];

	if (request == DIOCTL_GET_GEO) {
		void * dst = buf;
		void * src = device < MAX_PRIM ? &hdi->primary[device] : &hdi->logical[(device - MINOR_hd1a) % NR_SUB_PER_DRIVE];

		//sys_printx("DIOCTL_GET_GEO is called\n");
		phys_copy(dst, src, sizeof(struct part_info));
	}
	else {
		//assert(0);
	}
}


/*****************************************************************************
 *                                get_part_table
 *****************************************************************************/
/**
 * <Ring 1> Get a partition table of a drive.
 * 
 * @param drive   Drive nr (0 for the 1st disk, 1 for the 2nd, ...)n
 * @param sect_nr The sector at which the partition table is located.
 * @param entry   Ptr to part_ent struct.
 *****************************************************************************/
PRIVATE void get_part_table(int drive, int sect_nr, struct part_ent * entry)
{
	
	//sys_printx("get_part_table is called with [");
	//sys_write_int_routine(drive);
	//sys_printx("]\n");

	struct hd_cmd cmd;
	cmd.features	= 0;
	cmd.count	= 1;
	cmd.lba_low	= sect_nr & 0xFF;
	cmd.lba_mid	= (sect_nr >>  8) & 0xFF;
	cmd.lba_high	= (sect_nr >> 16) & 0xFF;
	cmd.device	= MAKE_DEVICE_REG(1, /* LBA mode*/
					  drive,
					  (sect_nr >> 24) & 0xF);
	cmd.command	= ATA_READ;
	hd_cmd_out(&cmd);
	interrupt_wait();

	port_read(REG_DATA, hdbuf, SECTOR_SIZE);
	memcpy(entry,
	       hdbuf + PARTITION_TABLE_OFFSET,
	       sizeof(struct part_ent) * NR_PART_PER_DRIVE);
}

//PRIVATE void get_part_table_continue()
//{
//	port_read(REG_DATA, hdbuf, SECTOR_SIZE);
//	memcpy(entry,
//	       hdbuf + PARTITION_TABLE_OFFSET,
//	       sizeof(struct part_ent) * NR_PART_PER_DRIVE);
//}

/*****************************************************************************
 *                                partition
 *****************************************************************************/
/**
 * <Ring 1> This routine is called when a device is opened. It reads the
 * partition table(s) and fills the hd_info struct.
 * 
 * @param device Device nr.
 * @param style  P_PRIMARY or P_EXTENDED.
 *****************************************************************************/
PRIVATE void partition(int device, int style)
{
	//sys_printx("partition is called [");
	//sys_write_int_routine(device);
	//sys_printx("]\n");
	
	int i;
	//int drive = DRV_OF_DEV(device);
	int drive = TASK_HD; 
	
	//sys_printx("drive is [");
	//sys_write_int_routine(drive);
	//sys_printx("]\n");
	
	struct hd_info * hdi = &hd_info[drive];

	struct part_ent part_tbl[NR_SUB_PER_DRIVE];

	if (style == P_PRIMARY) {
		get_part_table(drive, drive, part_tbl);

		int nr_prim_parts = 0;
		for (i = 0; i < NR_PART_PER_DRIVE; i++) { /* 0~3 */
			if (part_tbl[i].sys_id == NO_PART) 
				continue;

			nr_prim_parts++;
			int dev_nr = i + 1;		  /* 1~4 */
			hdi->primary[dev_nr].base = part_tbl[i].start_sect;
			hdi->primary[dev_nr].size = part_tbl[i].nr_sects;

			if (part_tbl[i].sys_id == EXT_PART) /* extended */
				partition(device + dev_nr, P_EXTENDED);
		}
		//assert(nr_prim_parts != 0);
	}
	else if (style == P_EXTENDED) {
		int j = device % NR_PRIM_PER_DRIVE; /* 1~4 */
		int ext_start_sect = hdi->primary[j].base;
		int s = ext_start_sect;
		int nr_1st_sub = (j - 1) * NR_SUB_PER_PART; /* 0/16/32/48 */

		for (i = 0; i < NR_SUB_PER_PART; i++) {
			int dev_nr = nr_1st_sub + i;/* 0~15/16~31/32~47/48~63 */

			get_part_table(drive, s, part_tbl);

			hdi->logical[dev_nr].base = s + part_tbl[0].start_sect;
			hdi->logical[dev_nr].size = part_tbl[0].nr_sects;

			s = ext_start_sect + part_tbl[1].start_sect;

			/* no more logical partitions
			   in this extended partition */
			if (part_tbl[1].sys_id == NO_PART)
				break;
		}
	}
	else {
		//assert(0);
	}
}

/*****************************************************************************
 *                                print_hdinfo
 *****************************************************************************/
/**
 * <Ring 1> Print disk info.
 * 
 * @param hdi  Ptr to struct hd_info.
 *****************************************************************************/
PRIVATE void print_hdinfo(struct hd_info * hdi)
{
	int i;
	for (i = 0; i < NR_PART_PER_DRIVE + 1; i++) {
		//printl("%sPART_%d: base %d(0x%x), size %d(0x%x) (in sector)\n",
		//       i == 0 ? " " : "     ",
		//       i,
		//       hdi->primary[i].base,
		//       hdi->primary[i].base,
		//       hdi->primary[i].size,
		//       hdi->primary[i].size);
		//sys_printx("PART[");
		//sys_write_int_routine(i);
		//sys_printx("]: base [");
		//sys_write_int_routine(hdi->primary[i].base);
		//sys_printx("]: size [");
		//sys_write_int_routine(hdi->primary[i].size);
		//sys_printx("]\n");
	}
	for (i = 0; i < NR_SUB_PER_DRIVE; i++) {
		if (hdi->logical[i].size == 0)
			continue;
		//printl("         "
		//       "%d: base %d(0x%x), size %d(0x%x) (in sector)\n",
		//       i,
		//       hdi->logical[i].base,
		//       hdi->logical[i].base,
		//       hdi->logical[i].size,
		//       hdi->logical[i].size);
		//sys_printx("[");
		//sys_write_int_routine(i);
		//sys_printx("]: base [");
		//sys_write_int_routine(hdi->logical[i].base);
		//sys_printx("]: size [");
		//sys_write_int_routine(hdi->logical[i].size);
		//sys_printx("]\n");
	}
}

/*****************************************************************************
 *                                hd_identify
 *****************************************************************************/
/**
 * <Ring 1> Get the disk information.
 * 
 * @param drive  Drive Nr.
 *****************************************************************************/
PRIVATE void hd_identify(int drive)
{
	struct hd_cmd cmd;
	cmd.device  = MAKE_DEVICE_REG(0, drive, 0);
	cmd.command = ATA_IDENTIFY;
	hd_cmd_out(&cmd);
	//sys_printx("hd_cmd_out(&cmd) is called.\n");
	interrupt_wait();
	//sys_printx("interrupt_wait() is called.\n");
	port_read(REG_DATA, hdbuf, SECTOR_SIZE);
	//sys_printx("port_read() is called.\n");

	print_identify_info((u16*)hdbuf);
	//sys_printx("print_identify_info() is called.\n");
}

//PRIVATE void hd_identify_continue()
//{
	//drive = 0;
//	port_read(REG_DATA, hdbuf, SECTOR_SIZE);
	//sys_printx("port_read() is called.\n");
//	print_identify_info((u16*)hdbuf);
	//sys_printx("print_identify_info() is called.\n");
//}

/*****************************************************************************
 *                            print_identify_info
 *****************************************************************************/
/**
 * <Ring 1> Print the hdinfo retrieved via ATA_IDENTIFY command.
 * 
 * @param hdinfo  The buffer read from the disk i/o port.
 *****************************************************************************/
PRIVATE void print_identify_info(u16* hdinfo)
{
	int i, k;
	char s[64];

	struct iden_info_ascii {
		int idx;
		int len;
		char * desc;
	} iinfo[] = {{10, 20, "HD SN"}, /* Serial number in ASCII */
		     {27, 40, "HD Model"} /* Model number in ASCII */ };

	for (k = 0; k < sizeof(iinfo)/sizeof(iinfo[0]); k++) {
		char * p = (char*)&hdinfo[iinfo[k].idx];
		for (i = 0; i < iinfo[k].len/2; i++) {
			s[i*2+1] = *p++;
			s[i*2] = *p++;
		}
		s[i*2] = 0;
		//printl("%s: %s\n", iinfo[k].desc, s);
		sys_printx(iinfo[k].desc);
		sys_printx(s);
		sys_printx("\n");
	}

	int capabilities = hdinfo[49];
	
	//printl("LBA supported: %s\n",(capabilities & 0x0200) ? "Yes" : "No");
	if (capabilities & 0x0200){
		sys_printx("LBA supported: Yes\n");
	}
	else {
		sys_printx("LBA supported: No\n");
	}


	int cmd_set_supported = hdinfo[83];
	if (cmd_set_supported & 0x0400){
		sys_printx("LBA supported: Yes\n");
	}
	else {
		sys_printx("LBA supported: No\n");
	}
	//printl("LBA48 supported: %s\n",(cmd_set_supported & 0x0400) ? "Yes" : "No");

	int sectors = ((int)hdinfo[61] << 16) + hdinfo[60];
	sys_printx("HD size (MB): ");
	sys_write_int_routine(sectors * 512 / 1000000);
	sys_printx("\n");
	//printl("HD size: %dMB\n", sectors * 512 / 1000000);
}

/*****************************************************************************
 *                                hd_cmd_out
 *****************************************************************************/
/**
 * <Ring 1> Output a command to HD controller.
 * 
 * @param cmd  The command struct ptr.
 *****************************************************************************/
PRIVATE void hd_cmd_out(struct hd_cmd* cmd)
{
	/**
	 * For all commands, the host must first check if BSY=1,
	 * and should proceed no further unless and until BSY=0
	 */
	if (!waitfor(STATUS_BSY, 0, HD_TIMEOUT)){
		sys_printx("hd error");
	}

	/* Activate the Interrupt Enable (nIEN) bit */
	out_byte(REG_DEV_CTRL, 0);
	/* Load required parameters in the Command Block Registers */
	out_byte(REG_FEATURES, cmd->features);
	out_byte(REG_NSECTOR,  cmd->count);
	out_byte(REG_LBA_LOW,  cmd->lba_low);
	out_byte(REG_LBA_MID,  cmd->lba_mid);
	out_byte(REG_LBA_HIGH, cmd->lba_high);
	out_byte(REG_DEVICE,   cmd->device);
	/* Write the command code to the Command Register */
	out_byte(REG_CMD,     cmd->command);
}

/*****************************************************************************
 *                                interrupt_wait
 *****************************************************************************/
/**
 * <Ring 1> Wait until a disk interrupt occurs.
 * 
 *****************************************************************************/
PRIVATE void interrupt_wait()
{
	MESSAGE msg;
	//sys_printx("waiting for interrupt\n");
	sys_sendrec(RECEIVE, INTERRUPT, &msg);
	//sys_printx("interrupt received\n");
	//while (ctn_flag == 0) {
	//	for (int i = 0; i < 4096000; i++) {}
	//	sys_printx(".");
	//}
}

/*****************************************************************************
 *                                waitfor
 *****************************************************************************/
/**
 * <Ring 1> Wait for a certain status.
 * 
 * @param mask    Status mask.
 * @param val     Required status.
 * @param timeout Timeout in milliseconds.
 * 
 * @return One if sucess, zero if timeout.
 *****************************************************************************/
PRIVATE int waitfor(int mask, int val, int timeout)
{
	int t = 0;

	while(t < timeout)
		if ((in_byte(REG_STATUS) & mask) == val)
			return 1;
		t = t + 1;

	return 0;
}

/*****************************************************************************
 *                                hd_handler
 *****************************************************************************/
/**
 * <Ring 0> Interrupt handler.
 * 
 * @param irq  IRQ nr of the disk interrupt.
 *****************************************************************************/
PUBLIC void hd_handler()
{
	/*
	 * Interrupts are cleared when the host
	 *   - reads the Status Register,
	 *   - issues a reset, or
	 *   - writes to the Command Register.
	 */
	hd_status = in_byte(REG_STATUS);

	//sys_printx("in_byte() is called\n");
	inform_int(TASK_HD);
	//sys_printx("inform_int() is called\n");
}
