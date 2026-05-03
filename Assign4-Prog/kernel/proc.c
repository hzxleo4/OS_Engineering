#include "type.h"
#include "const.h"
#include "tty.h"
#include "console.h"
#include "string.h"
#include "global.h"
#include "proto.h"
#include "task.h"
#include "kernel.h"

PRIVATE void block(TASK* p);
PRIVATE void unblock(TASK* p);
PRIVATE int  msg_send(TASK* current, int dest, MESSAGE* m);
PRIVATE int  msg_receive(TASK* current, int src, MESSAGE* m);
PRIVATE int  msg_send2(TASK* current, int dest, MESSAGE* m);
PRIVATE int  msg_receive2(TASK* current, int src, MESSAGE* m);
PRIVATE int  deadlock(int src, int dest);
void timer_interrupt();

PUBLIC int proc2pid(TASK* p) 
{
	return p->pid; 
}

PUBLIC int sys_get_pid()
{
	return current->pid;
}

/*****************************************************************************
 *                                sys_sendrec
 *****************************************************************************/
/**
 * <Ring 0> The core routine of system call `syscall_sendrec'.
 * 
 * @param function SEND or RECEIVE
 * @param src_dest To/From whom the message is transferred.
 * @param m        Ptr to the MESSAGE body.
 * @param p        The caller proc.
 * 
 * @return Zero if success.
 *****************************************************************************/
PUBLIC int sys_sendrec(int function, int src_dest, MESSAGE* m)
{
	//assert(k_reenter == 0);	/* make sure we are not in ring0 */
	
	TASK* p = current; 
	//assert((src_dest >= 0 && src_dest < NR_TASKS) ||
	//       src_dest == ANY ||
	//       src_dest == INTERRUPT);
	
	//sys_printx("sys_sendrec is called\n");
	//sys_printx("pid ");
	//sys_write_int_routine(current->pid);
	//sys_printx("function");
	//sys_write_int_routine(function);
	//sys_printx("src_dest");
	//sys_write_int_routine(src_dest);
	//sys_printx("type");
	//sys_write_int_routine(m->type);
	//sys_printx("value");
	//sys_write_int_routine(m->RETVAL);

	int ret = 0;
	int caller = proc2pid(p);
	MESSAGE* mla = (MESSAGE*)va2la(caller, m);
	mla->source = caller;

	//assert(mla->source != src_dest);

	/**
	 * Actually we have the third message type: BOTH. However, it is not
	 * allowed to be passed to the kernel directly. Kernel doesn't know
	 * it at all. It is transformed into a SEND followed by a RECEIVE
	 * by `send_recv()'.
	 */
	if (function == SEND) {
		ret = msg_send(p, src_dest, m);
		if (ret != 0)
			return ret;
	}
	else if (function == RECEIVE) {
		ret = msg_receive(p, src_dest, m);
		if (ret != 0)
			return ret;
	}
	else {
		//panic("{sys_sendrec} invalid function: "
		//      "%d (SEND:%d, RECEIVE:%d).", function, SEND, RECEIVE);
	}

	return 0;
}

/*****************************************************************************
 *                                send_recv
 *****************************************************************************/
/**
 * <Ring 1~3> IPC syscall.
 *
 * It is an encapsulation of `syscall_sendrec',
 * invoking `syscall_sendrec' directly should be avoided
 *
 * @param function  SEND, RECEIVE or BOTH
 * @param src_dest  The caller's proc_nr
 * @param msg       Pointer to the MESSAGE struct
 * 
 * @return always 0.
 *****************************************************************************/
//PUBLIC int send_recv(int function, int src_dest, MESSAGE* msg)
//{
//	int ret = 0;

//	if (function == RECEIVE)
//		memset(msg, 0, sizeof(MESSAGE));

//	switch (function) {
//	case BOTH:
//		syscall_sendrec(SEND, src_dest, msg);
//		syscall_sendrec(RECEIVE, src_dest, msg);
//		break;
//	case SEND:
//	case RECEIVE:
//		syscall_sendrec(function, src_dest, msg);
//		break;
//	default:
		//assert((function == BOTH) ||
		//       (function == SEND) || (function == RECEIVE));
//		break;
//	}

//	return ret;
//}


/*****************************************************************************
 *				  va2la
 *****************************************************************************/
/**
 * <Ring 0~1> Virtual addr --> Linear addr.
 * 
 * @param pid  PID of the proc whose address is to be calculated.
 * @param va   Virtual address.
 * 
 * @return The linear address for the given virtual address.
 *****************************************************************************/
