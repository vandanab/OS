/* 
    File: file_system.H

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 10/04/05

    Description: File System.
    

*/

#ifndef _FILE_SYSTEM_H_                   // include file only once
#define _FILE_SYSTEM_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define BLOCK_SIZE 512
//#define MAX_NUM_OF_FILES 128
#define MAX_NUM_OF_FILES (BLOCK_SIZE/sizeof(struct dir_node))
//1 block for the directory map

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "utils.H"
//#include "blocking_disk.H"
#include "simple_disk.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

struct dir_node {
	unsigned long start_block;
	unsigned long size;
};

/*--------------------------------------------------------------------------*/
/* FORWARD DECLARATIONS */ 
/*--------------------------------------------------------------------------*/

class FileSystem;

/*--------------------------------------------------------------------------*/
/* F i l e */
/*--------------------------------------------------------------------------*/

class File {
private: 
     /* -- DEFINE YOUR FILE DATA STRUCTURES HERE. */
     
     FileSystem   * file_system;

     unsigned int   file_id;


     /* -- You may want to store other information, such as 
             .. position in the file
             .. cached block(s)
             .. etc.
      */
		 
		 unsigned long current_ptr; //index of byte in the file
		 unsigned long file_size;
		 unsigned long start_block;

		 BOOLEAN is_current_ptr_in_allocated_block();

public:

    File();
    /* Constructor for the file handle. Set the 'current
       position' to be at the beginning of the file. */

    unsigned int Read(unsigned int _n, char * _buf);
    /* Read _n characters from the file starting at the 
       current location and copy them in _buf.
       Return the number of characters read. */

    unsigned int Write(unsigned int _n, char * _buf);
    /* Write _n characters to the file starting at the current 
       location, if we run past the end of file, we increase 
       the size of the file as needed. 
     */

    void Reset();
    /* Set the 'current position' at the beginning of the file. */

    void Rewrite();
    /* Erase the content of the file. Return any freed blocks. 
       Note: This function does not delete the file! It just erases its
       content. */

    BOOLEAN EoF();
    /* Is the current location for the file at the end of the file? */
		 
		void init(unsigned int fid, FileSystem *fs);
		/* Initializes the file system and related attributes */

};

/*--------------------------------------------------------------------------*/
/* F i l e S y s t e m  */
/*--------------------------------------------------------------------------*/

class FileSystem {

friend class File;

private:
     /* -- DEFINE YOUR FILE SYSTEM DATA STRUCTURES HERE. */
		 unsigned long FAT_size; //basically no. of blocks in FS
		 unsigned long *FAT;
		 struct dir_node dir[MAX_NUM_OF_FILES];
		 unsigned long alloc_start_block;
     
     //BlockingDisk *disk;
     SimpleDisk *disk;
     unsigned int size;

		 void write_dir_to_disk();
		 void write_FAT_to_disk();
		 void read_dir_from_disk();
		 void read_FAT_from_disk();

		 BOOLEAN get_free_block(unsigned long *block, unsigned long parent);

		 //static void clear_ds_blocks(BlockingDisk *_disk, unsigned int _size);
		 static void clear_ds_blocks(SimpleDisk *_disk, unsigned int _size);
     
public:

   FileSystem();   
   /* Just initializes local data structures. Does not connect to disk yet. */

   //BOOLEAN Mount(BlockingDisk * _disk);
   BOOLEAN Mount(SimpleDisk * _disk);
   /* Associates the file system with a disk. We limit ourselves to at most one
      file system per disk. Returns TRUE if 'Mount' operation successful (i.e. there
      is indeed a file system on the disk. */

   //static BOOLEAN Format(BlockingDisk * _disk, unsigned int _size);
   static BOOLEAN Format(SimpleDisk * _disk, unsigned int _size);
   /* Wipes any file system from the given disk and installs a new, empty, file
      system that supports up to _size Byte. */

   BOOLEAN LookupFile(int _file_id, File * _file);
   /* Find file with given id in file system. If found, initialize the file 
      object and return TRUE. Otherwise, return FALSE. */

   BOOLEAN CreateFile(int _file_id);
   /* Create file with given id in the file system. If file exists already,
      abort and return FALSE. Otherwise, return TRUE. */

   BOOLEAN DeleteFile(int _file_id);
   /* Delete file with given id in the file system and free any disk block
      occupied by the file. */

   
};
#endif
