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

PUBLIC void sys_init_hd()
{
	/* Get the number of drives from the BIOS data area */
	u8 * pNrDrives = (u8*)(0x475);
	enable_irq(CASCADE_IRQ);
	enable_irq(AT_WINI_IRQ);
	int i = 0;
	for (i = 0; i < (sizeof(hd_info) / sizeof(hd_info[0])); i++)
		memset(&hd_info[i], 0, sizeof(hd_info[0]));
	hd_info[0].open_cnt = 0;
}


PUBLIC void sys_hd_open(int device)
{
	sys_printx("\nsys_hd_open is called\n");
	int drive = TASK_HD;
	hd_identify(drive);

	if (hd_info[drive].open_cnt++ == 0) {
		partition(drive * (NR_PART_PER_DRIVE + 1), P_PRIMARY);
		print_hdinfo(&hd_info[drive]);
	}
}

PUBLIC void sys_hd_close(int device)
{
	int drive = TASK_HD;
	hd_info[drive].open_cnt--;
}

PUBLIC void sys_hd_rdwt(int io_type, int dev, u64 pos, int bytes, int proc_nr, void* buf)
{
	int drive = TASK_HD;
	/**
	 * We only allow to R/W from a SECTOR boundary:
	 */

	u32 sect_nr = (u32)(pos >> SECTOR_SIZE_SHIFT); /* pos / SECTOR_SIZE */
	int logidx = (dev - MINOR_hd1a) % NR_SUB_PER_DRIVE;
	sect_nr += dev < MAX_PRIM ?
		hd_info[drive].primary[dev].base :
		hd_info[drive].logical[logidx].base;


	struct hd_cmd cmd;
	cmd.features	= 0;
	cmd.count	= (bytes + SECTOR_SIZE - 1) >> 9;
	cmd.lba_low	= sect_nr & 0xFF;
	cmd.lba_mid	= (sect_nr >>  8) & 0xFF;
	cmd.lba_high	= (sect_nr >> 16) & 0xFF;
	cmd.device	= MAKE_DEVICE_REG(1, drive, (sect_nr >> 24) & 0xF);
	cmd.command	= (io_type == DEV_READ) ? ATA_READ : ATA_WRITE;
	hd_cmd_out(&cmd);
	

	int bytes_left = bytes;
	void * la = buf;

	while (bytes_left) {
		int bytes;
		if (bytes_left < SECTOR_SIZE) {
			bytes = bytes_left;
		} else {
			bytes = SECTOR_SIZE; 
		}
		if (io_type == DEV_READ) {
			interrupt_wait();
			port_read(REG_DATA, hdbuf, SECTOR_SIZE);
			phys_copy(la, hdbuf, bytes);
		}
		else {
			if (!waitfor(STATUS_DRQ, STATUS_DRQ, HD_TIMEOUT))
				sys_printx("hd writing error\n");
			port_write(REG_DATA, la, bytes);
			interrupt_wait();
		}
		bytes_left -= SECTOR_SIZE;
		la += SECTOR_SIZE;
	}
}															

PUBLIC void sys_hd_ioctl(int dev, int request, void* buf)
{
	int device = dev;
	//int drive = DRV_OF_DEV(device);
	int drive = TASK_HD; 

	struct hd_info * hdi = &hd_info[drive];

	if (request == DIOCTL_GET_GEO) {
		void * dst = buf;
		void * src = device < MAX_PRIM ? &hdi->primary[device] : &hdi->logical[(device - MINOR_hd1a) % NR_SUB_PER_DRIVE];
		phys_copy(dst, src, sizeof(struct part_info));
	}
	else {
		// do nothing
	}
}


PRIVATE void get_part_table(int drive, int sect_nr, struct part_ent * entry)
{
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

PRIVATE void partition(int device, int style)
{
	int i;
	int drive = TASK_HD; 
	
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
	}
}


PRIVATE void print_hdinfo(struct hd_info * hdi)
{
	int i;
	for (i = 0; i < NR_PART_PER_DRIVE + 1; i++) {
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
	interrupt_wait();
	port_read(REG_DATA, hdbuf, SECTOR_SIZE);

	print_identify_info((u16*)hdbuf);
}

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
		sys_printx(iinfo[k].desc);
		sys_printx(s);
		sys_printx("\n");
	}

	int capabilities = hdinfo[49];
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

	int sectors = ((int)hdinfo[61] << 16) + hdinfo[60];
	sys_printx("HD size (MB): ");
	sys_write_int_routine(sectors * 512 / 1000000);
	sys_printx("\n");
}

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

PRIVATE void interrupt_wait()
{
	MESSAGE msg;
	sys_sendrec(RECEIVE, INTERRUPT, &msg);
}

PRIVATE int waitfor(int mask, int val, int timeout)
{
	int t = 0;

	while(t < timeout)
		if ((in_byte(REG_STATUS) & mask) == val)
			return 1;
		t = t + 1;

	return 0;
}

PUBLIC void hd_handler()
{
	/*
	 * Interrupts are cleared when the host
	 *   - reads the Status Register,
	 *   - issues a reset, or
	 *   - writes to the Command Register.
	 */
	hd_status = in_byte(REG_STATUS);
	inform_int(TASK_HD);
}
