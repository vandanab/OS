/*
    File: page_table.C

    Author: Vandana Bachani
            Department of Computer Science
            Texas A&M University
    Date  : 02/16/2013

    Description: Basic Paging.

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/
#define KERNEL_READ_PAGE_NOT_PRESENT 0
#define KERNEL_READ_PROTECTION_FAULT 1
#define KERNEL_WRITE_PAGE_NOT_PRESENT 2
#define KERNEL_WRITE_PROTECTION_FAULT 3
#define USER_READ_PAGE_NOT_PRESENT 4
#define USER_READ_PROTECTION_FAULT 5
#define USER_WRITE_PAGE_NOT_PRESENT 6
#define USER_WRITE_PROTECTION_FAULT 7

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/
#include "page_table.H"
#include "paging_low.H"

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* P A G E - T A B L E  */
/*--------------------------------------------------------------------------*/

//Global/Static variables

/* THESE MEMBERS ARE COMMON TO ENTIRE PAGING SUBSYSTEM */
PageTable *PageTable::current_page_table; /* pointer to currently loaded page table object */
unsigned int PageTable::paging_enabled; /* is paging turned on (i.e. are addresses logical)? */
FramePool *PageTable::kernel_mem_pool; /* Frame pool for the kernel memory */
FramePool *PageTable::process_mem_pool; /* Frame pool for the process memory */
unsigned long PageTable::shared_size; /* size of shared address space */


//Constructor

/* Initializes a page table with a given location for the directory and the
	page table proper.
	NOTE: The PageTable object still needs to be stored somewhere! Probably it is best
	to have it on the stack, as there is no memory manager yet...
	NOTE2: It may also be simpler to create the first page table *before* paging
	has been enabled.
*/
PageTable::PageTable() {
	this->page_directory = (unsigned long *)(kernel_mem_pool->get_frame() * Machine::PAGE_SIZE);
	unsigned long *page_table = (unsigned long *)(kernel_mem_pool->get_frame() * PAGE_SIZE); //shifting by 12 bits by multiplying by PAGE_SIZE
	
	//map the first 4MB of memory
	unsigned long address = 0x0;
	for(unsigned int i = 0; i < ENTRIES_PER_PAGE; i++) {
		page_table[i] = address | 3; //attributes set to supervisor level, R/W, etc.
		address += PAGE_SIZE;
	}

	//fill the first entry of the page directory
	this->page_directory[0] = (unsigned long)page_table;
	this->page_directory[0] = this->page_directory[0] | 3;

	//fill the remaining ENTRIES_PER_PAGE - 1 entries as not-present
	for(unsigned int i = 1; i < ENTRIES_PER_PAGE; i++) {
		this->page_directory[0] = 0 | 2;
	}
}


//Static functions

/* Set the global parameters for the paging subsystem. */
void PageTable::init_paging(FramePool * _kernel_mem_pool,
													FramePool * _process_mem_pool,
													const unsigned long _shared_size) {
	kernel_mem_pool = _kernel_mem_pool;
	process_mem_pool = _process_mem_pool;
	shared_size = _shared_size;
}


/* Enable paging on the CPU. Typically, a CPU start with paging disabled, and
	memory is accessed by addressing physical memory directly. After paging is
	enabled, memory is addressed logically. */
void PageTable::enable_paging() {
	//write the page_directory address into CR3
	write_cr3((unsigned long)current_page_table->get_page_directory());

	//set paging bit in CR0 to 1
	write_cr0(read_cr0() | 0x80000000);
}


/* The page fault handler. */
void PageTable::handle_fault(REGS * _r) {
	unsigned int err_code = _r->err_code & 1;
	unsigned long fault_address = read_cr2();
	switch(err_code) {
		case KERNEL_READ_PAGE_NOT_PRESENT:
		case KERNEL_WRITE_PAGE_NOT_PRESENT:
			unsigned long page_directory_index = fault_address >> 22; //constify this
			unsigned long page_table_index = (fault_address & 0x003FFFFF) >> 12; //constify this
			unsigned long *page_directory = current_page_table->get_page_directory();
			if(page_directory[page_directory_index] && 1) { //constify this
				//page table entry is present in the directory
				unsigned long *page_table = (unsigned long *)page_directory[page_directory_index];
				if(page_table[page_table_index] && 1) {
					//page entry is present in the page table
					//why did the page fault occur? :P
				}
				else {
					//constify the 3 - PT entry control bits
					create_page_table_entry(page_table, page_table_index, fault_address, 3);
					//handle the fault by reading/writing to that location or do what is required
				}
			} else {
				create_page_table_entry(page_directory, page_directory_index, fault_address, 3);
				unsigned long *page_table = (unsigned long *)page_directory[page_directory_index];
				create_page_table_entry(page_table, page_table_index, fault_address, 3);
				//handle the fault by reading/writing to that location or do what is required
			}
			break;
	}
	/*
	switch(err_code) {
		case KERNEL_READ_PAGE_NOT_PRESENT:
			break;
		case KERNEL_READ_PROTECTION_FAULT:
			break;
		case KERNEL_WRITE_PAGE_NOT_PRESENT:
			break;
		case KERNEL_WRITE_PROTECTION_FAULT:
			break;
		case USER_READ_PAGE_NOT_PRESENT:
			break;
		case USER_READ_PROTECTION_FAULT:
			break;
		case USER_WRITE_PAGE_NOT_PRESENT:
			break;
		case USER_WRITE_PROTECTION_FAULT:
			break;
	}*/
}


//private static
void PageTable::create_page_table_entry(unsigned long *paging_entity_address,
																				unsigned long index,
																				unsigned long fault_address,
																				unsigned long control_bits) {
	unsigned long *requested_page_from_framepool;
	if(fault_address >= shared_size) {
		requested_page_from_framepool =
									(unsigned long *)(process_mem_pool->get_frame() * PAGE_SIZE);
	} else {
		requested_page_from_framepool =
									(unsigned long *)(kernel_mem_pool->get_frame() * PAGE_SIZE);
	}
	paging_entity_address[index] = (unsigned long)requested_page_from_framepool | control_bits;
}


//Public member functions
  
/* Makes the given page table the current table. This must be done once during system startup and whenever the address space is switched (e.g. during process switching). */
void PageTable::load() {
	current_page_table = this;
}


/* Get the private page_directory variable. */
unsigned long *PageTable::get_page_directory() {
	return this->page_directory;
}
