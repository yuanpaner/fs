#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // strcmp, strlen, strcpy

#include <stdbool.h>
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
#define clamp(x, y) (((x) <= (y)) ? (x) : (y))
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

    uint16_t  fat_used;    
    uint16_t  rdir_used;    

    char     unused[4063];     // 4079 Unused/Padding, I use 32bits
}__attribute__((packed));


struct RootDirEntry {                // Inode structure
    char        filename[FS_FILENAME_LEN];         // Whether or not inode is valid
    uint32_t    file_sz;          // Size of file
    uint16_t    first_data_blk; // Direct pointers
    uint16_t    last_data_blk; // Direct pointers
    uint8_t     open;
    char        unused[7];     
}__attribute__((packed));

struct FileDescriptor
{
    void * file_entry;
    size_t offset;
};

/*
union Block {
    struct SuperBlock   super;              // Superblock
    uint16_t            fat[BLOCK_SIZE/2];        
    struct RootDirEntry rootdir[FS_FILE_MAX_COUNT];   // Pointer block
    char                block[BLOCK_SIZE];     // Data block
};
*/


char * disk = NULL; //virtual disk name pointer
struct SuperBlock * sp = NULL;  // superblock pointer
void * root_dir = NULL;         // root directory pointer
struct RootDirEntry * dir_entry = NULL; // 32B * 128 entry, file entry pointer
void * fat = NULL;              //FAT block pointer
uint16_t * fat16 = NULL;        //fat array entry pointer

int fd_cnt = 0;     // fd used number
struct FileDescriptor* filedes[FS_OPEN_MAX_COUNT];

/* used to check the validation of the input file descirptor number */
bool is_valid_fd(int fd){
    if(fd < 0 || fd >= FS_OPEN_MAX_COUNT || filedes[fd] == NULL) 
        return false;
    else return true;
}

/* get valid file descirptor number */
int get_valid_fd(){
    if(filedes == NULL || fd_cnt >= FS_OPEN_MAX_COUNT) return -1;
    for (int i = 0; i < FS_OPEN_MAX_COUNT; ++i)
        if(filedes[i] == NULL)
            return i;

    return -1;
}

/* get file directory entry pointer according to id */
struct RootDirEntry * get_dir(int id){
    if(root_dir == NULL) return NULL;
    return (struct RootDirEntry *)(root_dir + id * sizeof(struct RootDirEntry));
}

/* get file directory entry pointer according to block id */
uint16_t * get_fat(int id){
    if(fat == NULL || id == 0xFFFF) return NULL;
    if(id < 0 || id >= sp->data_blk_count) return NULL; // out of boundary

    return (uint16_t *)(fat + 2 * id);
    // return (uint16_t *)(fat + sizeof(uint16_t) * id);
}


/* get next free file directory entry id;
 * check the duplicated existed filename
 */
int get_freeEntry_idx(const char * filename){
    if(root_dir == NULL || sp == NULL)
        return -1;
    int i;
    for (i = 0; i < FS_FILE_MAX_COUNT; ++i)
    {
        struct RootDirEntry * tmp = root_dir + i * sizeof(struct RootDirEntry);
        if(strcmp(tmp->filename, filename) == 0){
            // eprintf("fs_create: @filename already exists error\n");
            return -1;
        }
        if(tmp->filename[0] == 0)
            break;
    }
    if(i == FS_FILE_MAX_COUNT){
        // eprintf("fs_create: root directory full error\n");
        return -1;
    }
    return i;
}

/* get next free fat entry id */
uint16_t get_freeFat_idx(){
    if(fat == NULL || sp == NULL)
        return -1;
    if(sp->data_blk_count - sp->fat_used == 0) {
        eprintf("data blk exhausted\n");
        return -1;
    }       

    uint16_t i = 1;
    uint16_t * tmp = fat;
    tmp++; // skip #0 fat
    // for (tmp = fat; i < sp->fat_blk_count * BLOCK_SIZE / 2 ; ++i, tmp += sizeof( uint16_t ))
    for (; i < sp->fat_blk_count * BLOCK_SIZE / 2 ; ++i, tmp++)
        if (*tmp == 0 )
            return i;
    if( i == sp->fat_blk_count * BLOCK_SIZE / 2)
        eprintf("fat exhausted\n");

    return -1;
}

/* get the dir entry id by filename*/
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

