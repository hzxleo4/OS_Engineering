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

PUBLIC int sys_sendrec(int function, int src_dest, MESSAGE* m)
{
	TASK* p = current; 
	int ret = 0;
	int caller = proc2pid(p);
	MESSAGE* mla = (MESSAGE*)va2la(caller, m);
	mla->source = caller;

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
	}

	return 0;
}

/*****************************************************************************
 *				  va2la
 *****************************************************************************/
/**
 * Virtual addr --> Linear addr.
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
	pa = (pg_tasks[index + page_mid] & 0xFFFFF000) + page_low;
	u32 la = PAGE_OFFSET + pa; 
	return (void*)la;
}


PUBLIC void reset_msg(MESSAGE* p)
{
	memset(p, 0, sizeof(MESSAGE));
}


PRIVATE void block(TASK* p)
{
	schedule_new();
}

PRIVATE void unblock(TASK* p)
{
}

PRIVATE int deadlock(int src, int dest)
{
	TASK* p = &tasks[dest];
	while (1) {
		if (p->p_flags & SENDING) {
			if (p->p_sendto == src) {
				/* print the chain */
				p = &tasks[dest];
				do {
					p = &tasks[p->p_sendto];
				} while (p != &tasks[src]);

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

PRIVATE int msg_send(TASK* current, int dest, MESSAGE* m)
{
	TASK* sender = current;
	TASK* p_dest = &tasks[dest]; /* proc dest */

	/* check for deadlock here */
	if (deadlock(proc2pid(sender), dest)) {
	}

	if ((p_dest->p_flags & RECEIVING) && /* dest is waiting for the msg */
	    (p_dest->p_recvfrom == proc2pid(sender) ||
	     p_dest->p_recvfrom == ANY)) {
		phys_copy(va2la(dest, p_dest->p_msg),
			  va2la(proc2pid(sender), m),
			  sizeof(MESSAGE));
		p_dest->p_msg = 0;
		p_dest->p_flags &= ~RECEIVING; /* dest has received the msg */
		p_dest->p_recvfrom = NO_TASK;
		unblock(p_dest);
	}
	else { /* dest is not waiting for the msg */
		sender->p_flags |= SENDING;
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
	}

	return 0;
}

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

	if ((p_who_wanna_recv->has_int_msg) &&
	    ((src == ANY) || (src == INTERRUPT))) {
		/* There is an interrupt needs p_who_wanna_recv's handling and
		 * p_who_wanna_recv is ready to handle it.
		 */

		MESSAGE msg;
		reset_msg(&msg);
		msg.source = INTERRUPT;
		msg.type = HARD_INT;
		phys_copy(va2la(proc2pid(p_who_wanna_recv), m), &msg,
			  sizeof(MESSAGE));

		p_who_wanna_recv->has_int_msg = 0;
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
			while (p) {
				if (proc2pid(p) == src) { /* if p is the one */
					p_from = p;
					break;
				}
				prev = p;
				p = p->next_sending;
			}
		}
	}

	if (copyok) {
		/* It's determined from which proc the message will
		 * be copied. Note that this proc must have been
		 * waiting for this moment in the queue, so we should
		 * remove it from the queue.
		 */
		if (p_from == p_who_wanna_recv->q_sending) { /* the 1st one */
			p_who_wanna_recv->q_sending = p_from->next_sending;
			p_from->next_sending = 0;
		}
		else {
			prev->next_sending = p_from->next_sending;
			p_from->next_sending = 0;
		}
		phys_copy(va2la(proc2pid(p_who_wanna_recv), m),
			  va2la(proc2pid(p_from), p_from->p_msg),
			  sizeof(MESSAGE));

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
		block(p_who_wanna_recv);
	}

	return 0;
}


PUBLIC void inform_int(int task_nr)
{
	TASK* p = &tasks[task_nr];

	if ((p->p_flags & RECEIVING) && /* dest is waiting for the msg */
	    ((p->p_recvfrom == INTERRUPT) || (p->p_recvfrom == ANY))) {
		p->p_msg->source = INTERRUPT;
		p->p_msg->type = HARD_INT;
		p->p_msg = 0;
		p->has_int_msg = 0;
		p->p_flags &= ~RECEIVING; /* dest has received the msg */
		p->p_recvfrom = NO_TASK;
		unblock(p);
	}
	else {
		p->has_int_msg = 1;
	}
}