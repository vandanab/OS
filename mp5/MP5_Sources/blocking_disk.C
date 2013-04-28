/*
     File        : blocking_disk.C

     Author      : Vandana Bachani
     Modified    : 04/20/2013

     Description : Unblocked READ/WRITE operations on a SimpleDisk. Extends SimpleDisk.
                   
*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#ifndef NULL
#define NULL 0L
#endif

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "blocking_disk.H"
#include "console.H"

/*--------------------------------------------------------------------------*/
/* B l o c k i n g D i s k  */
/*--------------------------------------------------------------------------*/

Node * BlockingDisk::blocked_thread_queue_head = NULL;

/* Creates a BlockingDisk device with the given size connected to the MASTER or 
	SLAVE slot of the primary ATA controller.
	NOTE: We are passing the _size argument out of laziness. In a real system, we would 
	infer this information from the disk controller.
	Calls the SimpleDisk Constructor. */

BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size, Scheduler *_scheduler) : SimpleDisk(_disk_id, _size) {
	this->scheduler = _scheduler;
}

/* Is called after each read/write operation to check whether the disk is
	ready to start transfering the data from/to the disk. */
/* Override the SimpleDisk's wait_until_ready and the thread gives up the CPU 
	and return to check later. */
void BlockingDisk::wait_until_ready() {
	if(!is_ready()) {
		Thread *th = Thread::CurrentThread();
		this->add_to_queue(th);
		this->scheduler->yield();
	}
}

void BlockingDisk::add_to_queue(Thread *_thread) {
	if(this->blocked_thread_queue_head == NULL) {
		this->blocked_thread_queue_head = new Node(_thread);
	} else {
		Node *p = this->blocked_thread_queue_head;
		while(p->next != NULL) {
			p = p->next;
		}
		p->next = new Node(_thread);
	}
}

void BlockingDisk::resume_from_queue() {
	if(this->blocked_thread_queue_head == NULL) {
		Console::puts("Error: Blocked queue is NULL.\n");
	} else {
		Thread *th = this->blocked_thread_queue_head->thread_ptr;
		this->blocked_thread_queue_head = this->blocked_thread_queue_head->next;
		this->scheduler->resume(th); 
	}
}

BOOLEAN BlockingDisk::is_ready() {
	return SimpleDisk::is_ready();
}
