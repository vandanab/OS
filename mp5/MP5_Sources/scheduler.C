/* 
    Author: Vandana Bachani
			Based on code by:
			R. Bettati
            Department of Computer Science
            Texas A&M University
			
			A thread scheduler.

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

#include "scheduler.H"
#include "console.H"
#include "blocking_disk.H"

/*--------------------------------------------------------------------------*/
/* SCHEDULER */
/*--------------------------------------------------------------------------*/

/* Setup the scheduler. This sets up the ready queue, for example.
	If the scheduler implements some sort of round-robin scheme, then
	the end_of_quantum handler is installed here as well. */
Scheduler::Scheduler() {
	this->head = NULL;
	this->disk = NULL;
}

/* Called by the currently running thread in order to give up the CPU.
	The scheduler selects the next thread from the ready queue to load onto 
	the CPU, and calls the dispatcher function defined in 'threads.h' to
	do the context switch. */
void Scheduler::yield() {
	//check if disk's last read/write is ready, the blocked thread needs to be resumed then from the disk's queue
	if(this->disk != NULL && this->disk->is_ready()) {
		this->disk->resume_from_queue();
	}

	if(this->head == NULL) {
		Console::puts("Error: Ready queue is NULL.\n");
	} else {
		//disable interrupts
		if(machine_interrupts_enabled())
			machine_disable_interrupts();
		
		Thread *the_one;
		the_one = this->head->thread_ptr;
		this->head = this->head->next;
		Thread::dispatch_to(the_one);
		
		//disable interrupts
		if(!machine_interrupts_enabled())
			machine_enable_interrupts();
	}
}

/* Add the given thread to the ready queue of the scheduler. This is called 
	for threads that were waiting for an event to happen, or that have 
	to give up the CPU in response to a preemption. */
void Scheduler::resume(Thread * _thread) {
	if(this->head == NULL) {
		this->head = new Node(_thread);
	} else {
		Node *p = this->head;
		while(p->next != NULL) {
			p = p->next;
		}
		p->next = new Node(_thread);
	}
}

/* Make the given thread runnable by the scheduler. This function is called 
	typically after thread creation. Depending on the 
	implementation, this may not entail more than simply adding the 
	thread to the ready queue (see scheduler_resume). */
void Scheduler::add(Thread * _thread) {
	//For now resume and add do the same thing.
	this->resume(_thread);
}

/* Remove the given thread from the scheduler in preparation for destruction 
	of the thread. */
void Scheduler::terminate(Thread * _thread) {
	_thread->free_resources();
	this->yield();
}

void Scheduler::end_of_quantum_handler_to_preempt() {
	Thread *_thread = Thread::CurrentThread();
	this->resume(_thread);
	this->yield();
}

void Scheduler::add_blocking_disk(BlockingDisk *_disk) {
	this->disk = _disk;
}
