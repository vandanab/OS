/* 
    File: file_system.C

    Author: Vandana Bachani
            Department of Computer Science
            Texas A&M University
    Date  : 04/27/13

    Description: File System Implementation

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "file_system.H"
#include "console.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARD DECLARATIONS */ 
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* F i l e */
/*--------------------------------------------------------------------------*/

/* Constructor for the file handle. Set the 'current 
	position' to be at the beginning of the file. */
File::File() {
	this->file_size = 0;
	this->current_ptr = 0;
}

void File::init(unsigned int fid, FileSystem *fs) {
	this->file_id = fid;
	this->file_system = fs;
	this->start_block = fs->dir[fid].start_block;
}

/* Read _n characters from the file starting at the 
	current location and copy them in _buf. 
	Return the number of characters read. */
unsigned int File::Read(unsigned int _n, char * _buf) {
	int buf_index = 0;
	unsigned long i = this->start_block;
	unsigned int y = this->file_size - this->current_ptr;
	Console::puts("In Read - num bytes in file: ");
	Console::putui(y);
	Console::puts("\n");
	int num_bytes_remaining_to_read = y > _n ? _n : y;
	unsigned char *rbuf = new unsigned char[BLOCK_SIZE];
	while(num_bytes_remaining_to_read > 0) {
		unsigned long offset_in_block = (this->current_ptr % BLOCK_SIZE);
		int x = BLOCK_SIZE - offset_in_block;
		int bytes_to_read_in_current_block = num_bytes_remaining_to_read > x ? x : num_bytes_remaining_to_read;
		
		//find current pointer block
		unsigned long ith_block_in_list = (this->current_ptr/BLOCK_SIZE);
		while(ith_block_in_list > 0) {
			i = this->file_system->FAT[i];
			ith_block_in_list--;
		}
		this->file_system->disk->read(i, rbuf);
		
		memcpy(_buf+buf_index, rbuf+offset_in_block, bytes_to_read_in_current_block);
		buf_index += bytes_to_read_in_current_block;
		num_bytes_remaining_to_read -= bytes_to_read_in_current_block;
		this->current_ptr += bytes_to_read_in_current_block;
	}
	delete rbuf;
	return (y > _n ? _n : y);
}

/* Write _n characters to the file starting at the current 
	location, if we run past the end of file, we increase 
	the size of the file as needed. 
*/
unsigned int File::Write(unsigned int _n, char * _buf) {
	Console::puts("In Write\n");
	unsigned long prev_file_size = this->file_size;
	int num_bytes_remaining = _n;
	int buf_index = 0;
	unsigned long i = this->start_block;
	unsigned char *rbuf = new unsigned char[BLOCK_SIZE];
	
	while(num_bytes_remaining > 0) {
		unsigned long offset_in_block = (this->current_ptr % BLOCK_SIZE);
		int x = BLOCK_SIZE - offset_in_block;
		unsigned int bytes_to_write_in_current_block = num_bytes_remaining > x ? x : num_bytes_remaining;

		Console::puts("In Write - bytes_to_write_in_current_block: ");
		Console::putui(bytes_to_write_in_current_block);
		Console::puts("\n");

		if(this->is_current_ptr_in_allocated_block()) {
			//find current pointer block
			unsigned long ith_block_in_list = (this->current_ptr/BLOCK_SIZE);
			while(ith_block_in_list > 0) {
				i = this->file_system->FAT[i];
				ith_block_in_list--;
			}
			if(offset_in_block > 0) {
				this->file_system->disk->read(i, rbuf);
			}
		} else {
			unsigned long free_block;
			if(this->file_system->get_free_block(&free_block, i)) {
				i = free_block;
			} else {
				break;
			}
		}
		Console::puts("Block to write in: ");
		Console::putui((unsigned int)i);
		Console::puts("\n");

		memcpy(rbuf+offset_in_block, _buf+buf_index, bytes_to_write_in_current_block);
		//Console::putch(rbuf[0]);
		Console::puts("\n");

		this->file_system->disk->write(i, rbuf);
		buf_index += bytes_to_write_in_current_block;
		num_bytes_remaining -= bytes_to_write_in_current_block;
		this->current_ptr += bytes_to_write_in_current_block;
		if(this->current_ptr > this->file_size) {
			this->file_size = this->current_ptr;
			this->file_system->dir[this->file_id].size = this->file_size;
		}
	}
	delete rbuf;
	if(this->file_size > prev_file_size) {
		Console::puts("File size incremented to: ");
		Console::putui((unsigned int)this->file_size);
		Console::puts("\n");

		//TODO: write dir to disk
		this->file_system->write_dir_to_disk();

		//TODO: write FAT to disk if get_free_block doesnt write it
	}
	return (_n - num_bytes_remaining);
}

