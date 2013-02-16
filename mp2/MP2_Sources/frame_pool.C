/*
    File: frame_pool.C

    Author: Vandana Bachani
            Department of Computer Science
            Texas A&M University
    Date  : 02/14/2013

    Description: Management of the Free-Frame Pool.


*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/
#include "frame_pool.H"
#include "machine.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* F r a m e   P o o l  */
/*--------------------------------------------------------------------------*/

//Global Variables

FramePool* FramePool::framepool_list[MAX_FRAMEPOOLS];
unsigned int FramePool::num_framepools;


//Constructor
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

FramePool::FramePool(unsigned long _base_frame_no,
					unsigned long _nframes,
					unsigned long _info_frame_no) {
	if(_info_frame_no == 0) {
		_info_frame_no += _base_frame_no;
		FramePool::num_framepools = 0;
	}
	this->_base_frame_no = (unsigned long *)(_base_frame_no * Machine::PAGE_SIZE); //shifting by 12 bits by multiplying by PAGE_SIZE
	*(this->_base_frame_no) = _base_frame_no;
	this->_nframes = (unsigned long *)(this->_base_frame_no + 1);
	*(this->_nframes) = _nframes;
	this->_info_frame_no = (unsigned long *)(this->_nframes + 1);
	*(this->_info_frame_no) = _info_frame_no;
	this->bitmap = (unsigned long *)(this->_info_frame_no + 1);
	//init bitmap - all zeros
	for(unsigned long i = 0; i < _nframes; i++) {
		this->unset_bit(i);
	}
	//set the bit for frame information frame
	this->set_bit(_info_frame_no - _base_frame_no);
	
	framepool_list[num_framepools] = this;
	num_framepools++;
}

/* Allocates a frame from the frame pool. If successful, returns the frame
	* number of the frame. If fails, returns 0. */
unsigned long FramePool::get_frame() {
	unsigned long frame_no = 0, i;
	for(i = 0; i < *this->_nframes; i++) {
		if(this->is_set(i))
			continue;
		this->set_bit(i);
		frame_no = *this->_base_frame_no + i;
		break;
	}
	return frame_no;
}


/* Mark the area of physical memory as inaccessible. The arguments have the
	* same semanticas as in the constructor.
	*/
void FramePool::mark_inaccessible(unsigned long _base_frame_no, unsigned long _nframes) {
	unsigned long bit_index = _base_frame_no - *this->_base_frame_no;
	for(bit_index; bit_index < bit_index + _nframes; bit_index++) {
		this->set_bit(bit_index);
	}
}

	
/* release memory for a frame no. from this frame pool. */
void FramePool::release(unsigned long _frame_no) {
	if(_frame_no >= *this->_base_frame_no && _frame_no < (*this->_base_frame_no + *this->_nframes))
		this->unset_bit(_frame_no - *this->_base_frame_no);
}


/* Releases frame back to the given frame pool.
	The frame is identified by the frame number.
	NOTE: This function is static because there may be more than one frame pool
	defined in the system, and it is unclear which one this frame belongs to.
	This function must first identify the correct frame pool and then call the frame
	pool's release_frame function. */
void FramePool::release_frame(unsigned long _frame_no) {
	FramePool *fp;
	for(unsigned int i = 0; i < num_framepools; i++) {
		fp = framepool_list[i];
		//release can return a status deleted or not and then we can use that to break this loop.
		fp->release(_frame_no);
	}
}
