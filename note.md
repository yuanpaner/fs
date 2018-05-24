ECS 150: Project #4 - File system

- [ ] memory leak for phase 1

## General Framework

## Mounting/unmounting

## File creation/deletion

## File descriptor operations

## File reading/writing




  
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

project  
http://www.openvirtualization.org/documentation/fat32_8c_source.html  
http://codeandlife.com/2012/04/07/simple-fat-and-sd-tutorial-part-2/  

links  
https://www.edaboard.com/showthread.php?176568-C-code-for-FAT-file-implementation-using-PIC18F4550
  
<sys/stat.h>  

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


## Else
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