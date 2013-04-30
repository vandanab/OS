/*
    File: vm_pool.C

    Author: Vandana Bachani
            Department of Computer Science
            Texas A&M University
    Date  : 03/02/2013

    Description: Management of the Virtual Memory Pool


*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "vm_pool.H"
#include "page_table.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* V M  P o o l  */
/*--------------------------------------------------------------------------*/

/* Initializes the data structures needed for the management of this
 * virtual-memory pool.
 * _base_address is the logical start address of the pool.
 * _size is the size of the pool in bytes.
 * _frame_pool points to the frame pool that provides the virtual
 * memory pool with physical memory frames.
 * _page_table points to the page table that maps the logical memory
 * references to physical addresses. */
VMPool::VMPool(unsigned long _base_address,
          unsigned long _size,
          FramePool *_frame_pool,
          PageTable *_page_table) {

	this->_real_base_address = _base_address;
	this->_base_address = _base_address + PageTable::PAGE_SIZE;
	this->_size = _size;
	this->_remaining_size = _size;
	this->_num_pages = _size / PageTable::PAGE_SIZE;
	if(_size % PageTable::PAGE_SIZE > 0) {
		this->_num_pages++;
	}
	this->_frame_pool = _frame_pool;
	this->_page_table = _page_table;

	//vmpool information such as list of regions needs to be maintained on a frame.
	//when we say framepool.get_frame(), it will return a physical frame no. which we cannot access due to logical address problem.
	//how to resolve this?
	//assuming if we store the physical address in the variable we can refer to its content as *address.
	//wrong assumption as the paging mechanism is in place already.	

	//next approach is to keep aside some space in the virtual memory pool for information.
	// we can ask the page table to make an entry for a a given frame.
	unsigned long page_directory_index = (this->_real_base_address / (PageTable::ENTRIES_PER_PAGE*PageTable::PAGE_SIZE));
	unsigned long page_table_index = ((this->_real_base_address / PageTable::PAGE_SIZE) & 0x000003FF);
			
	//read pde
	unsigned long *page_directory_entry_ptr = _page_table->get_page_directory_entry_address(page_directory_index);

	if(!(*page_directory_entry_ptr & 1)) { //constify this
		_page_table->create_page_table_entry((unsigned long *)(_frame_pool->get_frame()*PageTable::PAGE_SIZE), page_directory_entry_ptr, 3);
	}
	Console::puts("creating page table entry\n");
	unsigned long *page_table_entry_ptr = _page_table->get_page_table_entry_address(page_directory_index, page_table_index);
	_page_table->create_page_table_entry((unsigned long *)(_frame_pool->get_frame()*PageTable::PAGE_SIZE), page_table_entry_ptr, 3);

	//bitmap wont work as the VM Pool is very big and bitmap itself is taking the whole page. :(
	/*
	this->bitmap = (unsigned long *)(this->_real_base_address);
	this->set_bit(0);
	for(unsigned long i = 1; i < this->_num_pages; i++) {
		this->unset_bit(i);
	}
	Console::puts("location written\n");
	unsigned long bitmap_size = this->_num_pages / LONG_SIZE_IN_BITS;
	if(this->_num_pages % LONG_SIZE_IN_BITS > 0)
		bitmap_size++;
	this->regions = (struct region *)(this->bitmap + bitmap_size);
	*/
	this->regions = (struct region *)(this->_real_base_address);
	this->num_regions = 0;
}

/* Allocates a region of _size bytes of memory from the virtual
 * memory pool. If successful, returns the virtual address of the
 * start of the allocated region of memory. If fails, returns 0. */
unsigned long VMPool::allocate(unsigned long _size) {
	if(this->_remaining_size < _size)
		return 0;
	unsigned long i;
	unsigned long num_pages = _size / PageTable::PAGE_SIZE;
	if(_size % PageTable::PAGE_SIZE > 0)
		num_pages++;
	//unsigned long region_begin = 0;
	unsigned int found = 0;
	/*
	for(unsigned long i = 0; i < this->_num_pages; i++) {
		if(this->is_set(i))
			continue;
		unsigned long potential_region_begin = i, free_pages = 0;
		while(free_pages < num_pages && i < this->_num_pages && !this->is_set(i)) {
			free_pages++;
			i++;
		}
		if(free_pages == num_pages) {
			region_begin = potential_region_begin;
			found = 1;
			break;
		}
	}*/
	unsigned long potential_region_begin = this->_base_address;
	for(i = 0; i < this->num_regions; i++) {
		if(this->regions[i].start_address > potential_region_begin) {
			if(this->regions[i].start_address > (potential_region_begin + num_pages * PageTable::PAGE_SIZE)) {
				found = 1; break;
			}
		}
		else {
			potential_region_begin = this->regions[i].start_address + (this->regions[i].num_pages * PageTable::PAGE_SIZE);
		}
	}

	unsigned long vmpool_boundry = this->_real_base_address + (this->_num_pages * PageTable::PAGE_SIZE);
	unsigned long region_boundry = potential_region_begin + (num_pages * PageTable::PAGE_SIZE);

	if(found || region_boundry <= vmpool_boundry) {
		struct region x;
		//x.start_address = (unsigned long)(this->_base_address + (region_begin * PageTable::PAGE_SIZE));
		x.start_address = (unsigned long)(potential_region_begin);
		x.size = _size;
		x.num_pages = num_pages;
		for(unsigned long j = this->num_regions; j > i; j--) {
			this->regions[j] = this->regions[j-1];
		}
		this->regions[i] = x;
		//this->regions[this->num_regions] = x;
		this->num_regions++;
		/*
		for(i = region_begin; i < num_pages; i++) {
			this->set_bit(i);
		}*/
		this->_remaining_size -= _size;
		return (unsigned long)x.start_address;
	}
	return 0;
}

/* Releases a region of previously allocated memory. The region
 * is identified by its start address, which was returned when the
 * region was allocated. */
void VMPool::release(unsigned long _start_address) {
	unsigned long page_no = _start_address / PageTable::PAGE_SIZE;
	unsigned long region_no = 0;
	int found = 0;
	for(unsigned long i = 0; i < this->num_regions; i++) {
		if(this->regions[i].start_address == _start_address) {
			found = 1; region_no = i; break;
		}
		else
			continue;
	}
	if(found) {
		unsigned long num_pages = this->regions[region_no].num_pages;
		for(unsigned long i = page_no; i < num_pages; i++) {
			//this->unset_bit(i);
			this->_page_table->free_page(i);
		}
		this->_remaining_size += this->regions[region_no].size;
		for(unsigned long i = region_no; i < num_regions-1; i++) {
			this->regions[i] = this->regions[i+1];
		}
		this->num_regions--;
	}
}

/* Returns FALSE if the address is not valid. An address is not valid
 * if it is not part of a region that is currently allocated. */
BOOLEAN VMPool::is_legitimate(unsigned long _address) {
	//currently the logic doesn't take into account whether an address belonging to a page is valid in terms of the size allocated to the region.
	//for that we will have to look for the region and also make sure the address is covered in the region's size.
	for(unsigned long i = 0; i < this->num_regions; i++) {
		unsigned long region_boundry = this->regions[i].start_address + this->regions[i].size;
		if(_address >= this->regions[i].start_address && _address <= region_boundry) {
			return 1;
		}
	}
	/*
	if(page_no >= this->_base_address && page_no < (this->_base_address + this->_num_pages)) {
		return this->is_set(page_no - this->_base_address);
	}*/
	return 0;
}
