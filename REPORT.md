ECS 150: Project #4 - File system  
I finally finish it on June 9, 2018

<strong>Question:</strong>   

- [ ] How to read and write efficiently? I update after each block read and write, read and write the first block specially and continue the next blocks if exist.  
Each time I check the remaining part to do this.  

- [X] How to avoid conflict writing to the same file? System has a global fiel table, how could we implement this in user level?  
In this project, we assume it as mono-thread programming and don't need to worried about any multi-thread or race condition. If so, it's a misuse in client level. In multi-thread scenario, we could use semaphore or lock or anything like that.  
- [X] Line 840 about touch it. could i use the unused bits and write back to disk to avoid writing the same file from diff places?  

- [X] How to update offset better? write or read a block and update it immediately or after the whole operation done?  
What I did is ok. 



- [ ] If I shouldn't use the unused bits in file system which has no guarantee that others also write under the same rules.  

- [ ] How to make the read and write efficiency?  

- [ ] (final) file directory vs metadata ? first block not good enough?  


# General Information
This project is to implement the support of a very simple file system based on a FAT and supports up to 128 files in a single root directory.  
The file system is implemented on top of a virtual disk, which is logically split into blocks.  
The first software layer involved in the file system implementation is the block API and is provided, which is used to open or close a virtual disk, and read or write entire blocks from it.
Above the block layer, the FS layer is in charge of the actual file system management. Through the FS layer, we can mount a virtual disk, list the files that are part of the disk, add or delete new files, read from files or write to files, etc.

## Implementation of Read and Write  
I treat the first block to read or write specially, because of the offset needed to take care.  

Take writing as example, I fetch the first written block according to the offset at first and read the whole block to the bounce_buffer; then I write corresponsive length of bits to bounce_buffer starting from the locate calculated from offset and write it back to the block. Then I continue to write the whole block one by one only if the leftover written bits are more than BLOCK_SIZE. When I reach the last block I need to write, I do the similar thing as the first block to write -- read the block to bounce_buffer, write the leftover length of bits to the bounce_buffer starting from position 0 and then write it back to the block to avoid overwriting the remaining part in the block. 
 
At each writing during the whole procedure, I check whether the current block is the last block the current file hold. If so, I search the next avaliable block to expand the file size and continue writing if capable, so the writing operaton will do as much as possbile.  

In a work, I take the first and last written blocks specially.  


## Questions and Answers

1. About `pass by reference` in C, use `void * * ` in function argument  

```c
//inspired by discussion lecture <main>  
int get_valid_directory_entry(const char * filename, void ** entry_ptr)
```


2. Memory allocation  from Bradley  

"If you only need the memory for the duration of a function and know the size beforehand, allocating on the stack may be better because you avoid the overhead of needing to request memory from the system and marking it used."  

It actually usually uses page sharing to get the memory but it still requires marking the memory for use. 
If you need memory you can just declare a char bounce_buffer[BLOCK_SIZE] and the compiler will just use the memory that's already available on the stack and you won't have to call free() later since it's naturally freed when we simply stop caring about that part of the stack.  

Calloc and malloc and other heap memory allocation is more useful when you either don't know how much memory you need until runtime, or you need the memory to be available after you return from the function.   
It's just an efficiency thing. Well the main thing is that the stack is naturally freed when we just move the stack pointer.  
Allocation on the heap requires the extra step to mark the memory as being used and asking the libc library to manage it for you
or if you use other allocation functions like mmap then you need to ask the kernel to mark the memory.  
And frequent allocation and deallocation can lead to fragmentation of heap space.  

One fun thing to try, if you call calloc or malloc then memset 0. You'll find that calloc happens far faster; it has to do with copy on write. 
(you should try researching that on your own if you want the gritty details).  

The *alloc variants are pretty mnemonic - clear-alloc, memory-alloc, re-alloc  
https://stackoverflow.com/questions/1538420/difference-between-malloc-and-calloc  


## Test
add small files within the limitation √  
add a lot small files  √  
add larger files out of the limitation  √
  
write with offset √  

read file √ 
read file with offset  √  
read in many different places, observe the offset  √  

write/read with offset out of the limitation  √ prompt msg  

open one file many times  √

### test_fs_yuan.sh  
Add some corner cases to write into the disk.
```python
run_tests() {
    run_fs_info
    run_fs_info_full

    run_fs_simple_create
    run_fs_xM_create # yuan: add large file
    run_fs_create_multiple # yuan: add two with test_fs.x, ls with fs_ref.x, within boundary
}
```

### test_fs.c
Add `read`, `readm` and `write` to test the cases of RW with offset.  
```c
static struct {
    const char *name;
    void(*func)(void *);
} commands[] = {
    { "info",   thread_fs_info },
    { "ls",     thread_fs_ls },
    { "add",    thread_fs_add },
    { "rm",     thread_fs_rm },
    { "cat",    thread_fs_cat },
    { "stat",   thread_fs_stat },
    { "read",   thread_fs_read }, // exefile read <diskname> <filename> <offset> <opt_count>  
    { "readm",  thread_fs_read_multiple }, // open multiple files and read, exefile read <diskname> <filename> <iterations>
    { "write",  thread_fs_write }, // write <diskname> <filename> <buffer> <offset> <count> <opt_file>
};
```

  
## Source - Piazza
@453   
```c
fs_info() should print the entire output generate when you enter "./fs_ref.x info ...".
The values you need to print are:
total_blk_count = total number of blocks
fat_blk_count = number of FAT blocks
rdir_blk = Index of root directory
data_blk = Index of first data block in virtual disk
data_blk_count = number of data blocks
fat_free_ratio = Number of FAT entries available (free) / number of data blocks
rdir_free_ratio = Number of root directory entries available (free) / size of root directory  
```
@421  
Since the value 0 denotes that a FAT entry is available, the data block #0 can never be allocated to a file. That's why this data block is simply lost.  