PUBLIC void* va2la(int pid, void* va)
{
	u32 virtual_address = (u32) va;
	u32 page_low = virtual_address & 0x0FFF;
	u32 page_mid = (virtual_address >> 12) & 0x03FF;
	u32 page_high = (virtual_address >> 22) & 0x03FF;
	u32 pa = 0x0;
	int index = pid * 1024;
	//if (pid == 0){
		//pa = (pg0_task0[page_mid] & 0xFFFFF000) + page_low;
	//}
	//else if (pid == 1){
	//	pa = (pg0_task1[page_mid] & 0xFFFFF000) + page_low;
	//}
	pa = (pg_tasks[index + page_mid] & 0xFFFFF000) + page_low;
	u32 la = PAGE_OFFSET + pa; 
	//u32 seg_base = PAGE_OFFSET;

	//u32 la = seg_base + (u32)va;

	return (void*)la;
}

/*****************************************************************************
 *                                reset_msg
 *****************************************************************************/
/**
 * <Ring 0~3> Clear up a MESSAGE by setting each byte to 0.
 * 
 * @param p  The message to be cleared.
 *****************************************************************************/
PUBLIC void reset_msg(MESSAGE* p)
{
	memset(p, 0, sizeof(MESSAGE));
}

/*****************************************************************************
 *                                block
 *****************************************************************************/
/**
 * <Ring 0> This routine is called after `p_flags' has been set (!= 0), it
 * calls `schedule()' to choose another proc as the `proc_ready'.
 *
 * @attention This routine does not change `p_flags'. Make sure the `p_flags'
 * of the proc to be blocked has been set properly.
 * 
 * @param p The proc to be blocked.
 *****************************************************************************/
PRIVATE void block(TASK* p)
{
	//assert(p->p_flags);
	schedule_new();
	//timer_interrupt();
}

/*****************************************************************************
 *                                unblock
 *****************************************************************************/
/**
 * <Ring 0> This is a dummy routine. It does nothing actually. When it is
 * called, the `p_flags' should have been cleared (== 0).
 * 
 * @param p The unblocked proc.
 *****************************************************************************/
PRIVATE void unblock(TASK* p)
{
	//assert(p->p_flags == 0);
}

/*****************************************************************************
 *                                deadlock
 *****************************************************************************/
/**
 * <Ring 0> Check whether it is safe to send a message from src to dest.
 * The routine will detect if the messaging graph contains a cycle. For
 * instance, if we have procs trying to send messages like this:
 * A -> B -> C -> A, then a deadlock occurs, because all of them will
 * wait forever. If no cycles detected, it is considered as safe.
 * 
 * @param src   Who wants to send message.
 * @param dest  To whom the message is sent.
 * 
 * @return Zero if success.
 *****************************************************************************/
PRIVATE int deadlock(int src, int dest)
{
	TASK* p = &tasks[dest];
	while (1) {
		if (p->p_flags & SENDING) {
			if (p->p_sendto == src) {
				/* print the chain */
				p = &tasks[dest];
				//printl("=_=%d", p->pid);
				do {
					//assert(p->p_msg);
					p = &tasks[p->p_sendto];
					//printl("->%d", p->pid);
				} while (p != &tasks[src]);
				//printl("=_=");

				return 1;
			}
			p = &tasks[p->p_sendto];
		}
		else {
			break;
		}
	}
	return 0;
}

/*****************************************************************************
 *                                msg_send
 *****************************************************************************/
/**
 * <Ring 0> Send a message to the dest proc. If dest is blocked waiting for
 * the message, copy the message to it and unblock dest. Otherwise the caller
 * will be blocked and appended to the dest's sending queue.
 * 
 * @param current  The caller, the sender.
 * @param dest     To whom the message is sent.
 * @param m        The message.
 * 
 * @return Zero if success.
 *****************************************************************************/