BOOLEAN File::is_current_ptr_in_allocated_block() {
	unsigned long num_allocated_blocks = this->file_size/BLOCK_SIZE;
	if(this->file_size == 0 || this->file_size % BLOCK_SIZE)
		num_allocated_blocks++;
	unsigned long current_block_no = this->current_ptr/BLOCK_SIZE;
	if(current_block_no < num_allocated_blocks)
		return TRUE;
	return FALSE;
}

/* Set the 'current position' at the beginning of the file. */
void File::Reset() {
	this->current_ptr = 0;
}

/* Erase the content of the file. Return any freed blocks. 
	Note: This function does not delete the file! It just erases its 
	content. */
void File::Rewrite() {
	unsigned long i = this->start_block;
	do {
		unsigned long next_block = this->file_system->FAT[i];
		this->file_system->FAT[i] = 0;
		i = next_block;
	} while(i != this->start_block);
	this->Reset();
	this->file_size = 0;
	this->file_system->dir[file_id].size = 0;
	//TODO: write dir to disk
	this->file_system->write_dir_to_disk();

	this->file_system->FAT[this->start_block] = this->start_block;
	//TODO: write FAT to disk
	this->file_system->write_FAT_to_disk();
}

/* Is the current location for the file at the end of the file? */
BOOLEAN File::EoF() {
	return (this->current_ptr == this->file_size);
}


/*--------------------------------------------------------------------------*/
/* F i l e S y s t e m  */
/*--------------------------------------------------------------------------*/

/* Just initializes local data structures. Does not connect to disk yet. */
FileSystem::FileSystem() {
	//initializing in-memory dir structure
	//cannot move to disk until a disk is mounted
	const int DIR_NODE_SIZE = sizeof(struct dir_node);
	for(int i = 0; i < MAX_NUM_OF_FILES; i++) {
		struct dir_node x;
		x.start_block = 0;
		x.size = 0;
		memcpy(&this->dir[i], &x, DIR_NODE_SIZE);
	}
}

/* Associates the file system with a disk. We limit ourselves to at most one 
	file system per disk. Returns TRUE if 'Mount' operation successful (i.e. there 
	is indeed a file system on the disk. */
BOOLEAN FileSystem::Mount(BlockingDisk * _disk) {
//BOOLEAN FileSystem::Mount(SimpleDisk * _disk) {
	//initialize disk
	this->disk = _disk;
	this->size = _disk->size();

	this->FAT_size = this->size/BLOCK_SIZE;
	int num_cells_per_block = BLOCK_SIZE/sizeof(long);
	int num_of_blocks_taken_by_FAT = this->FAT_size/num_cells_per_block;

	//file alloation should start after directory and FAT blocks.
	this->alloc_start_block = num_of_blocks_taken_by_FAT+1;

	//read dir from disk
	read_dir_from_disk();
	
	//read FAT from disk
	read_FAT_from_disk();
	Console::puts("In Mount - FAT size: ");
	Console::putui((unsigned int)this->FAT_size);
	Console::puts("\n");
	/*
	for(unsigned long i = this->alloc_start_block; i < this->alloc_start_block + 1; i++) {
		Console::putui((unsigned int)this->FAT[i]);
		Console::putch(' ');
	}
	Console::puts("\n");
	for(;;) {
	}
	*/

	return TRUE;
}

void FileSystem::read_dir_from_disk() {
	Console::puts("In read_dir_from_disk\n");
	const int DIR_NODE_SIZE = sizeof(struct dir_node);
	unsigned char *buf = new unsigned char[BLOCK_SIZE];
	this->disk->read(0, buf);
	int num_cells_per_block = BLOCK_SIZE/DIR_NODE_SIZE;
	for(int i = 0; i < num_cells_per_block; i++) {
		memcpy(&this->dir[i], buf, DIR_NODE_SIZE);
		buf += DIR_NODE_SIZE;
	}
	delete buf;
}

void FileSystem::read_FAT_from_disk() {
	const int LONG_SIZE = sizeof(unsigned long);
	int num_cells_per_block = BLOCK_SIZE/LONG_SIZE;
	int num_of_blocks_taken_by_FAT = this->FAT_size/num_cells_per_block;
	this->FAT = new unsigned long[this->FAT_size];
	unsigned char *buf = new unsigned char[BLOCK_SIZE];
	for(int i = 1; i <= num_of_blocks_taken_by_FAT; i++) {
		int buf_index = 0;
		this->disk->read(i, buf);
		for(int j = 0; j < num_cells_per_block; j++) {
			int fat_index = (i-1)*num_cells_per_block + j;
			memcpy(&this->FAT[fat_index], buf+buf_index, LONG_SIZE);
			buf_index += LONG_SIZE; 
		}
	}
	delete buf;
}

