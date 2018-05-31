# Project 4 Report

## Phase 0

```c
struct SuperBlock 
{
    char     signature[8];
    uint16_t total_blk_count;  // Total amount of blocks of virtual disk 
    uint16_t rdir_blk;         // Root directory block index
    uint16_t data_blk;         // Data block start index
    uint16_t data_blk_count;   // Amount of data blocks
    uint8_t  fat_blk_count;    // Number of blocks for FAT

    uint16_t  fat_used;     // myself
    uint16_t  rdir_used;    

    char     unused[4063];     // 4079 Unused/Padding, I use 32bits
}__attribute__((packed));
```

​   For the Superblock, we decided to create a `struct` that contained each of the
members of the   superblock. 

​   Each member type was of the unsigned integer types that are defined in the `stdint.h`
header file.   We determined the number of bits each field was supposed to be according to
the number of bytes that was specified in the project specification. For instance, the total
block count, in the project specification, is 2 bytes long. So, we chose the `unint16_t`  type
for this field.

​   There are fields that we added, which were the `fat_used` and the `rdir_used` fields.
These fields keep track of the number of free data and root directory blocks. This way,
whenever we need to know these numbers, they will already be stored and there will be no
need to perform additional calculations each time `fs_info()` is called.

​   Then, we created a global field to store the block when we read it in as shown below:

```c
struct SuperBlock * sp = NULL;
```

​   Similarly, we make a`struct` data structure to store the Root Directory block, as shown below,
and create a global variable that is of the type `void*` to store it. In this case, the global 
array is a dynamic array that will be read in from the disk.

```c
struct RootDirEntry {                // Inode structure
    char        filename[FS_FILENAME_LEN];         // Whether or not inode is valid
    uint32_t    file_sz;          // Size of file
    uint16_t    first_data_blk; // Direct pointers

    uint16_t    last_data_blk; // Direct pointers
    uint8_t     open;
    char        unused[7];     // one char for indicating writing 'w'
}__attribute__((packed));
```

​   For the FAT block, we just have a global dynamic array of `uint16_t` variables that we
allocate memory to when mounting the disk. This global array is also of type `void *`. The
reason for this is that this gives us the flexibility to store whatever we want inside each of
these arrays. In addition, we can dynamically allocate memory for these arrays. Our
implementation is shown below:

```c
void * fat = NULL;
```

As you can see, this implementation is very basic and simple. 

In addition, we also have other global variables that we made:

```c
char * disk = NULL;
struct RootDirEntry * dir_entry = NULL;
uint16_t * fat16 = NULL;  
```

The `disk` string is the disk name of the current disk that is open. `dir_entry` is a pointer
that is used to point to various elements of the Root Directory array `sp`. Finally, `fat16` is
a pointer that is used to point to various elements of the FAT array `fat`. These globals are
used later, in phase 2 - 4.

## Phase 1

​   Using the data structures that we created in phase 0, we read in each of the blocks in
from the disk.

### fs_mount()

​   For this function, we first open the disk using the the `block_disk_open(diskname)`
function provided to us in the `disk`API. Then, we allocate space to each of our global
variables that we made in phase 0 and initialize them. Then, we read in each of the blocks. The
way we find the index at which to read in each of the blocks from is by using the fields of the
Super Block and the function `block_disk_read()`. 

​   When we read in the Super Block, we do a couple of checks to make sure that each of the
fields are correct. First, we check to make sure the signature is equivalent to `"ECS150FS"` .
Then, we check to make sure that the total number of blocks is equal to the number that is
returned by `block_disk_count()` . Then, we assume that the total number of fat blocks
given to us by the Superblock is correct. Provided these numbers are correct, we then
calculate the rest of the fields, such as the Data Block and Root Directory Indices. Then, we
make sure that the fields in the Superblock are equal to these numbers. 

### fs_umount()

​   For this function, we simply deallocate memory and un-initialize all the global variables
that we used. The reason we used variables of type `void*` becomes clear here: since they
are of this type, we can easily un-initialize them by simply freeing them. If we had used
statically allocated arrays, then we would have to go through each element of the array and
set them to default values. Before any of this happens though, we make sure to write back all
the metadata back to the disk. Then, we go ahead and deallocate memory for each of the
global variables that we used and set them equal to `NULL`.

### fs_info()

​   This function was fairly simple. All we did was print the various metadata of the disk.
Since we already have the number of free data blocks and root directory entries, all we do is
output these fields.

## Phase 2

### fs_create()

​   In this function, we make sure to first make the various edge case testing, such as making
sure that this function is not called when there is no disk mounted, when the filename is not
valid, when a file with that name already exists, or when there is no space for a new file, i.e.
the number of files on the disk has reached the limit, *128* *files*. If there is space for a new
file, then we manually set each of the fields of the new file in the Root Directory, as shown
below:

```c
dir_entry = get_dir(entry_id); 
    strcpy(dir_entry->filename, filename);
    dir_entry->file_sz = 0;
    dir_entry->open = 0;
    dir_entry->first_data_blk = FAT_EOC;//entry, should assign block here, in case the file size is 0;
    dir_entry->last_data_blk = FAT_EOC;
    memset(dir_entry->unused, 0, 7);
```

### fs_delete()

​   In this function, we perform a couple checks before we delete the file: we make sure the
filename exists and we make sure that the file is already open. Then, if the file exists, we go
ahead through the fat and delete all the file blocks from the disk and from the FAT. Then we
set the corresponding file entry on the disk to default values. Then, we write everything to the
disk.

