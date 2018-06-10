ECS 150: Project #4 - File system  
I finally finish it on June 9, 2018

question:   
How to avoid conflict writing to the same file? System has a global fiel table,  
how could we implement this in user level?  


# General Information
This project is to implement the support of a very simple file system based on  
a FAT and supports up to 128 files in a single root directory.  

The file system is implemented on top of a virtual disk, which is logically  
split into blocks.  

The first software layer involved in the file system implementation is the  
block API and is provided, which is used to open or close a virtual disk,  
and read or write entire blocks from it.

Above the block layer, the FS layer is in charge of the actual  
file system management. Through the FS layer, we can mount a virtual disk,  
list the files that are part of the disk, add or delete new files,  
read from files or write to files, etc.

## Implementation of Read and Write


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




914517045
999279751