PRIVATE int msg_send(TASK* current, int dest, MESSAGE* m)
{
	TASK* sender = current;
	TASK* p_dest = &tasks[dest]; /* proc dest */

	//assert(proc2pid(sender) != dest);
	
	//disp_str("send called: ");
	//disp_str("pid");
	//disp_int(current->pid);
	//disp_str("dest");
	//disp_int(dest);
	//disp_str("message");
	//disp_int(m->type);

	/* check for deadlock here */
	if (deadlock(proc2pid(sender), dest)) {
		//panic(">>DEADLOCK<< %d->%d", sender->pid, p_dest->pid);
	}

	if ((p_dest->p_flags & RECEIVING) && /* dest is waiting for the msg */
	    (p_dest->p_recvfrom == proc2pid(sender) ||
	     p_dest->p_recvfrom == ANY)) {
		//assert(p_dest->p_msg);
		//assert(m);

		//sys_printx("dest is waiting, now process [");
		//sys_write_int_routine(sender->pid);
		//sys_printx("] send to [");
		//sys_write_int_routine(p_dest->pid);
		//sys_printx("]\n");
		//p_dest->p_msg->RETVAL = m->RETVAL;
		//sys_printx("m: ");
		//sys_write_int_routine(m->RETVAL);
		//sys_printx("p_msg: ");
		//sys_write_int_routine(p_dest->p_msg->RETVAL);
		//sys_printx("p_msg addr: ");
		//sys_write_int_routine(p_dest->p_msg);

		//sys_write_int_routine(va2la(dest, p_dest->p_msg));
		//sys_printx("\n");
		phys_copy(va2la(dest, p_dest->p_msg),
			  va2la(proc2pid(sender), m),
			  sizeof(MESSAGE));
		p_dest->p_msg = 0;
		p_dest->p_flags &= ~RECEIVING; /* dest has received the msg */
		p_dest->p_recvfrom = NO_TASK;
		unblock(p_dest);

		//assert(p_dest->p_flags == 0);
		//assert(p_dest->p_msg == 0);
		//assert(p_dest->p_recvfrom == NO_TASK);
		//assert(p_dest->p_sendto == NO_TASK);
		//assert(sender->p_flags == 0);
		//assert(sender->p_msg == 0);
		//assert(sender->p_recvfrom == NO_TASK);
		//assert(sender->p_sendto == NO_TASK);
	}
	else { /* dest is not waiting for the msg */
		//sys_printx("cannot send: ");
		//sys_printx("dest is not waiting, process [");
		//sys_write_int_routine(sender->pid);
		//sys_printx("] put to the sending list");
		//sys_printx("\n");
		sender->p_flags |= SENDING;
		//assert(sender->p_flags == SENDING);
		sender->p_sendto = dest;
		sender->p_msg = m;

		/* append to the sending queue */
		TASK * p;
		if (p_dest->q_sending) {
			p = p_dest->q_sending;
			while (p->next_sending)
				p = p->next_sending;
			p->next_sending = sender;
		}
		else {
			p_dest->q_sending = sender;
		}
		sender->next_sending = 0;

		block(sender);

		//assert(sender->p_flags == SENDING);
		//assert(sender->p_msg != 0);
		//assert(sender->p_recvfrom == NO_TASK);
		//assert(sender->p_sendto == dest);
	}

	return 0;
}


/*****************************************************************************
 *                                msg_receive
 *****************************************************************************/
/**
 * <Ring 0> Try to get a message from the src proc. If src is blocked sending
 * the message, copy the message from it and unblock src. Otherwise the caller
 * will be blocked.
 * 
 * @param current The caller, the proc who wanna receive.
 * @param src     From whom the message will be received.
 * @param m       The message ptr to accept the message.
 * 
 * @return  Zero if success.
 *****************************************************************************/
