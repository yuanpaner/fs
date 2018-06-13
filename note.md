ECS 150: Project #4 - File system



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

- [ ] https://stackoverflow.com/questions/11830979/c-strcpy-function-copies-null  
about filename and memory copy  

- [ ] http://www.runoob.com/linux/linux-shell-passing-arguments.html  
shell tutorial  

- [ ] it fail my own test_fs_yuan.sh when adding clear(), check the script and code; another branch is good  

## General Framework

## Mounting/unmounting

## File creation/deletion
If file size is zero, the file entry in root directory should be added while no block is allocated to this file.
## File descriptor operations

## File reading/writing



 

## Test output

- [ ] unmount "cannot be closed, or if there are still open file descriptors."  


- [ ] should module the metadata writing to a function and invoke each time any data updated, instead of only put in fs_unmount  
piazza @424  
Update by prof: as a general rule, there's only so much that an API can do. If the API imposes a sequence of operations (eg call mount() and unmount() at the end), then it's up to the client to respect such sequence.   
Be mindful that there are two kind of data that can be written while a disk is mounted: actual file data, and metadata (FAT entries and root directory entries). If it's OK that one is delayed, it probably doesn't make sense to delay the other one.  


> fs_read  

- [X] repeated filename    
- [X] what file name we use  
For example, if I add a file ../filename to the test.fs. Should I add it as "filename" or as "../filename" directly?  
- [X] write with different filenames  
Your program should be able to handle filenames no larger than 16 bytes  
And my question is more about the requirement, like if we add a file from the parent folder (../), which filename are we expected to use, "../filename", or just "filename" ?   
I think I got it.  
<strong>FYI. I use fs_ref.x to try that, and it shows, using "../filename", although I think it a little weird.   
In my opinion, the file in the virtual disk is an independent new file unrelated to the copied one, and we should remove the absolute path included in the filename. But for this assignment I think it understandable to keep it, for testing conveniently.</strong>  
piazza @499 answered by Joël  
Whatever filename is received by fs_create() is the correct filename that your library should use.  

15-char name, added, pass  
16-char name, unable to added, pass  
name with space,  added,  pass   

- [ ] filename which is not null terminator ? char array? try


- [X] info, ls
- [ ] what is script  
piazza @ 498  
The "script" subcommand is used by the grading script to test your code. It isn't meant for you to use (unless you can figure out how to reverse-engineer it!).  

Everything that this script subcommand does can be reproduced by coding your own testers.  


- [ ] file delete with the fat erase  


- [ ] read with cat
- [ ] read with offset? multiple read? Each has an independent offset.   
- [ ] read when writing
So I need to update the file_sz very carefully.  


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

- [ ] writing in the middle, with offset!!! 
@496  
We just overwrite the old content. -- by Bradley  
That's when you realize that inserting a line in a text file technically moves the rest of the content farther. -- by Joël  


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

 



