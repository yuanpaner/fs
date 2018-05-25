#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // strcmp

#include <stdint.h> //Integers
#include "disk.h"
#include "fs.h"


//in header
/** Maximum filename length (including the NULL character) */
// #define FS_FILENAME_LEN 16

/** Maximum number of files in the root directory */
// #define FS_FILE_MAX_COUNT 128 // entry

/** Maximum number of open files */
// #define FS_OPEN_MAX_COUNT 32

// #define FS_NAME "ECS150FS"

// 需要一个global var存储the currently mounted file system.

const static char FS_NAME[8] = "ECS150FS";

#define eprintf(format, ...) \
    fprintf (stderr, format, ##__VA_ARGS__)
#define oprintf(format, ...) \
    fprintf (stdout, format, ##__VA_ARGS__)
/* Superbock
* Holds statistics about the filesystem 
* according to the requirment 8 + 2 + 2 + 2 + 2 + 1 + 4079 = 4096; one block

Offset  Length (bytes)  Description
0x00    8   Signature (must be equal to “ECS150FS”)
0x08    2   Total amount of blocks of virtual disk
0x0A    2   Root directory block index
0x0C    2   Data block start index
0x0E    2   Amount of data blocks
0x10    1   Number of blocks for FAT
0x11    4079    Unused/Padding


* FAT
The FAT is a flat array, possibly spanning several blocks, which entries are composed of 16-bit unsigned words. There are as many entries as data blocks in the disk.

* Root Directory 
Offset  Length (bytes)  Description
0x00    16  Filename (including NULL character)
0x10    4   Size of the file (in bytes)
0x14    2   Index of the first data block
0x16    10  Unused/Padding
*/

struct SuperBlock 
{
	// uint64_t signature; // 8 bytes, "ECS150FS"
    char     signature[8];
	uint16_t total_blk_count;  // Total amount of blocks of virtual disk 
	uint16_t rdir_blk;         // Root directory block index
	uint16_t data_blk;         // Data block start index
	uint16_t data_blk_count;   // Amount of data blocks
	uint8_t  fat_blk_count;    // Number of blocks for FAT
    char     unused[4079];     // 4079 Unused/Padding
}__attribute__((packed));


struct RootDirEntry {                // Inode structure
    char        filename[FS_FILENAME_LEN];         // Whether or not inode is valid
    uint32_t    file_sz;          // Size of file
    uint16_t    first_data_blk; // Direct pointers
    // uint8_t     frentry_idx;  //10 byte Unused/Padding
    uint8_t     open;
    char        unused[2];     
}__attribute__((packed));

struct FileDescriptor
{
    void * dir_entry;
};

union Block {
    struct SuperBlock   super;              // Superblock
    uint16_t            fat[BLOCK_SIZE/2];        
    struct RootDirEntry rootdir[FS_FILE_MAX_COUNT];   // Pointer block
    char                block[BLOCK_SIZE];     // Data block
};



char * disk = NULL;
struct SuperBlock * sp = NULL;
void * root_dir = NULL;
struct RootDirEntry * dir_entry = NULL; // 32B * 128 entry
void * fat = NULL;
uint16_t * fat16 = NULL;

int fd = 0; 
struct FileDescriptor filedes[FS_OPEN_MAX_COUNT];

struct RootDirEntry * get_dir(int id){
    if(root_dir == NULL) return NULL;
    return (struct RootDirEntry *)(root_dir + id * sizeof(struct RootDirEntry));
}

uint16_t * get_fat(int id){
    if(fat == NULL) return NULL;
    return (uint16_t *)(fat + sizeof(uint16_t) * id);
}

int get_freeEntry_idx(const char * filename){
    if(root_dir == NULL || sp == NULL)
        return -1;
    int i;
    for (i = 0; i < FS_FILE_MAX_COUNT; ++i)
    {
        struct RootDirEntry * tmp = root_dir + i * sizeof(struct RootDirEntry);
        if(strcmp(tmp->filename, filename) == 0){
            eprintf("fs_create: @filename already exists error\n");
            return -1;
        }
        if(tmp->filename == NULL)
            break;
    }
    if(i == FS_FILE_MAX_COUNT){
        eprintf("fs_create: root directory full error\n");
        return -1;
    }
    return i;
}
int get_freeFat_idx(){
    if(fat == NULL || sp == NULL)
        return -1;
    int i = 1;
    uint16_t * tmp;
    for (tmp = fat; i < sp->fat_blk_count * BLOCK_SIZE / 2 ; ++i, tmp += sizeof( uint16_t ))
        if (*tmp == 0 )
            return i;
    if( i == sp->fat_blk_count * BLOCK_SIZE / 2)
        eprintf("fat exhausted\n");

    return -1;

}

int get_dirEntry_idx(const char * filename){
    if(filename == NULL || root_dir == NULL || sp == NULL)
        return -1;
    int i;
    for (i = 0; i < FS_FILE_MAX_COUNT; ++i)
    {
        // struct RootDirEntry * tmp = root_dir + i * sizeof(struct RootDirEntry);
        struct RootDirEntry * tmp = get_dir(i);

        if(strcmp(tmp->filename, filename) == 0)
            return i;

        // if(strcmp(dir[i]->filename, filename) == 0){
        //     return i;
        // }
    }
    if(i == FS_FILE_MAX_COUNT){
        eprintf("get_firstEntry_idx: not found\n");
        return -1;
    }

    return -1;
}

int erase_fat(uint16_t * id){ // recursion to erase
    if(sp == NULL || root_dir == NULL)
        return -1;

    if(*id == 0xFFFF){
        *id = 0;
        return 0;
    }

    uint16_t * next = fat + sizeof(uint16_t) * (*id);
    erase_fat(next);
    *id = 0;

    return 0;
}



/* TODO: Phase 1 */

/**
 * fs_mount - Mount a file system
 * @diskname: Name of the virtual disk file
 *
 * Open the virtual disk file @diskname and mount the file system that it
 * contains. A file system needs to be mounted before files can be read from it
 * with fs_read() or written to it with fs_write().
 *
 * Return: -1 if virtual disk file @diskname cannot be opened, or if no valid
 * file system can be located. 0 otherwise.
 */
 /* http://www.cs.ucsb.edu/~chris/teaching/cs170/projects/proj5.html
With the mount operation, a file system becomes "ready for use." 
You need to open the disk and then load the meta-information that is necessary to handle the file system operations that are discussed below. 
 */
int fs_mount(const char *diskname)
{
	/* TODO: Phase 1 */
	if (block_disk_open(diskname) != 0) return -1;
	

    sp = malloc(BLOCK_SIZE); // struct SuperBlock * super = malloc(BLOCK_SIZE); malloc(sizeof(struct SuperBlock))
    memset(sp, 0, BLOCK_SIZE);
    if(block_read(0, (void *)sp) < 0) 
        return -1;

    root_dir = malloc(BLOCK_SIZE);
    memset(root_dir, 0, BLOCK_SIZE);
    if(block_read(sp->rdir_blk, root_dir) < 0){
        eprintf("fs_mount read root dir error\n");
        return -1;
    }
    // dir_entry = (struct RootDirEntry *)root_dir;
    dir_entry = get_dir(0);


    fat = malloc(BLOCK_SIZE * sp->fat_blk_count);
    memset(fat, 0, BLOCK_SIZE * sp->fat_blk_count);
    for (int i = 0; i < sp->fat_blk_count; ++i)
    {
        if(block_read(i+1, fat + BLOCK_SIZE * i) < 0){
            eprintf("fs_mount read %d th(from 1) fat block error\n", i);
            return -1;
        }
    }
    fat16 = get_fat(0);
    

    // memcpy(sp->signature,FS_NAME,8);
    // sp->total_blk_count = block_disk_count();
    // sp->fat_blk_count = sp->total_blk_count % BLOCK_SIZE - 2;
    // sp->rdir_blk = sp->fat_blk_count + 1;
    // sp->data_blk = sp->rdir_blk + 1;
    // sp->data_blk_count = block_disk_count() / BLOCK_SIZE * BLOCK_SIZE;

    // eprintf("signature=%s\n",sp->signature);
    // eprintf("total_blk_count=%d\n",sp->total_blk_count);
    // eprintf("fat_blk_count=%d\n",sp->fat_blk_count);
    // eprintf("rdir_blk=%d\n",sp->rdir_blk);
    // eprintf("data_blk=%d\n",sp->data_blk);
    // eprintf("data_blk_count=%d\n",sp->data_blk_count);
    
    // if(block_write(0, (void *)sp) < 0) return -1;

    // block_disk_close();

	return 0;
}

/**
 * fs_umount - Unmount file system
 *
 * Unmount the currently mounted file system and close the underlying virtual
 * disk file.
 *
 * Return: -1 if no underlying virtual disk was opened, or if the virtual disk
 * cannot be closed, or if there are still open file descriptors. 0 otherwise.
 */

/**
This function unmounts your file system from a virtual disk with name disk_name.  

need to write back all meta-information so that the disk persistently reflects all changes that were made to the file system (such as new files that are created, data that is written, ...). 

You should also close the disk. The function returns 0 on success, and -1 when the disk disk_name could not be closed or when data could not be written to the disk (this should not happen).

It is important to observe that your file system must provide persistent storage. 
This means that whenever umount_fs is called, all meta-information and file data (that you could temporarily have only in memory; depending on your implementation) must be written out to disk.

*/ 
int fs_umount(void)
{
    /* TODO: Phase 1 */
	/* write back: super block, fat, dir*/
    if(block_write(0, (void *)sp) < 0)
    {
        eprintf("fs_umount write back sp error\n");
        return -1; 
    }
    if(block_write(sp->rdir_blk, root_dir) < 0)// write back
    {
        eprintf("fs_umount write back dir error\n");
        return -1; 
    }
    for (int i = 0; i < sp->fat_blk_count; ++i)
    {
        if(block_write(1 + i, fat + BLOCK_SIZE * i) < 0)// write back
        {
            eprintf("fs_umount write back dir error\n");
            return -1; 
        }
    }


    if(block_disk_close() < 0) {
        eprintf("fs_umount error\n");
        return -1;
    }

    if(sp) {
        free(sp);
        sp = NULL;
    }
    if(root_dir){
        free(root_dir);
        root_dir = NULL;
    }
    if(fat)
    {
        free(fat);
        fat = NULL;
    }

    // todo: close all the file descriptors

    return 0;
}

/**
 * fs_info - Display information about file system
 *
 * Display some information about the currently mounted file system.
 *
 * Return: -1 if no underlying virtual disk was opened. 0 otherwise.
 */
int fs_info(void)
{
	/* TODO: Phase 1 */
    oprintf("FS Info:\n");
    eprintf("signature=%s\n",sp->signature);
    oprintf("total_blk_count=%d\n",sp->total_blk_count);
    oprintf("fat_blk_count=%d\n",sp->fat_blk_count);
    oprintf("rdir_blk=%d\n",sp->rdir_blk);
    oprintf("data_blk=%d\n",sp->data_blk);
    oprintf("data_blk_count=%d\n",sp->data_blk_count);

    // my info
    eprintf("unused[0]=%d\n", (uint8_t)(sp->unused)[0]); // unused[0]=0

    if(dir_entry->filename == NULL)
        eprintf("root_dir[0].filename == NULL!!!\n"
    else eprintf("root_dir[0].filename != NULL...\n"
    eprintf("root_dir[0].filename=%s\n", dir_entry->filename); // root_dir[0].filename=(null)

    memset(dir_entry, 0, sizeof(dir_entry->filename));
    // for (int i = 0; i < FS_FILENAME_LEN; ++i)
    // {
    //     (dir[0]->filename)[i] = '\0';
    // }
    eprintf("after memeset(0), root_dir[0].filename=%s\n", dir_entry->filename); // root_dir[0].filename=(null)
    // eprintf("root_dir[0].unused=%s\n", dir[0]->unused); 
    eprintf("fat[0]=%d\n", *fat16); // fat[0]=65535
    // eprintf("fat[1]=%d\n", (uint16_t)(fat+2));  
    return 0;
}

/**
 * fs_create - Create a new file
 * @filename: File name
 *
 * Create a new and empty file named @filename in the root directory of the
 * mounted file system. String @filename must be NULL-terminated and its total
 * length cannot exceed %FS_FILENAME_LEN characters (including the NULL
 * character).
 *
 * Return: -1 if @filename is invalid, if a file named @filename already exists,
 * or if string @filename is too long, or if the root directory already contains
 * %FS_FILE_MAX_COUNT files. 0 otherwise.


 This function creates a new file with name in the root directory of your file system. The file is initially empty. The maximum length for a file name is 15 characters. Also, there can be at most 64 files in the directory. Upon successful completion, a value of 0 is returned. fs_create returns -1 on failure. It is a failure when the file with name already exists, when the file name is too long (it exceeds 15 characters), or when there are already 64 files present in the root directory. Note that to access a file that is created, it has to be subsequently opened.

 */
int fs_create(const char *filename)
{
    /* TODO: Phase 2 */
    if(sp == NULL || root_dir == NULL){
        eprintf("fs_create: no vd mounted or root dir read\n");
        return -1;
    }
	/* @filename is invalid; or string @filename is too long*/
    if (filename == NULL || strlen(filename) >= FS_FILENAME_LEN ) // strlen doesn't include NULL char
    {
        eprintf("fs_create: filaname error\n");
        return -1;
    }
    /* a file named @filename already exists; or the root directory already contains
 * %FS_FILE_MAX_COUNT files*/
    // packed to function get_direntry_idx(const char*)
    // int i;
    // for (i = 0; i < FS_FILE_MAX_COUNT; ++i)
    // {
    //     /* code */
    //     if(strcmp(dir[i]->filename, filename) == 0){
    //         eprintf("fs_create: @filename already exists error\n");
    //         return -1;
    //     }
    //     if(dir[i]->filename == NULL)
    //         break;
    // }
    // if(i == FS_FILE_MAX_COUNT){
    //     eprintf("fs_create: root directory full error\n");
    //     return -1;
    // }
    int entry_id = get_freeEntry_idx(filename);
    if(entry_id < 0)
        return -1; // no valid dir entry 
    // the ith entry is available
    // dir_entry = root_dir + entry_id * sizeof(struct RootDirEntry);
    dir_entry = get_dir(entry_id);
    strcpy(dir_entry->filename, filename);
    dir_entry->file_sz = 0;
    dir_entry->first_data_blk = get_freeFat_idx(); // entry
    if(dir_entry->first_data_blk == -1)
        return -1;  // ? need unmounted?

    // fat16 = fat + sizeof(uint16_t) * (dir_entry->first_data_blk);
    fat16 = get_fat(dir_entry->first_data_blk);
    *fat16 = 0xFFFF;

    return 0;
}

/**
 * fs_delete - Delete a file
 * @filename: File name
 *
 * Delete the file named @filename from the root directory of the mounted file
 * system.
 *
 * Return: -1 if @filename is invalid, if there is no file named @filename to
 * delete, or if file @filename is currently open. 0 otherwise.

 This function deletes the file with name from the root directory of your file system and frees all data blocks and meta-information that correspond to that file. The file that is being deleted must not be open. That is, there cannot be any open file descriptor that refers to the file name. When the file is open at the time that fs_delete is called, the call fails and the file is not deleted. Upon successful completion, a value of 0 is returned. fs_delete returns -1 on failure. It is a failure when the file with name does not exist. It is also a failure when the file is currently open (i.e., there exists at least one open file descriptor that is associated with this file).

 */
int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */
    int entry_id = get_dirEntry_idx(filename);
    if(entry_id < 0) return -1; // not found or sp, dir == NULL
    // or if file @filename is currently open. 0 otherwise.

    struct RootDirEntry * cur_entry = root_dir + sizeof(struct RootDirEntry) * entry_id;
    // if(dir[entry_id]->open > 0)
    if(cur_entry->open > 0)
        return -1; // open

    fat16 =  get_fat(cur_entry->first_data_blk);
    erase_fat(fat16);
    memset(cur_entry->filename, 0, FS_FILENAME_LEN); // error: dir[entry_id]->filename = NULL;

    return 0;
}

/**
 * fs_ls - List files on file system
 *
 * List information about the files located in the root directory.
 *
 * Return: -1 if no underlying virtual disk was opened. 0 otherwise.
 */
int fs_ls(void)
{
	/* TODO: Phase 2 */
    return 0;
}

/**
 * fs_open - Open a file
 * @filename: File name
 *
 * Open file named @filename for reading and writing, and return the
 * corresponding file descriptor. The file descriptor is a non-negative integer
 * that is used subsequently to access the contents of the file. The file offset
 * of the file descriptor is set to 0 initially (beginning of the file). If the
 * same file is opened multiple files, fs_open() must return distinct file
 * descriptors. A maximum of %FS_OPEN_MAX_COUNT files can be open
 * simultaneously.
 *
 * Return: -1 if @filename is invalid, there is no file named @filename to open,
 * or if there are already %FS_OPEN_MAX_COUNT files currently open. Otherwise,
 * return the file descriptor.


 The file specified by name is opened for reading and writing, and the file descriptor corresponding to this file is returned to the calling function. If successful, fs_open returns a non-negative integer, which is a file descriptor that can be used to subsequently access this file. Note that the same file (file with the same name) can be opened multiple times. When this happens, your file system is supposed to provide multiple, independent file descriptors. Your library must support a maximum of 32 file descriptors that can be open simultaneously. fs_open returns -1 on failure. It is a failure when the file with name cannot be found (i.e., it has not been created previously or is already deleted). It is also a failure when there are already 32 file descriptors active. When a file is opened, the file offset (seek pointer) is set to 0 (the beginning of the file).


 */
int fs_open(const char *filename)
{
	/* TODO: Phase 3 */
    int entry_id = get_dirEntry_idx(filename);
    if(entry_id < 0) return -1; // not found or sp, dir == NULL
    // or if file @filename is currently open. 0 otherwise.

    dir_entry = get_dir(entry_id);

    if(dir_entry->open >= FS_OPEN_MAX_COUNT)
        return -1; 

    int fd;

    return fd;
}

/**
 * fs_close - Close a file
 * @fd: File descriptor
 *
 * Close file descriptor @fd.
 *
 * Return: -1 if file descriptor @fd is invalid (out of bounds or not currently
 * open). 0 otherwise.


 The file descriptor fildes is closed. A closed file descriptor can no longer be used to access the corresponding file. Upon successful completion, a value of 0 is returned. In case the file descriptor fildes does not exist or is not open, the function returns -1.

 */
int fs_close(int fd)
{
	/* TODO: Phase 3 */
    return 0;
}

/**
 * fs_stat - Get file status
 * @fd: File descriptor
 *
 * Get the current size of the file pointed by file descriptor @fd.
 *
 * Return: -1 if file descriptor @fd is invalid (out of bounds or not currently
 * open). Otherwise return the current size of file.
 */
int fs_stat(int fd)
{
	/* TODO: Phase 3 */
    return 0;
}

/**
 * fs_lseek - Set file offset
 * @fd: File descriptor
 * @offset: File offset
 *
 * Set the file offset (used for read and write operations) associated with file
 * descriptor @fd to the argument @offset. To append to a file, one can call
 * fs_lseek(fd, fs_stat(fd));
 *
 * Return: -1 if file descriptor @fd is invalid (out of bounds or not currently
 * open), or if @offset is out of bounds (beyond the end of the file). 0
 * otherwise.


 This function sets the file pointer (the offset used for read and write operations) associated with the file descriptor fildes to the argument offset. It is an error to set the file pointer beyond the end of the file. To append to a file, one can set the file pointer to the end of a file, for example, by calling fs_lseek(fd, fs_get_filesize(fd));. Upon successful completion, a value of 0 is returned. fs_lseek returns -1 on failure. It is a failure when the file descriptor fildes is invalid, when the requested offset is larger than the file size, or when offset is less than zero.




 */
int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */
    return 0;
}

/**
 * fs_write - Write to a file
 * @fd: File descriptor
 * @buf: Data buffer to write in the file
 * @count: Number of bytes of data to be written
 *
 * Attempt to write @count bytes of data from buffer pointer by @buf into the
 * file referenced by file descriptor @fd. It is assumed that @buf holds at
 * least @count bytes.
 *
 * When the function attempts to write past the end of the file, the file is
 * automatically extended to hold the additional bytes. If the underlying disk
 * runs out of space while performing a write operation, fs_write() should write
 * as many bytes as possible. The number of written bytes can therefore be
 * smaller than @count (it can even be 0 if there is no more space on disk).
 *
 * Return: -1 if file descriptor @fd is invalid (out of bounds or not currently
 * open). Otherwise return the number of bytes actually written.

 This function attempts to write nbyte bytes of data to the file referenced by the descriptor fildes from the buffer pointed to by buf. The function assumes that the buffer buf holds at least nbyte bytes. When the function attempts to write past the end of the file, the file is automatically extended to hold the additional bytes. It is possible that the disk runs out of space while performing a write operation. In this case, the function attempts to write as many bytes as possible (i.e., to fill up the entire space that is left). The maximum file size is 16M (which is, 4,096 blocks, each 4K). Upon successful completion, the number of bytes that were actually written is returned. This number could be smaller than nbyte when the disk runs out of space (when writing to a full disk, the function returns zero). In case of failure, the function returns -1. It is a failure when the file descriptor fildes is not valid. The write function implicitly increments the file pointer by the number of bytes that were actually written.

 */
int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
    return 0;
}

/**
 * fs_read - Read from a file
 * @fd: File descriptor
 * @buf: Data buffer to be filled with data
 * @count: Number of bytes of data to be read
 *
 * Attempt to read @count bytes of data from the file referenced by file
 * descriptor @fd into buffer pointer by @buf. It is assumed that @buf is large
 * enough to hold at least @count bytes.
 *
 * The number of bytes read can be smaller than @count if there are less than
 * @count bytes until the end of the file (it can even be 0 if the file offset
 * is at the end of the file). The file offset of the file descriptor is
 * implicitly incremented by the number of bytes that were actually read.
 *
 * Return: -1 if file descriptor @fd is invalid (out of bounds or not currently
 * open). Otherwise return the number of bytes actually read.


 This function attempts to read nbyte bytes of data from the file referenced by the descriptor fildes into the buffer pointed to by buf. The function assumes that the buffer buf is large enough to hold at least nbyte bytes. When the function attempts to read past the end of the file, it reads all bytes until the end of the file. Upon successful completion, the number of bytes that were actually read is returned. This number could be smaller than nbyte when attempting to read past the end of the file (when trying to read while the file pointer is at the end of the file, the function returns zero). In case of failure, the function returns -1. It is a failure when the file descriptor fildes is not valid. The read function implicitly increments the file pointer by the number of bytes that were actually read.


 */
int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
    return 0;
}