PRIVATE int msg_receive(TASK* current, int src, MESSAGE* m)
{
	TASK* p_who_wanna_recv = current; /**
						  * This name is a little bit
						  * wierd, but it makes me
						  * think clearly, so I keep
						  * it.
						  */
	TASK* p_from = 0; /* from which the message will be fetched */
	TASK* prev = 0;
	int copyok = 0;

	//assert(proc2pid(p_who_wanna_recv) != src);
	
	//disp_str("rec called: ");
	//disp_str("pid");
	//disp_int(current->pid);
	//disp_str("src");
	//disp_int(src);
	//disp_str("message");
	//disp_int(m->type);
	
	if ((p_who_wanna_recv->has_int_msg) &&
	    ((src == ANY) || (src == INTERRUPT))) {
		/* There is an interrupt needs p_who_wanna_recv's handling and
		 * p_who_wanna_recv is ready to handle it.
		 */

		MESSAGE msg;
		reset_msg(&msg);
		msg.source = INTERRUPT;
		msg.type = HARD_INT;
		//assert(m);
		phys_copy(va2la(proc2pid(p_who_wanna_recv), m), &msg,
			  sizeof(MESSAGE));

		p_who_wanna_recv->has_int_msg = 0;

		//assert(p_who_wanna_recv->p_flags == 0);
		//assert(p_who_wanna_recv->p_msg == 0);
		//assert(p_who_wanna_recv->p_sendto == NO_TASK);
		//assert(p_who_wanna_recv->has_int_msg == 0);

		return 0;
	}


	/* Arrives here if no interrupt for p_who_wanna_recv. */
	if (src == ANY) {
		/* p_who_wanna_recv is ready to receive messages from
		 * ANY proc, we'll check the sending queue and pick the
		 * first proc in it.
		 */
		if (p_who_wanna_recv->q_sending) {
			p_from = p_who_wanna_recv->q_sending;
			copyok = 1;

			//assert(p_who_wanna_recv->p_flags == 0);
			//assert(p_who_wanna_recv->p_msg == 0);
			//assert(p_who_wanna_recv->p_recvfrom == NO_TASK);
			//assert(p_who_wanna_recv->p_sendto == NO_TASK);
			//assert(p_who_wanna_recv->q_sending != 0);
			//assert(p_from->p_flags == SENDING);
			//assert(p_from->p_msg != 0);
			//assert(p_from->p_recvfrom == NO_TASK);
			//assert(p_from->p_sendto == proc2pid(p_who_wanna_recv));
		}
	}
	else {
		/* p_who_wanna_recv wants to receive a message from
		 * a certain proc: src.
		 */
		p_from = &tasks[src];

		if ((p_from->p_flags & SENDING) &&
		    (p_from->p_sendto == proc2pid(p_who_wanna_recv))) {
			/* Perfect, src is sending a message to
			 * p_who_wanna_recv.
			 */
			copyok = 1;

			TASK* p = p_who_wanna_recv->q_sending;
			//assert(p); /* p_from must have been appended to the
			//	    * queue, so the queue must not be NULL
			//	    */
			while (p) {
				//assert(p_from->p_flags & SENDING);
				if (proc2pid(p) == src) { /* if p is the one */
					p_from = p;
					break;
				}
				prev = p;
				p = p->next_sending;
			}

			//assert(p_who_wanna_recv->p_flags == 0);
			//assert(p_who_wanna_recv->p_msg == 0);
			//assert(p_who_wanna_recv->p_recvfrom == NO_TASK);
			//assert(p_who_wanna_recv->p_sendto == NO_TASK);
			//assert(p_who_wanna_recv->q_sending != 0);
			//assert(p_from->p_flags == SENDING);
			//assert(p_from->p_msg != 0);
			//assert(p_from->p_recvfrom == NO_TASK);
			//assert(p_from->p_sendto == proc2pid(p_who_wanna_recv));
		}
	}

	if (copyok) {
		/* It's determined from which proc the message will
		 * be copied. Note that this proc must have been
		 * waiting for this moment in the queue, so we should
		 * remove it from the queue.
		 */
		if (p_from == p_who_wanna_recv->q_sending) { /* the 1st one */
			//assert(prev == 0);
			p_who_wanna_recv->q_sending = p_from->next_sending;
			p_from->next_sending = 0;
		}
		else {
			//assert(prev);
			prev->next_sending = p_from->next_sending;
			p_from->next_sending = 0;
		}

		//assert(m);
		//assert(p_from->p_msg);
		/* copy the message */
		
		//sys_printx("src is waiting, now process [");
		//sys_write_int_routine(p_who_wanna_recv->pid);
		//sys_printx("] copy from [");
		//sys_write_int_routine(p_from->pid);
		//sys_printx("]\n");

		phys_copy(va2la(proc2pid(p_who_wanna_recv), m),
			  va2la(proc2pid(p_from), p_from->p_msg),
			  sizeof(MESSAGE));
		//m->type = p_from->p_msg->type; 

		p_from->p_msg = 0;
		p_from->p_sendto = NO_TASK;
		p_from->p_flags &= ~SENDING;
		unblock(p_from);
	}
	else {  /* nobody's sending any msg */
		/* Set p_flags so that p_who_wanna_recv will not
		 * be scheduled until it is unblocked.
		 */

		p_who_wanna_recv->p_flags |= RECEIVING;

		p_who_wanna_recv->p_msg = m;

		if (src == ANY)
			p_who_wanna_recv->p_recvfrom = ANY;
		else
			p_who_wanna_recv->p_recvfrom = proc2pid(p_from);

		//sys_printx("src is not waiting, now process [");
		//sys_write_int_routine(p_who_wanna_recv->pid);
		//sys_printx("] blocked");
		//sys_printx("\n");

		block(p_who_wanna_recv);

		//assert(p_who_wanna_recv->p_flags == RECEIVING);
		//assert(p_who_wanna_recv->p_msg != 0);
		//assert(p_who_wanna_recv->p_recvfrom != NO_TASK);
		//assert(p_who_wanna_recv->p_sendto == NO_TASK);
		//assert(p_who_wanna_recv->has_int_msg == 0);
	}

	return 0;
}