/* set the fat as zero ( free ) again
 * update the sp->fat_used
*/
int erase_fat(uint16_t * id){ // recursion to erase
    if(sp == NULL || root_dir == NULL || id == NULL)
        return -1;

    if(*id == 0xFFFF){
        *id = 0;
        // sp->fat_used -= 1;
        return 0;
    }

    uint16_t * next = fat + sizeof(uint16_t) * (*id);
    erase_fat(next);
    // sp->fat_used -= 1;
    *id = 0;

    return 0;
}

/* calculate how many blocks needed for a file of size @sz */
int file_blk_count(uint32_t sz){
    if(sz == 0) return 1;
    
    int k = sz / BLOCK_SIZE;
    if( k * BLOCK_SIZE < sz)
        return k + 1;
    else return k;
}

/* not used
uint16_t id_to_real_blk(int i){
    // if sp == NULL || root_dir == NULL;
    return i + sp->data_blk;
} 
*/


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
 open the disk and then load the meta-information that is necessary to handle the file system operations
 */

void sp_setup(){
    if(sp == NULL || root_dir == NULL)
        return ;
    if(sp->fat_used > 1 && sp->rdir_used > 0)
        return ; // no need to set, has already been written

    dir_entry = root_dir;
    sp->fat_used = 1;
    sp->rdir_used = 0;
    for (int i = 0; i < FS_FILE_MAX_COUNT; ++i, dir_entry++)
    {
        if(dir_entry->filename[0] != 0){
            sp->rdir_used += 1;
            int tmp = dir_entry->file_sz / BLOCK_SIZE;
            if(tmp * BLOCK_SIZE < dir_entry->file_sz)
                tmp += 1;
            sp->fat_used += tmp;
        }
    }
    // if(sp->fat_used == 0)
    //     sp->fat_used = 1;
}

void clear(){
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

    // char * disk = NULL; //virtual disk name pointer
    // struct SuperBlock * sp = NULL;  // superblock pointer
    // void * root_dir = NULL;         // root directory pointer
    // struct RootDirEntry * dir_entry = NULL; // 32B * 128 entry, file entry pointer
    // void * fat = NULL;              //FAT block pointer
    // uint16_t * fat16 = NULL;        //fat array entry pointer

    // int fd_cnt = 0;     // fd used number
    // struct FileDescriptor* filedes[FS_OPEN_MAX_COUNT];
}

/*
 * alloc space to sp, root_dir, and fat; set to zero for all of them
 * initialize filedes, fd_cnt
 * initialize sp_setup()
 * fail return -1; succeed return 0;
*/
int init_alloc(){


    sp = malloc(BLOCK_SIZE); 
    // sp = calloc(BLOCK_SIZE,1); 
    if(sp == NULL) {
        clear();
        return -1;
    }
    root_dir = malloc(BLOCK_SIZE);
    // root_dir = calloc(BLOCK_SIZE,1);
    if(root_dir == NULL) {
        clear();
        return -1;
    }
    fat = malloc(BLOCK_SIZE * sp->fat_blk_count);
    // fat = calloc(BLOCK_SIZE * sp->fat_blk_count, 1);
    if(fat == NULL) {
        clear();
        return -1;
    }


    for (int i = 0; i < FS_OPEN_MAX_COUNT; ++i)
        filedes[i] = NULL;
    fd_cnt = 0;

    memset(sp, 0, BLOCK_SIZE);
    memset(root_dir, 0, BLOCK_SIZE);
    memset(fat, 0, BLOCK_SIZE * sp->fat_blk_count);    
    
    // dir_entry = get_dir(0);  

    return 0;   
}
/*
 * free space to sp, root_dir, and fat; set to zero for all of them
 * fail return -1; succeed return 0;
*/


