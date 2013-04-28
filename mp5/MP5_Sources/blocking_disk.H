/*
     File        : blocking_disk.H

     Author      : Vandana Bachani
     Modified    : 04/20/2013

     Description : Unblocked READ/WRITE operations on a SimpleDisk. Extends SimpleDisk.
                   
*/

#ifndef _BLOCKING_DISK_H_
#define _BLOCKING_DISK_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "simple_disk.H"
#include "scheduler.H"
#include "thread.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/
/*
class Node {
public:
	Thread *thread_ptr;
	Node *next;
	
	Node(Thread *t) {
		this->thread_ptr = t;
		this->next = NULL;
	}
};*/

/*--------------------------------------------------------------------------*/
/* B l o c k i n g D i s k  */
/*--------------------------------------------------------------------------*/

class BlockingDisk : public SimpleDisk {
private:

	Scheduler *scheduler;

	static Node *blocked_thread_queue_head;
	/* Queue of blocked threads, blocked on a read/write operation to complete */

	void add_to_queue(Thread *_thread);

public:

   BlockingDisk(DISK_ID _disk_id, unsigned int _size, Scheduler *_scheduler); 
   /* Creates a BlockingDisk device with the given size connected to the MASTER or 
      SLAVE slot of the primary ATA controller.
      NOTE: We are passing the _size argument out of laziness. In a real system, we would
      infer this information from the disk controller.
			Calls the SimpleDisk Constructor and initializes BlockingDisk structures. */


     void wait_until_ready();
     /* Is called after each read/write operation to check whether the disk is
        ready to start transfering the data from/to the disk. */
     /* Override the SimpleDisk's wait_until_ready and the thread gives up the CPU
        and return to check later. */

		 void resume_from_queue();
		 /* Resume a thread from the blocked thread queue when the disk has completed the
		 		read/write. */
     
		 BOOLEAN is_ready();
     /* making the is_ready function public */

};

#endif