@493  char *, null terminator  
By definition a C string is a character array that has a null character at the end to denote the end of the string.
However the memory pointed to by a char* need not be NULL terminated character array. (It's just a pointer to memory, the char* is just to indicate to compiler as to how many bytes it should add/ subtract when doing pointer arithmetic)  
https://stackoverflow.com/questions/47288881/are-c-constant-character-strings-always-null-terminated  
https://softwareengineering.stackexchange.com/questions/344603/are-c-strings-always-null-terminated-or-does-it-depend-on-the-platform  

@520 RW fails doesn't mean disk fails, good points  
block_write failing doesn't always mean disk failure, it could just be the block that failed.

  
## Source
Variadic  Macro  
https://gcc.gnu.org/onlinedocs/cpp/Variadic-Macros.html  
Struct packed  
https://stackoverflow.com/questions/11770451/what-is-the-meaning-of-attribute-packed-aligned4  
https://www.geeksforgeeks.org/structure-member-alignment-padding-and-data-packing/  

Integer  
https://www.gnu.org/software/libc/manual/html_node/Integers.html  

FS  
http://www.cs.cornell.edu/courses/cs4410/2010fa/CS4411/slides/project6/project6.pdf  
https://gitlab.com/nd-cse-30341-fa17/cse-30341-fa17-project06  

FAT / innode  
http://pages.cs.wisc.edu/~remzi/OSTEP/file-implementation.pdf
http://www.tavi.co.uk/phobos/fat.html  
https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system  
https://www.pjrc.com/tech/8051/ide/fat32.html  
http://www.c-jump.com/CIS24/Slides/FAT/lecture.html#F01_0010_overview  
https://www.pjrc.com/tech/8051/ide/fat32.html  

project  
http://www.openvirtualization.org/documentation/fat32_8c_source.html  
http://codeandlife.com/2012/04/07/simple-fat-and-sd-tutorial-part-2/   !!!

links  
https://www.edaboard.com/showthread.php?176568-C-code-for-FAT-file-implementation-using-PIC18F4550
  


## Source II 
Virtual Filesystem (VFS) in Linux  
https://www.tldp.org/LDP/lki/lki-3.html  

FCB  
http://www.cs.odu.edu/~cs471w/spring11/lectures/FileSystemImplementation.htm  
https://cs.nyu.edu/~mwalfish/classes/15sp/labs/lab6.html  

Block and Cluster  
https://unix.stackexchange.com/questions/14409/difference-between-block-size-and-cluster-size  

Mounting Definition (Linux)  
http://www.linfo.org/mounting.html  

memcpy and strcpy  
https://stackoverflow.com/questions/2898364/strcpy-vs-memcpy  

align in C  
https://wr.informatik.uni-hamburg.de/_media/teaching/wintersemester_2013_2014/epc-14-haase-svenhendrik-alignmentinc-paper.pdf  

Cstring in C, null terminator  
https://stackoverflow.com/questions/17522327/is-null-character-included-while-allocating-using-malloc  
Conceptually the null terminator is just a convenient way of marking the end of a string. The C standard library exploits this convention when modelling a string. For example, strlen computes the length of a string by examining the memory from the input location (probably a char*) until it reaches a null terminator; but the null terminator itself is not included in the length. But it's still part of the memory consumed by the string.  

Calloc and malloc  
https://www.tutorialspoint.com/c_standard_library/c_function_calloc.htm  
strcpy, strncpy, memset, memcpy  
https://stackoverflow.com/questions/11830979/c-strcpy-function-copies-null  


struct stat, and fstat  
http://pubs.opengroup.org/onlinepubs/009695399/basedefs/sys/stat.h.html  
http://pubs.opengroup.org/onlinepubs/009695399/functions/fstat.html  

Union  
https://www.geeksforgeeks.org/difference-structure-union-c/  


bash, shell, command  
https://www.gnu.org/software/bash/manual/html_node/The-Set-Builtin.html  
> BASH_SOURCE[0]  

https://stackoverflow.com/questions/35006457/choosing-between-0-and-bash-source  
http://www.tutorialspoint.com/unix/unix-special-variables.htm  
> ||   

make > /dev/null 2>&1 ||  
        die "Compilation failed"  
https://unix.stackexchange.com/questions/325705/why-is-pattern-command-true-useful  
> "$@", "$*", $*  

https://stackoverflow.com/questions/9994295/what-does-mean-in-a-shell-script  
{, , }  
[A..Z]  

> dd  

makefile  
https://www.gnu.org/software/make/manual/html_node/Wildcard-Function.html 