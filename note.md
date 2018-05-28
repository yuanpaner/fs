ECS 150: Project #4 - File system

感觉我的write没必要一开始计算offset啊，因为create之后就立马会write，总是从0开始的。。。我为什么要这么搞？  有没有可能我可以对一个file write很多次，例如append这种，从结尾开始。我的code已经具备这个功能，但是需要一个合适的test。  
  
可以open好几次？在不同的地方读写？ 每次读写都会移动相同的offset还是。听老师上课的意思互相独立那就是各有各的offset.  
如果不是用dup2这种，那就是independent fd。  
读ok，缺功能， 每个文件都从不同的地方读。  
写怎么办。我在csif上测试的结果是，写的时候会报提醒！在文件本身，设置一个open的检查。不能同时write。每次都是从末尾写么，会不会从中间写。功能已有，缺test检查。  

Read 也一样，会不会有fd1在write，fd2在read，并且同时进行，那么就要用事实更新的file_sz  
见test  

w_dir_entry->first_data_blk == 0xFFFF 说明block的个数怎么都不会是0xFFFF个对吧。power(2, 16) * 4K  

关于fd和filename  
```c
int fs_create(const char *filename);
int fs_delete(const char *filename);
int fs_open(const char *filename);

// after create, and open, we should use file descriptor to keep track of the file status
// fd contains: filename, offset, and file entry shows the open and closed status
int fs_close(int fd);
int fs_stat(int fd);
int fs_lseek(int fd, size_t offset);
int fs_write(int fd, void *buf, size_t count);
int fs_read(int fd, void *buf, size_t count);
```

## General Framework

## Mounting/unmounting

## File creation/deletion
If file size is zero, the file entry in root directory should be added while no block is allocated to this file.
## File descriptor operations

## File reading/writing



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


struct stat, and fstat  
http://pubs.opengroup.org/onlinepubs/009695399/basedefs/sys/stat.h.html  
http://pubs.opengroup.org/onlinepubs/009695399/functions/fstat.html

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



## Test output
- [X] info, ls
- [ ] what is script  
piazza 

- [ ] read with cat
- [ ] read with offset? multiple read? Each has an independent offset.   
- [ ] read when writing
So I need to update the file_sz very carefully.  

- [ ] what file name we use  
For example, if I add a file ../filename to the test.fs. Should I add it as "filename" or as "../filename" directly?  

- [ ] write with different filenames  
Your program should be able to handle filenames no larger than 16 bytes  
And my question is more about the requirement, like if we add a file from the parent folder (../), which filename are we expected to use, "../filename", or just "filename" ?   
I think I got it.  
FYI. I use fs_ref.x to try that, and it shows, using "../filename", although I think it a little weird.   
In my opinion, the file in the virtual disk is an independent new file unrelated to the copied one, and we should remove the absolute path included in the filename. But for this assignment I think it understandable to keep it, for testing conveniently.   

- [X] write one small file
- [X] write one large file
- [X] write one large file out of boundary
- [ ] write several small files
- [ ] write lots of small files, out of the range of directory entry - FS_FILE_MAX_COUNT
- [ ] write several median files out of block boundary

- [ ] write several large files
- [ ] write several large files out of boundary 

- [ ] write to same files, multiple writes?
need using new c file to test  
the csif prompt when I try to edit one file simoutaneously:  
(1) Another program may be editing the same file.  If this is the case, be careful not to end up with two different instances of the same file when making changes.  Quit, or continue with caution.  
(2) An edit session for this file crashed.
    If this is the case, use ":recover" or "vim -r run.sh" to recover the changes (see ":help recovery").  
    If you did this already, delete the swap file ".run.sh.swp" to avoid this message.  
It's ok if I use `vim -R` to open a file(Readonly) simoutaneously. 

- [ ] add a "clean" to the test, remove all the files in the virtual disk



- [X] write to full disk  
one directory entry used, no block assigned 
√ data blk exhausted  
√ Segmentation fault (core dumped)  


- [X] remove regular file  
- [X] remove empty file  
- [ ] remove file which is opened by others????

- [ ] memory leak for phase 1
- [ ] group all the malloc and free in function
- [X] phase 1, FAT
- [X] fat_free_ratio=4095/4096 ? rdir_free_ratio=128/128 ?
- [X] don't assign block in @fs_create, in case the file is empty. 

```c
$ fs_ref.x add disk run.sh
Wrote file 'run.sh' (0/121 bytes)
$ fs_ref.x info disk
FS Info:
total_blk_count=303
fat_blk_count=1
rdir_blk=2
data_blk=3
data_blk_count=300
fat_free_ratio=0/300
rdir_free_ratio=124/128 // decrease
```






```c
int main(int argc, char **argv)
{
    char *program;

    program = argv[0];

    if (argc == 1)
        printf("argc = 1 %s\n",program );
    else printf("argc = %d\n", argc);

    return 0;
}

//gcc -o tmp_argv tmp_argv.c 
```

```c
// execute result
$ ./tmp_argv 123
argc = 2
$ ./tmp_argv
argc = 1 ./tmp_argv
$ ./tmp_argv 123 456
argc = 3
```


```c 
// fs_ref.x
$fs_ref.x info disk
FS Info:
total_blk_count=1003
fat_blk_count=1
rdir_blk=2
data_blk=3
data_blk_count=1000
fat_free_ratio=999/1000
rdir_free_ratio=128/128

$fs_ref.x add disk Makefile
Wrote file 'Makefile' (1434/1434 bytes)

$fs_ref.x add disk fun.sh
open: No such file or directory

$fs_ref.x add disk test_fs.d
Wrote file 'test_fs.d' (35/35 bytes)

$fs_ref.x add disk run.sh
Wrote file 'run.sh' (117/117 bytes)

$fs_ref.x info disk
FS Info:
total_blk_count=1003
fat_blk_count=1
rdir_blk=2
data_blk=3
data_blk_count=1000
fat_free_ratio=996/1000
rdir_free_ratio=125/128

$fs_ref.x ls disk
FS Ls:
file: Makefile, size: 1434, data_blk: 1
file: test_fs.d, size: 35, data_blk: 2
file: run.sh, size: 117, data_blk: 3
$fs_ref.x stat disk Makefile
Size of file 'Makefile' is 1434 bytes

$fs_ref.x rm disk Makefile
Removed file 'Makefile'

$fs_ref.x info disk
FS Info:
total_blk_count=1003
fat_blk_count=1
rdir_blk=2
data_blk=3
data_blk_count=1000
fat_free_ratio=997/1000
rdir_free_ratio=126/128

$fs_ref.x ls disk 
FS Ls:
file: test_fs.d, size: 35, data_blk: 2
file: run.sh, size: 117, data_blk: 3
```

 



