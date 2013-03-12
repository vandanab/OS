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
#define CONTROL_BITS_PAGE_PRESENT 3
#define CONTROL_BITS_PAGE_NOT_PRESENT 2


/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/
#include "page_table.H"
#include "paging_low.H"
#include "console.H"

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
	this->page_directory = (unsigned long *)(process_mem_pool->get_frame() * PAGE_SIZE);
	
	unsigned long *page_table = (unsigned long *)(process_mem_pool->get_frame() * PAGE_SIZE); //shifting by 12 bits by multiplying by PAGE_SIZE
	
	//map the first 4MB of memory
	unsigned long address = 0x0;
	for(unsigned int i = 0; i < ENTRIES_PER_PAGE; i++) {
		page_table[i] = address | 3; //attributes set to supervisor level, R/W, etc.
		address += PAGE_SIZE;
	}

	//fill the first entry of the page directory
	this->page_directory[0] = (unsigned long)page_table;
	this->page_directory[0] = this->page_directory[0] | CONTROL_BITS_PAGE_PRESENT;

	//fill the remaining ENTRIES_PER_PAGE - 1 entries as not-present
	for(unsigned int i = 1; i < ENTRIES_PER_PAGE - 1; i++) {
		this->page_directory[i] = 0 | CONTROL_BITS_PAGE_NOT_PRESENT;
	}

	//point the last entry to point to itself
	this->page_directory[ENTRIES_PER_PAGE - 1] = (unsigned long)(this->page_directory) | CONTROL_BITS_PAGE_PRESENT;

	this->num_registered_vmpools = 0;
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
/* TODO: change the handler code to first check if the page is legitimate
	by calling the function on the various registered vmpools. */
void PageTable::handle_fault(REGS * _r) {
	unsigned int err_code = _r->err_code & 1;
	unsigned long fault_address = read_cr2();
	//The pagetable has been allocated in the mapped memory so
	//page faults will have to be handled using logical addresses
	//instead of physical addresses.
	//After paging is enabled all addresses will be converted to logical
	//addresses.
	//We will use the self pointing entry in page_directory to make entries in
	//page_directory and page_table.
	switch(err_code) {
		//for now just handling the case of page faults where page is not present.
		case KERNEL_READ_PAGE_NOT_PRESENT:
		case KERNEL_WRITE_PAGE_NOT_PRESENT:
			unsigned long page_directory_index = (fault_address / (ENTRIES_PER_PAGE * PAGE_SIZE));
			unsigned long page_table_index = (fault_address / PAGE_SIZE) & 0x000003FF;
			//we shouldn't be using the page_directory address (as this is a physical
			//address which the MMU won't understand or map to something else.
			//unsigned long *page_directory = current_page_table->get_page_directory();
			
			//read pde
			unsigned long *page_directory_entry_ptr = get_page_directory_entry_address(page_directory_index);

			if(*page_directory_entry_ptr & 1) { //constify this
				//page table entry is present in the directory
				
				//we need to get to the page_table entry now using
				//logical addressing
				unsigned long *page_table_entry_ptr = get_page_table_entry_address(page_directory_index, page_table_index);
				if(*page_table_entry_ptr & 1) {
					//page entry is present in the page table
					//why did the page fault occur? :P
				}
				else {
					unsigned long *requested_page_from_framepool;
					if(fault_address >= shared_size) {
						requested_page_from_framepool =
							(unsigned long *)(process_mem_pool->get_frame() * PAGE_SIZE);
					} else {
						requested_page_from_framepool =
							(unsigned long *)(kernel_mem_pool->get_frame() * PAGE_SIZE);
					}
					
					create_page_table_entry(requested_page_from_framepool,
																	page_table_entry_ptr,
																	CONTROL_BITS_PAGE_PRESENT);

					//handle the fault by reading/writing to that location or do what is required

				}
			} else {
				//create page table
				unsigned long *requested_page_from_framepool =
									(unsigned long *)(process_mem_pool->get_frame() * PAGE_SIZE);

				create_page_table_entry(requested_page_from_framepool,
																page_directory_entry_ptr,
																CONTROL_BITS_PAGE_PRESENT);

				unsigned long *page_table_entry_ptr = get_page_table_entry_address(page_directory_index, page_table_index);
				
				if(fault_address >= shared_size) {
					requested_page_from_framepool =
							(unsigned long *)(process_mem_pool->get_frame() * PAGE_SIZE);
				} else {
					requested_page_from_framepool =
							(unsigned long *)(kernel_mem_pool->get_frame() * PAGE_SIZE);
				}

				create_page_table_entry(requested_page_from_framepool,
																page_table_entry_ptr,
																CONTROL_BITS_PAGE_PRESENT);
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


//public static
void PageTable::create_page_table_entry(unsigned long *requested_frame_from_framepool,
																				unsigned long *paging_entry_address,
																				unsigned long control_bits) {	
	*paging_entry_address = (unsigned long)requested_frame_from_framepool | control_bits;
}

unsigned long *PageTable::get_page_directory_entry_address(unsigned long page_directory_index) {
	page_directory_index = page_directory_index << 2; //convert to 12 bit address
	unsigned long *pde_ptr = 
		(unsigned long *)((ENTRIES_PER_PAGE - 1) * (ENTRIES_PER_PAGE * PAGE_SIZE) | (ENTRIES_PER_PAGE - 1) * (PAGE_SIZE) | page_directory_index);
	return pde_ptr;
}

unsigned long *PageTable::get_page_table_entry_address(unsigned long page_directory_index, unsigned long page_table_index) {
	page_table_index = page_table_index << 2; //convert to 12 bit address
	unsigned long *pte_ptr = 
		(unsigned long *)((ENTRIES_PER_PAGE - 1) * (ENTRIES_PER_PAGE * PAGE_SIZE) | (page_directory_index) * (PAGE_SIZE) | page_table_index);
	return pte_ptr;
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

void PageTable::free_page(unsigned long _page_no) {
	//free page
	//check if page no. is legitimate? I think there is no need as this is called by deallocate.
	//what if it is a page which has been allocated according to vmpool but not has been actually referenced and hence present in the page table?
	//hence the following logic...
	unsigned long page_directory_index = (_page_no / ENTRIES_PER_PAGE);
	unsigned long page_table_index = _page_no & 0x000003FF;
			
	//read pde
	unsigned long *page_directory_entry_ptr = get_page_directory_entry_address(page_directory_index);

	if(*page_directory_entry_ptr & 1) { //constify this
		unsigned long *page_table_entry_ptr = get_page_table_entry_address(page_directory_index, page_table_index);
		if(*page_table_entry_ptr & 1) {
			*page_table_entry_ptr = 0 | CONTROL_BITS_PAGE_NOT_PRESENT;
		}
	}
}

void PageTable::register_vmpool(VMPool *_pool) {
	this->registered_vmpools[this->num_registered_vmpools] = _pool;
	this->num_registered_vmpools++;
}