/*****************************************************************************
 *                                inform_int
 *****************************************************************************/
/**
 * <Ring 0> Inform a proc that an interrupt has occured.
 * 
 * @param task_nr  The task which will be informed.
 *****************************************************************************/
PUBLIC void inform_int(int task_nr)
{
	//sys_printx("inform_int is called[");
	//sys_write_int_routine(task_nr);
	//sys_printx("]\n");

	TASK* p = &tasks[task_nr];

	if ((p->p_flags & RECEIVING) && /* dest is waiting for the msg */
	    ((p->p_recvfrom == INTERRUPT) || (p->p_recvfrom == ANY))) {
		p->p_msg->source = INTERRUPT;
		p->p_msg->type = HARD_INT;
		p->p_msg = 0;
		p->has_int_msg = 0;
		p->p_flags &= ~RECEIVING; /* dest has received the msg */
		p->p_recvfrom = NO_TASK;
		//assert(p->p_flags == 0);
		unblock(p);

		//assert(p->p_flags == 0);
		//assert(p->p_msg == 0);
		//assert(p->p_recvfrom == NO_TASK);
		//assert(p->p_sendto == NO_TASK);
	}
	else {
		p->has_int_msg = 1;
	}
}

/*****************************************************************************
 *                                dump_proc
 *****************************************************************************/
//PUBLIC void dump_proc(TASK* p)
//{
//	char info[STR_DEFAULT_LEN];
//	int i;
//
//	int dump_len = sizeof(TASK);
//
//	out_byte(CRTC_ADDR_REG, START_ADDR_H);
//	out_byte(CRTC_DATA_REG, 0);
//	out_byte(CRTC_ADDR_REG, START_ADDR_L);
//	out_byte(CRTC_DATA_REG, 0);
//
//	sprintf(info, "byte dump of task_table[%d]:\n", p - tasks); disp_str(info);
//	for (i = 0; i < dump_len; i++) {
//		sprintf(info, "%x.", ((unsigned char *)p)[i]);
//		disp_str(info);
//	}
//
//	/* printl("^^"); */
//
//	disp_str("\n\n");
//	sprintf(info, "ANY: 0x%x.\n", ANY); disp_str(info);
//	sprintf(info, "NO_TASK: 0x%x.\n", NO_TASK); disp_str(info);
//	disp_str("\n");
//
//	sprintf(info, "pid: 0x%x.  ", p->pid); disp_str(info);
//	disp_str("\n");
//	sprintf(info, "p_flags: 0x%x.  ", p->p_flags); disp_str(info);
//	sprintf(info, "p_recvfrom: 0x%x.  ", p->p_recvfrom); disp_str(info);
//	sprintf(info, "p_sendto: 0x%x.  ", p->p_sendto); disp_str(info);
//	sprintf(info, "nr_tty: 0x%x.  ", p->nr_tty); disp_str(info);
//	disp_str("\n");
//	sprintf(info, "has_int_msg: 0x%x.  ", p->has_int_msg); disp_str(info);
//}


/*****************************************************************************
 *                                dump_msg
 *****************************************************************************/
//PUBLIC void dump_msg(const char * title, MESSAGE* m)
//{
//	int packed = 0;
//	printl("{%s}<0x%x>{%ssrc:%d(%d),%stype:%d,%s(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x)%s}%s",  //, (0x%x, 0x%x, 0x%x)}",
//	       title,
//	       (int)m,
//	       packed ? "" : "\n        ",
//	       tasks[m->source].pid,
//	       m->source,
//	       packed ? " " : "\n        ",
//	       m->type,
//	       packed ? " " : "\n        ",
//	       m->u.m3.m3i1,
//	       m->u.m3.m3i2,
//	       m->u.m3.m3i3,
//	       m->u.m3.m3i4,
//	       (int)m->u.m3.m3p1,
//	       (int)m->u.m3.m3p2,
//	       packed ? "" : "\n",
//	       packed ? "" : "\n"/* , */
//		);
//}