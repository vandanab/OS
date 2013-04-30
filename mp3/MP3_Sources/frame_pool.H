/*
    File: frame_pool.H

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 12/09/03

    Description: Management of the Free-Frame Pool.


*/

#ifndef _FRAME_POOL_H_                   // include file only once
#define _FRAME_POOL_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/
#define MAX_FRAMEPOOLS 2
#define BYTE_SIZE 8

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/
#include "console.H"
/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* F r a m e   P o o l  */
/*--------------------------------------------------------------------------*/

class FramePool {

private:
     /* -- DEFINE YOUR FRAME POOL DATA STRUCTURE(s) HERE. */
		 
		 //static variables
		 static const unsigned int LONG_SIZE_IN_BITS = sizeof(unsigned long) * BYTE_SIZE;

		 static FramePool* framepool_list[MAX_FRAMEPOOLS];
		 static unsigned int num_framepools;

		 unsigned long *_base_frame_no;
		 unsigned long *_nframes;
		 unsigned long *_info_frame_no;
		 unsigned long *bitmap;
     
		 //private functions - hope c++ inlining works
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
		 }

public:
  
   FramePool(unsigned long _base_frame_no,
             unsigned long _nframes,
             unsigned long _info_frame_no);
   /* Initializes the data structures needed for the management of this
      frame pool. This function must be called before the paging system
      is initialized.
      _base_frame_no is the frame number at the start of the physical memory
      region that this frame pool manages.
      _nframes is the number of frames in the physical memory region that this
      frame pool manages.
      e.g. If _base_frame_no is 16 and _nframes is 4, this frame pool manages
      physical frames numbered 16, 17, 18 and 19
      _info_frame_no is the frame number (within the directly mapped region) of
      the frame that should be used to store the management information of the
      frame pool. However, if _info_frame_no is 0, the frame pool is free to
      choose any frame from the pool to store management information.
      */

   unsigned long get_frame();
   /* Allocates a frame from the frame pool. If successful, returns the frame
    * number of the frame. If fails, returns 0. */

   void mark_inaccessible(unsigned long _base_frame_no,
                          unsigned long _nframes);
   /* Mark the area of physical memory as inaccessible. The arguments have the
    * same semanticas as in the constructor.
    */

	 void release(unsigned long _frame_no);
	 /* release memory for a frame no. from this frame pool. */

   static void release_frame(unsigned long _frame_no);
   /* Releases frame back to the given frame pool.
      The frame is identified by the frame number. 
      NOTE: This function is static because there may be more than one frame pool
      defined in the system, and it is unclear which one this frame belongs to.
      This function must first identify the correct frame pool and then call the frame
      pool's release_frame function. */
};
#endif