/* version 1. 0   */
int fs_mount(const char *diskname)
{
    if (block_disk_open(diskname) != 0) return -1;
    
    // if(init_alloc() < 0) return -1; // allocate error

    sp = malloc(BLOCK_SIZE); // struct SuperBlock * super = malloc(BLOCK_SIZE); malloc(sizeof(struct SuperBlock))
    if(sp == NULL) { clear(); return -1; }

    memset(sp, 0, BLOCK_SIZE);
    if(block_read(0, (void *)sp) < 0) { clear(); return -1; }
    // if(sp->fat_used == 0)
    //     sp->fat_used = 1;
    // sp_setup();

    root_dir = malloc(BLOCK_SIZE);
    memset(root_dir, 0, BLOCK_SIZE);
    if(block_read(sp->rdir_blk, root_dir) < 0){
        eprintf("fs_mount read root dir error\n");
        clear(); 
        return -1; 
    }
    // dir_entry = (struct RootDirEntry *)root_dir;
    dir_entry = get_dir(0);
    sp_setup();

    fat = malloc(BLOCK_SIZE * sp->fat_blk_count);
    memset(fat, 0, BLOCK_SIZE * sp->fat_blk_count);
    for (int i = 0; i < sp->fat_blk_count; ++i)
    {
        if(block_read(i+1, fat + BLOCK_SIZE * i) < 0){
            eprintf("fs_mount read %d th(from 1) fat block error\n", i);
            clear();
            return -1;
        }
    }
    fat16 = get_fat(0);

    for (int i = 0; i < FS_OPEN_MAX_COUNT; ++i)
        filedes[i] = NULL;
    
    fd_cnt = 0;
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
/**/

/* version 2.0 fail , it fails at info 
int fs_mount(const char *diskname)
{
    if (block_disk_open(diskname) != 0) return -1;
    
    // if(init_alloc() < 0) return -1; // allocate error
    if(init_alloc() < 0) {
        eprintf("alloc error\n");
        return -1;
    }
    if(block_read(0, (void *)sp) < 0) {
        clear();
        eprintf("fs_mount read superblock error\n");
        return -1;
    }
    if(strncmp(sp->signature, FS_NAME, 8) != 0){
        clear();
        eprintf("fs_mount non ECS150FS file system\n");
        return -1;
    }
    sp_setup();
    if(block_read(sp->rdir_blk, root_dir) < 0){
        eprintf("fs_mount read root dir error\n");
        clear();
        return -1;
    }
    // dir_entry = (struct RootDirEntry *)root_dir;
    dir_entry = get_dir(0); // not necessary
    for (int i = 0; i < sp->fat_blk_count; ++i)
    {
        if(block_read(i+1, fat + BLOCK_SIZE * i) < 0){
            eprintf("fs_mount read %d th(from 1) fat block error\n", i);
            clear();
            return -1;
        }
    }
    fat16 = get_fat(0);
    return 0;
}
*/



/**
 * fs_umount - Unmount file system
 *
 * Unmount the currently mounted file system and close the underlying virtual
 * disk file.
 *
 * Return: -1 if no underlying virtual disk was opened, or if the virtual disk
 * cannot be closed, or if there are still open file descriptors. 0 otherwise.
This function unmounts your file system from a virtual disk with name disk_name.  
need to write back all meta-information so that the disk persistently reflects all changes that were made to the file system (such as new files that are created, data that is written, ...). 
You should also close the disk. The function returns 0 on success, and -1 when the disk disk_name could not be closed or when data could not be written to the disk (this should not happen).
It is important to observe that your file system must provide persistent storage. 
This means that whenever umount_fs is called, all meta-information and file data (that you could temporarily have only in memory; depending on your implementation) must be written out to disk.
*/ 
int write_meta(){
    if(sp == NULL || root_dir == NULL || fat == NULL){
        eprintf("no virtual disk mounted to write_meta");
        return -1;
    }
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
            eprintf("fs_umount write back fat blk %d error\n", i);
            return -1; 
        }
    }
    return 0;
}
int fs_umount(void)
{
    /* TODO: Phase 1 */
    /* write back: super block, fat, dir*/
    if(sp == NULL)
        return -1;  // no underlying virtual disk was opened

    if(write_meta() < 0 ) return -1;
    // if(block_write(0, (void *)sp) < 0)
    // {
    //     eprintf("fs_umount write back sp error\n");
    //     return -1; 
    // }
    // if(block_write(sp->rdir_blk, root_dir) < 0)// write back
    // {
    //     eprintf("fs_umount write back dir error\n");
    //     return -1; 
    // }
    // for (int i = 0; i < sp->fat_blk_count; ++i)
    // {
    //     if(block_write(1 + i, fat + BLOCK_SIZE * i) < 0)// write back
    //     {
    //         eprintf("fs_umount write back dir error\n");
    //         return -1; 
    //     }
    // }

    if(fd_cnt > 0){
        eprintf("there are files open, unable to umount\n");
        return -1;
    }
    // for (int i = 0; i < FS_OPEN_MAX_COUNT; ++i)
    // {
    //     if(filedes[i] != NULL){
    //         free(filedes[i]);
    //         filedes[i] = NULL; 
    //     }
            
    // }


    if(block_disk_close() < 0) { //cannot be closed
        eprintf("fs_umount error\n");
        return -1; 
    }

    clear();

    // for (int i = 0; i < FS_OPEN_MAX_COUNT; ++i)
    // {
    //     if(filedes[i] != NULL){
    //         free(filedes[i]);
    //         filedes[i] = NULL; 
    //     }
            
    // }
    // fd_cnt = 0;


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
    if(sp == NULL ) {
        oprintf("no underlying virtual disk was mounted sucessfully\n");
        return -1;
    }
    oprintf("FS Info:\n");
    // eprintf("signature=%s\n",sp->signature); // non-terminator
    // eprintf("%.*s\n", 8, sp->signature); // works

    oprintf("total_blk_count=%d\n",sp->total_blk_count);
    oprintf("fat_blk_count=%d\n",sp->fat_blk_count);
    oprintf("rdir_blk=%d\n",sp->rdir_blk);
    oprintf("data_blk=%d\n",sp->data_blk);
    oprintf("data_blk_count=%d\n",sp->data_blk_count);

    oprintf("fat_free_ratio=%d/%d\n", (sp->data_blk_count - sp->fat_used), sp->data_blk_count);
    oprintf("rdir_free_ratio=%d/%d\n", (FS_FILE_MAX_COUNT - sp->rdir_used),FS_FILE_MAX_COUNT);


    /* my info
    eprintf("unused[0]=%d\n", (uint8_t)(sp->unused)[0]); // unused[0]=0
    if(dir_entry->filename == NULL)
        eprintf("root_dir[0].filename == NULL!!!\n");
    else eprintf("root_dir[0].filename != NULL...\n");
    eprintf("root_dir[0].filename[0]=%d\n", (int)(dir_entry->filename[0])); // root_dir[0].filename=(null)
    memset(dir_entry, 0, sizeof(dir_entry->filename));
    eprintf("after memeset(0), root_dir[0].filename=%s\n", dir_entry->filename); // root_dir[0].filename=(null)
    eprintf("fat[0]=%d\n", *fat16); // fat[0]=65535
    */
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
    if (filename == NULL || filename[0] == 0 || strlen(filename) >= FS_FILENAME_LEN ) // strlen doesn't include NULL char
    {
        eprintf("fs_create: filaname is invalid\n");
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
    dir_entry->open = 0;
    dir_entry->first_data_blk = 0xFFFF; // entry, should assign block here, in case the file size is 0;
    dir_entry->last_data_blk = 0xFFFF;
    memset(dir_entry->unused, 0, 7);
    // dir_entry->first_data_blk = get_freeFat_idx(); // entry, should assign block here, in case the file size is 0;
    // if(dir_entry->first_data_blk == -1){ // not valid fat
    //     memset(dir_entry->filename, 0, sizeof(dir_entry->filename)); // make it free again.
    //     return -1; // unmount?
    // }
    // dir_entry->last_data_blk = dir_entry->first_data_blk;
    // fat16 = get_fat(dir_entry->first_data_blk);
    // *fat16 = 0xFFFF;

    // sp->fat_used += 1;
    sp->rdir_used += 1; // how to deal with @setup_sp

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
    erase_fat(fat16); // how about return -1?
    memset(cur_entry, 0, sizeof(struct RootDirEntry)); // error: dir[entry_id]->filename = NULL;
    // memset(cur_entry->filename, 0, FS_FILENAME_LEN); // error: dir[entry_id]->filename = NULL;

    sp->rdir_used -= 1;
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
    /* TODO: Phase 2 
    FS Ls:
    file: tmp_argv.c, size: 2038, data_blk: 1
    */
    oprintf("FS Ls:\n");
    if(root_dir != NULL && sp != NULL){
        dir_entry = get_dir(0);
        for (int i = 0; i < FS_FILE_MAX_COUNT; ++i, dir_entry++)
        {
            // dir_entry = get_dir(i);
            if((dir_entry->filename)[0] != 0){
                oprintf("file: %s, size: %d, data_blk: %d\n", dir_entry->filename, dir_entry->file_sz, dir_entry->first_data_blk);
            }
        }
    }

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
    if(fd_cnt >= FS_OPEN_MAX_COUNT)
        return -1;

    int entry_id = get_dirEntry_idx(filename);
    if(entry_id < 0) return -1; // not found or sp, dir == NULL
    // or if file @filename is currently open. 0 otherwise.

    dir_entry = get_dir(entry_id);
    ++(dir_entry->open);

    int fd = get_valid_fd();
    filedes[fd] = malloc(sizeof(struct FileDescriptor));
    filedes[fd]->file_entry = dir_entry;
    filedes[fd]->offset = 0;
    if(fs_lseek(fd, fs_stat(fd)) < 0)
        return -1; 

    fd_cnt++;

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
    // if(fd < 0 || fd >= FS_OPEN_MAX_COUNT || filedes[fd] == NULL)  return -1;
    if(!is_valid_fd(fd)) return -1;

    dir_entry = filedes[fd]->file_entry;
    dir_entry->open -= 1;

    free(filedes[fd]);
    filedes[fd] = NULL;

    fd_cnt--;

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
    // if(fd < 0 || fd >= FS_OPEN_MAX_COUNT || filedes[fd] == NULL) 
    if(!is_valid_fd(fd)) 
        return -1;
    //Size of file 'tmp_argv.c' is 2038 bytes
    dir_entry = filedes[fd]->file_entry;
    return dir_entry->file_sz;
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
 */
int fs_lseek(int fd, size_t offset)
{
    /* TODO: Phase 3 */
    if(!is_valid_fd(fd)) return -1;
    dir_entry = (struct RootDirEntry *)(filedes[fd]->file_entry);
    if(offset > dir_entry->file_sz) return -1;

    filedes[fd]->offset = offset;

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
 √ calculate the real count written
 √ set up the FAT index (should after written success )
 √ write the content
 √ update file entry(should after written success)
 */
int fs_write(int fd, void *buf, size_t count)
{
    /* TODO: Phase 4 */
    if(!is_valid_fd(fd)) return -1;
    if(sp->data_blk_count == sp->fat_used ) return 0; // not error -1; return "written" count 0;

    struct RootDirEntry * w_dir_entry = (struct RootDirEntry *)(filedes[fd]->file_entry);

    if(w_dir_entry->unused[0] == 'w') return -1; // others are writing this file

    int real_count = count;

    if(real_count == 0){ // write 0 B
        w_dir_entry->unused[0] = 'n'; 
        return real_count;
    }

    if( w_dir_entry->first_data_blk == 0xFFFF){ // no blk assign
        w_dir_entry->first_data_blk = get_freeFat_idx();
        if(w_dir_entry->first_data_blk == -1){ // not valid fat
            // memset(w_dir_entry->filename, 0, sizeof(w_dir_entry->filename)); // make it free again.
            w_dir_entry->first_data_blk = 0xFFFF; // according to fs_ref.x result, occupy the rdir, no dota block
            return -1; // unmount?
        }

        w_dir_entry->last_data_blk = w_dir_entry->first_data_blk;
        fat16 = get_fat(w_dir_entry->first_data_blk);
        *fat16 = 0xFFFF;
        sp->fat_used += 1; // !!!!
    }
    // w_dir_entry->first_data_blk = get_freeFat_idx(); // entry, should assign block here, in case the file size is 0;
    // if(w_dir_entry->first_data_blk == -1){ // not valid fat
    //     memset(w_dir_entry->filename, 0, sizeof(w_dir_entry->filename)); // make it free again.
    //     return -1; // unmount?
    // }
    // w_dir_entry->last_data_blk = w_dir_entry->first_data_blk;
    // fat16 = get_fat(w_dir_entry->first_data_blk);
    // *fat16 = 0xFFFF;

    int file_sz_old = w_dir_entry->file_sz;
    int file_sz_new = file_sz_old + count;
    int blk_old = file_blk_count(file_sz_old); // block count needed
    // blk_old = (blk_old == 0 ? 1 : blk_old);
    int blk_new = file_blk_count(file_sz_new);

    uint16_t old_last = w_dir_entry->last_data_blk;

    int blk_more = blk_new - blk_old;
    if(blk_more > sp->data_blk_count - sp->fat_used){
        blk_more = sp->data_blk_count - sp->fat_used;
        real_count = (blk_more + blk_old) * BLOCK_SIZE - w_dir_entry->file_sz;
    }
        

    if(blk_old != blk_new){
        //set up the blk;        
        while(blk_more > 0){
            int next = get_freeFat_idx();
            // if(next == -1) break; // no valid fat; actually impossible
            uint16_t * fat_entry = get_fat(w_dir_entry->last_data_blk);
            *fat_entry = next;
            fat_entry = get_fat(next);
            *fat_entry = 0xFFFF; // if not, the "next" block still available
            // *(get_fat(w_dir_entry->last_data_blk)) = next ; // update the fat chain
            w_dir_entry->last_data_blk = next;
            blk_more -= 1;
            sp->fat_used += 1;
        }
        // if(blk_more != 0) // succeed to write all; error, fat entry migth be bigger than block entry
        //     real_count = (blk_new - blk_more) * BLOCK_SIZE - file_sz_old; // actually impossible

        *(get_fat(w_dir_entry->last_data_blk)) = 0xFFFF; 
    }

    //start to write 
    //1. get remaining content in the old last blk
    //2. cat the old and new real_count
    //3. write the whole block

    //write the old last blk specially
    //write the other blocks as a whole

    // int blk_more_real = blk_new - blk_old - blk_more;
    // ? fat index, and actual index, id should think about
    // uint16_t truncate = w_dir_entry->file_sz % BLOCK_SIZE;
    // if(truncate == 0){

    // }
    // else{
    //     void * buf_temp = malloc(BLOCK_SIZE);
    //     if(block_read(0, buf_temp) < 0) 
    //     memcpy(buf_temp, )
    // }
    //     truncate += real_count;

    // int block_write(size_t block, const void *buf), block start from 0

    void * bounce_buffer = malloc(BLOCK_SIZE);
    memset(bounce_buffer, 0, BLOCK_SIZE);

    if(block_read(sp->data_blk + old_last, bounce_buffer) < 0) return -1; // should free bounce_buffer before return -1
    // size_t truncate = strlen(bounce_buffer); // or should I use truncate = w_dir_entry->file_sz % BLOCK_SIZE // error
    if(fs_lseek(fd, fs_stat(fd)) < 0)
        return -1; 
    size_t truncate = (filedes[fd]->offset) % BLOCK_SIZE;
    size_t buf_idx = clamp(BLOCK_SIZE - truncate, real_count);
    memcpy(bounce_buffer + truncate, buf, buf_idx);
    if(block_write(sp->data_blk + old_last, bounce_buffer) < 0 ) return -1; 

    int temp_count = real_count;
    temp_count -= buf_idx;
    int temp_blk_id = *(get_fat(old_last));     
    while(temp_count > 0){ // into the loop, temp_blk_id != 0xFFFF       
        // truncate = clamp(temp_count, BLOCK_SIZE);//reuse the var truncate 
        if(temp_count > BLOCK_SIZE)
            block_write(sp->data_blk + temp_blk_id, buf + buf_idx); // < 0 should return -1;
        else {
            memset(bounce_buffer, 0, BLOCK_SIZE);
            memcpy(bounce_buffer, buf + buf_idx, temp_count);
            block_write(sp->data_blk + temp_blk_id, bounce_buffer);
        }

        buf_idx += BLOCK_SIZE;
        temp_count -= BLOCK_SIZE;
        temp_blk_id = *(get_fat(temp_blk_id));
    }

    free(bounce_buffer);

    w_dir_entry->unused[0] = 'n';

    w_dir_entry->file_sz += real_count;

    return real_count;
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
 int block_read(size_t block, void *buf);
 */
int fs_read(int fd, void *buf, size_t count)
{
    /* TODO: Phase 4 */
    if(!is_valid_fd(fd)) return -1;
    dir_entry = filedes[fd]->file_entry;

    size_t real_count = clamp(dir_entry->file_sz, count);
    void *bounce_buffer = malloc(BLOCK_SIZE);
    int i = 0;
    uint16_t temp_blk_id = dir_entry->first_data_blk; 
    while(count > 0){
        if(count >= BLOCK_SIZE){
            if(block_read(temp_blk_id + sp->data_blk, buf + i) < 0 ) return -1;
            i += BLOCK_SIZE;
        }
        else{
            memset(bounce_buffer, 0, BLOCK_SIZE);
            if(block_read(temp_blk_id + sp->data_blk, bounce_buffer) < 0 ) return -1;
            memcpy(buf+i, bounce_buffer, count);
        }

        temp_blk_id = *(get_fat(temp_blk_id));
        count -= BLOCK_SIZE;
    }

    free(bounce_buffer);

    if(fs_lseek(fd, real_count) < 0) return -1;
 
    return real_count;
}