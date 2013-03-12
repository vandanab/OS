/*
    File: vm_pool.H

    Author: R. Bettati, Nikhil Gupta
            Department of Computer Science
            Texas A&M University
    Date  : 10/26/2010

    Description: Management of the Virtual Memory Pool


*/

#ifndef _VM_POOL_H_                   // include file only once
#define _VM_POOL_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "utils.H"
#include "frame_pool.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* Forward declaration of class PageTable */
class PageTable;

struct region {
	//24 bytes of storage per region - optimize this...
	unsigned long start_address;
	unsigned long size;
	unsigned long num_pages;
};

/*--------------------------------------------------------------------------*/
/* V M  P o o l  */
/*--------------------------------------------------------------------------*/

class VMPool { /* Virtual Memory Pool */
private:
   /* -- DEFINE YOUR VIRTUAL MEMORY POOL DATA STRUCTURE(s) HERE. */
	 //stack variables
	 static const unsigned int LONG_SIZE_IN_BITS = sizeof(unsigned long) * BYTE_SIZE; //BYTE_SIZE is defined in frame_pool.h
	 static const unsigned int REGION_DESC_SIZE = sizeof(struct region);
	 unsigned long _real_base_address;
	 unsigned long _base_address;
	 unsigned long _num_pages;
	 unsigned long _size;
	 unsigned long _remaining_size;
	 FramePool *_frame_pool;
	 PageTable *_page_table;
	 //assume that we create only limited regions.
	 //unsigned long *frames;
	 unsigned long num_regions;
	 
	 //frame variables
	 //unsigned long *bitmap;
	 struct region *regions;

	 //functions
	 //redundant code (we can make frame_pool.h more generic by making it static and accepting the bitmap (any bitmap) and making these functions public static.
	 /*
	 void unset_bit(unsigned long bit_no) {
		 unsigned int array_index = bit_no / LONG_SIZE_IN_BITS;
		 unsigned int bit_index = bit_no % LONG_SIZE_IN_BITS;
		 this->bitmap[array_index] &= ~((unsigned long) 1 << bit_index);
	 }
	 void set_bit(unsigned long bit_no) {
		 unsigned int array_index = bit_no / LONG_SIZE_IN_BITS;
		 unsigned int bit_index = bit_no % LONG_SIZE_IN_BITS;
		 this->bitmap[array_index] |= ((unsigned long) 1 << bit_index);
	 }
	 
	 bool is_set(unsigned long bit_no) {
		 unsigned int array_index = bit_no / LONG_SIZE_IN_BITS; //should be int or long?
		 unsigned int bit_index = bit_no % LONG_SIZE_IN_BITS;
		 unsigned long x = this->bitmap[array_index] & ((unsigned long) 1 << bit_index);
		 return x > 0;
	 }*/

public:
   VMPool(unsigned long _base_address,
          unsigned long _size,
          FramePool *_frame_pool,
          PageTable *_page_table);
   /* Initializes the data structures needed for the management of this
    * virtual-memory pool.
    * _base_address is the logical start address of the pool.
    * _size is the size of the pool in bytes.
    * _frame_pool points to the frame pool that provides the virtual
    * memory pool with physical memory frames.
    * _page_table points to the page table that maps the logical memory
    * references to physical addresses. */

   unsigned long allocate(unsigned long _size);
   /* Allocates a region of _size bytes of memory from the virtual
    * memory pool. If successful, returns the virtual address of the
    * start of the allocated region of memory. If fails, returns 0. */

   void release(unsigned long _start_address);
   /* Releases a region of previously allocated memory. The region
    * is identified by its start address, which was returned when the
    * region was allocated. */

   BOOLEAN is_legitimate(unsigned long _address);
   /* Returns FALSE if the address is not valid. An address is not valid
    * if it is not part of a region that is currently allocated. */
};

#endif