void FileSystem::write_dir_to_disk() {
	Console::puts("In write_dir_to_disk\n");
	const int DIR_NODE_SIZE = sizeof(struct dir_node);
	unsigned char *buf = new unsigned char[BLOCK_SIZE];
	for(int i = 0; i < MAX_NUM_OF_FILES; i++) {
		int index = i * DIR_NODE_SIZE;
		memcpy(buf+index, &this->dir[i], DIR_NODE_SIZE);
	}
	this->disk->write(0, buf);
	Console::puts("Write of dir to disk complete\n");
	delete buf;
}

void FileSystem::write_FAT_to_disk() {
	const int LONG_SIZE = sizeof(unsigned long);
	int num_cells_per_block = BLOCK_SIZE/sizeof(unsigned long);
	int num_of_blocks_taken_by_FAT = this->FAT_size/num_cells_per_block;
	unsigned char *buf = new unsigned char[BLOCK_SIZE];
	for(int i = 1; i <= num_of_blocks_taken_by_FAT; i++) {
		for(int j = 0; j < num_cells_per_block; j++) {
			int fat_index = (i-1) * num_cells_per_block + j;
			int index = j * LONG_SIZE;
			memcpy(buf+index, &this->FAT[fat_index], LONG_SIZE);
		}
		this->disk->write(i, buf);
	}
	Console::puts("Write FAT to disk complete\n");
	delete buf;
}

/* Wipes any file system from the given disk and installs a new, empty, file 
	system that supports up to _size Byte. */
BOOLEAN FileSystem::Format(BlockingDisk * _disk, unsigned int _size) {
//BOOLEAN FileSystem::Format(SimpleDisk * _disk, unsigned int _size) {
	FileSystem::clear_ds_blocks(_disk, _size);
}

void FileSystem::clear_ds_blocks(BlockingDisk *_disk, unsigned int _size) {
//void FileSystem::clear_ds_blocks(SimpleDisk *_disk, unsigned int _size) {
	int fat_size = (_size/BLOCK_SIZE);
	int num_cells_per_block = BLOCK_SIZE/sizeof(unsigned long);
	int num_of_blocks_taken_by_FAT = fat_size/num_cells_per_block;
	
	//0th block taken by dir

	unsigned char *buf = new unsigned char[BLOCK_SIZE];
	for(int i = 0; i < BLOCK_SIZE; i++) {
		buf[i] = (char)0;
	}
	for(int i = 0; i <= num_of_blocks_taken_by_FAT; i++) {
		_disk->write(i, buf);
	}
	delete buf;
}

/* Find file with given id in file system. If found, initialize the file 
	object and return TRUE. Otherwise, return FALSE. */
BOOLEAN FileSystem::LookupFile(int _file_id, File * _file) {
	if(this->dir[_file_id].start_block != 0) {
		_file->init(_file_id, this);
		return TRUE;
	}
	return FALSE;
}

/* Create file with given id in the file system. If file exists already, 
	abort and return FALSE. Otherwise, return TRUE. */
BOOLEAN FileSystem::CreateFile(int _file_id) {
	if(this->dir[_file_id].start_block != 0)
		return FALSE;
	unsigned long free_block;
	if(get_free_block(&free_block, 0)) {
		struct dir_node x;
		x.start_block = free_block;
		x.size = 0;
		memcpy(&this->dir[_file_id], &x, sizeof(struct dir_node));
		
		//TODO:write dir to disk
		this->write_dir_to_disk();
		return TRUE;
	}
	return FALSE;
}

/* Get a free block from FAT. Returns false if FAT is full. */
BOOLEAN FileSystem::get_free_block(unsigned long *block, unsigned long parent) {
	for(unsigned long i = this->alloc_start_block; i < this->FAT_size; i++) {
		if(this->FAT[i] == 0) {
			if(parent != 0) {
				this->FAT[i] = this->FAT[parent];
				this->FAT[parent] = i;
			} else {
				this->FAT[i] = i; //last block points to itself (if first block)
			}
			*block = i;
			//TODO:write FAT to disk
			this->write_FAT_to_disk();
			//or optimize and write to disk when the file write is complete
			Console::puts("In get_free_block - free_block: ");
			Console::putui((unsigned int)i);
			Console::puts(" ");
			Console::putui((unsigned int)this->FAT[i]);
			Console::puts("\n");
			return TRUE;
		}
	}
	return FALSE;
}

/* Delete file with given id in the file system and free any disk block 
	occupied by the file. */
BOOLEAN FileSystem::DeleteFile(int _file_id) {
	int i = this->dir[_file_id].start_block;
	//free blocks from FAT
	do {
		i = this->FAT[i];
		this->FAT[i] = 0;
	} while(i != this->dir[_file_id].start_block);

	//TODO:write FAT to disk
	this->write_FAT_to_disk();
	
	this->dir[_file_id].start_block = 0;
	this->dir[_file_id].size = 0;
	//TODO:write dir to disk
	this->write_dir_to_disk();
}