### fs_ls()

​   This function was very straightforward to implement. All we did was go through our Root
Directory array and print out each file and the desired description of the each file.

## Phase 3

​   For this phase, we ended up making new global variables in order to keep track of our file
descriptors. The following code snippet shows the new data structures we made:

```c
struct FileDescriptor
{
    void * file_entry;
    size_t offset;
};
int fd_cnt = 0;     // fd used number
struct FileDescriptor* filedes[FS_OPEN_MAX_COUNT];
```

​   The `struct` stores each file descriptor as an entry of the `filedes` array, which
contains all the file descriptors. This array contains `FileDescriptor` pointers because
then we can dynamically allocate memory as needed whenever a new file descriptor is made.
Finally, the `fd_cnt` global field is used to keep track of the total number of file descriptors.
This is used to keep track of the total number of file descriptors that exist. This fields exists to
avoid having to look through the array.

### fs_open()

​   In this function, we first make a couple checks. First, we check whether or not the
filename itself is valid. Then we check if a file with that filename exists: if it does exist, and if
there number of file descriptors do not surpass the macro `FS_OPEN_MAX_COUNT` , then we
make a new element by allocating memory in our `filedes` array for a new element and
setting the fields of that element to default values. Then, we make sure to increment the
`fd_count` global. Because this file is empty, we make sure to not allocate space in our FAT
for the file.

### fs_close()

​   In this function, we first make sure that the file descriptor is valid, i.e. it exists. Then, we
make sure to decrement the `fd_count` global. Finally, we free the element of the array.

### fs_stat()

​   This function is very straightforward. After making sure the file descriptor passed is valid,
we simply return the size of the file, which is accessible via the `file_entry` field of the
`struct`.

### fs_lseek()

​   In this function, after checking that the file descriptor passed in is valid (exists), then
simply set the offset of the file descriptor equal to the offset passed in. We also make sure to
check that this offset is not bigger than the file size of the file that the file descriptor points to. 

## Phase 4

This phase was by far the most difficult phases for us to implement. We had to rewrite this
phase due to the fact that we did not realize that the offset might force to read and write from
the middle of a block. There are a couple of important cases that apply to both `fs_read()`
and `fs_write()`. 

​   The first case is that the offset may force us to read from the middle of a block. The way
we dealt with this is that we made two calculations: one calculation to calculate the number of
blocks to cycle through until we reach the correct block to start reading/writing from and
another calculation to find where exactly in the block to start writing from. The first calculation
was made by dividing the offset by the block size, **4096**. The second was made by
modding the offset by the block size. This gave us the exact position from within the file to
start reading/writing from.

​   The second case is that when we finish reading/writing, we can potentially end up in the
middle of a block. Therefore, we cannot simply ready the entirety of the last block that we end
at. Therefore, we have a variable called `real_count` that keeps track of the amount of
bytes that we have to read/write. Then, we use a temporary variable that we initialize to
`real_count` and then decrement as we read and write data until the temporary variable
becomes 0. 

### fs_read()

​   This function was a lot more straightforward for us to implement simply due to the fact
that there less edge cases to deal with. First, we checked the file descriptor was valid. Then,
after performing the calculations that were previously, we obtain the place to start reading
from. Then, we used an intermediary variable called `bounce_buffer` to read to. We
essentially used a while to read to this variable until we reach the last block of data. Then,
once we reach the last block of data, we read the remainder of the data to `bounce_buffer`
and then copy the data from this variable to the buffer argument that was passed in. This 
while loop can be viewed below:

```c
while(real_count_temp > 0 && read_blk != FAT_EOC){
        if(real_count_temp >= BLOCK_SIZE){
            if(block_read(read_blk + sp->data_blk, buf + buf_idx) < 0){
                free(bounce_buffer);
                return -1;
            }
            buf_idx += BLOCK_SIZE;
        }
        else{
            if(block_read(read_blk + sp->data_blk, bounce_buffer) < 0){
                free(bounce_buffer);
                return -1;
            }
            memcpy(buf + buf_idx, bounce_buffer, real_count_temp);
            buf_idx += real_count_temp; // unnecessary acutally
        }
        real_count_temp -= BLOCK_SIZE;
        read_blk = *(get_fat(read_blk));
    }
```

Note that the way we knew we were on the last block was by the temporary variable that was 
mentioned previously. If this variable was below the block size **4096**, then we were 
definitely on the last block of data

#### fs_write()

​   This function, by far, was the least straightforward for us to implement due to the type of 
edge cases that we had to deal with. We built off of our implementation for `fs_read()`. We 
had to consider some more edge cases here.

​   The first case we had to consider was this: what if the amount of data we wanted to write 
would be cause us to go beyond the number of files allocated for the file?

​   The second case was: what if the amount of data we had to write was greater than the 
amount of free space on the disk?

​   Now let us go into our implementation. First, we make sure that the file descriptor is valid 
and that other file descriptors are not writing to the file. We do this using the first index of the 
padding in the root directory entry. If it is equal to `'w'`, then it is currently being written to. 
Then, after we just write to the file. We start writing in the same way we start reading. Then, if 
we run into edge case 1, we essentially expand the file to accommodate the extra space. If we 
run into edge case 2, then we just write what we can to the disk.
