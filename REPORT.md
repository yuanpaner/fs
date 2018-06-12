ECS 150: Project #4 - File system  
I finally finish it on June 9, 2018

question:   
How to read and write efficiently? I update after each block read and write, read and write the first block specially and continue the next blocks if exist.  
Each time I check the remaining part to do this.  

How to avoid conflict writing to the same file? System has a global fiel table, how could we implement this in user level?  
(final) file directory vs metadata ? first block not good enough?

How to update offset better? write or read a block and update it immediately or after the whole operation done?

Line 840 about touch it. could i use the unused bits and write back to disk to avoid writing the same file from diff places?

If I shouldn't use the unused bits in file system which has no guarantee that others also write under the same rules.  

How to make the read and write efficiency?  

# General Information
This project is to implement the support of a very simple file system based on a FAT and supports up to 128 files in a single root directory.  

The file system is implemented on top of a virtual disk, which is logically split into blocks.  

The first software layer involved in the file system implementation is the block API and is provided, which is used to open or close a virtual disk, and read or write entire blocks from it.

Above the block layer, the FS layer is in charge of the actual file system management. Through the FS layer, we can mount a virtual disk, list the files that are part of the disk, add or delete new files, read from files or write to files, etc.

## Implementation of Read and Write  
I treat the first block to read or write specially, because of the offset needed to take care.  
Take writing as example, I fetch the first written block according to the offset at first and read the whole block to the bounce_buffer; then I write corresponsive length of bits to bounce_buffer starting from the locate calculated from offset and write it back to the block. Then I continue to write the whole block one by one only if the leftover written bits are more than BLOCK_SIZE. When I reach the last block I need to write, I do the similar thing as the first block to write -- read the block to bounce_buffer, write the leftover length of bits to the bounce_buffer starting from position 0 and then write it back to the block to avoid overwriting the remaining part in the block.  
In a work, I take the first and last written blocks specially.  
At each writing during the whole procedure, I check whether the current block is the last block the current file hold. If so, I search the next avaliable block to expand the file size and continue writing if capable, so the writing operaton will do as much as possbile.  

## Questions and Answers
1. 
About `pass by reference` in C, use `void * * ` in function argument  
```c
int get_valid_directory_entry(const char * filename, void ** entry_ptr)
```
inspired by lecture <main>  

2. 
Memory allocation  from Bradley  
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
add larger files out of the limitation 
add a lot small files  √  

write with offset √  
write with offset out of the limitation  

read file √ 
read file with offset  √  
read in many different places, observe the offset  

open one file many times  


## Problems in debugging  
fs_read()  
√ cant't read the last block at first   
√ when offset is 4096*x, there exists problem.
fs_write() -- cant't written the last block at first   


  
## Source
* discussion on Piazza  

* semaphore  
lecture slides  

* segment fault handler  
http://devarea.com/linux-writing-fault-handlers/#.WvPc52bMzOQ  

* mmap()  
https://notes.shichao.io/lkd/ch15/  
https://techoverflow.net/2013/08/21/a-simple-mmap-readonly-example/  
https://gist.github.com/marcetcheverry/991042  

* pthread  
http://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread.h.html  
https://www.cs.nmsu.edu/~jcook/Tools/pthreads/library.html  

* TPS / TLS(thread local storage)  
https://en.wikipedia.org/wiki/Thread-local_storage  
https://www.ibm.com/support/knowledgecenter/en/ssw_i5_54/apis/mprotect.htm  
http://pubs.opengroup.org/onlinepubs/009696799/functions/mprotect.html  

