ECS 150: Project #4 - File system  
# General Information
This is the second part of the last project -- an user-level thread library,  
which is to provide an interface for applications to create, schedule and  
execute threads concurrently. This time, we focus on implementing Semaphres,  
for efficient thread synchronization by using waiting queues and per-thread   
protected memory regions, which is transparently accessible by an API and  
protected from other threads.

## Semaphore API
After understanding hwo the given testers work, the semaphore implementation  
is very straighforward.  
In our `sem.c`, I create a structure to hold the waiting list and count:  
```c
struct semaphore {
    int count; //represents the number of resources
    queue_t waiting_queue; // queue of threads waiting for this semaphore  
};
```
The API contains:  
```c
sem_t sem_create(size_t count);  
int sem_destroy(sem_t sem);  
int sem_down(sem_t sem);
int sem_up(sem_t sem);
```
In `sem_create()`, the variable `count` and `waiting_queue` are initialized --  
`count` represents the number of resources. In `sem_destroy()`, we free the  
memory allocated to the `waiting_queue` and the semaphore object, after  
checking the input is legal and the `waiting_queue` is empty.  

In `sem_down()`, after checking the semaphore object is legal,  
`enter_critical_section()` is invoked to support the lock function. Then the  
current running thread decrease `count` or is blocked if it is already zero.  
If blocked, the thread's id is pushed into `waiting_queue`. At last  
`exit_critical_section()` is invoked so other threads are able to grab the 
"lock".  

In `sem_up()`, the operation is nearly the same, just opposite. Also,  
`enter_critical_section()` and `exit_critical_section()` are invoked to protect  
it's not interrupted by other threads when the counter and queue's states are  
changing. At last, the thread increases the counter and wakes up the thread  
whose id is in the front of the `waiting_queue` if it's not empty. 


### Testing
Those three given testing programs are sufficient to test our semaphore  
implementation. I just print out our test results and the workflow of the  
testers.
* sem_count: simple test with two threads and two semaphores  
```c
circle@pc32:~/ecs150psGit/p3_thread/test$ ./sem_count.x 10
thread 2, x = 0
thread 1, x = 1
thread 2, x = 2
thread 1, x = 3
...
thread 2, x = 8
thread 1, x = 9
```
* sem_buffer: producer/consumer exchanging data in a buffer
```c
circle@pc32:~/ecs150psGit/p3_thread/test$ ./sem_buffer.x 10
Producer wants to put 10 items into buffer...
Producer is putting 0 into buffer
Producer is putting 1 into buffer
Producer is putting 2 into buffer
...
Producer is putting 7 into buffer
Producer is putting 8 into buffer
Producer is putting 9 into buffer
Consumer wants to get 2 items out of buffer...
Consumer is taking 0 out of buffer
Consumer is taking 1 out of buffer
Consumer wants to get 8 items out of buffer...
Consumer is taking 2 out of buffer
Consumer is taking 3 out of buffer
...
Consumer is taking 8 out of buffer
Consumer is taking 9 out of buffer
```
* sem_prime: prime sieve implemented with a growing pipeline of threads
```c
circle@pc32:~/ecs150psGit/p3_thread/test$ ./sem_prime.x 30
2 is prime.
3 is prime.
5 is prime.
7 is prime.
11 is prime.
13 is prime.
17 is prime.
19 is prime.
23 is prime.
29 is prime.
```

## TPS API
The goal of the TPS API is to provide a single private and protected their  
own memory page,while keeping the sharing memory address space. Here we are  
given `TPS_SIZE` of 4096, 

The API contains:  
```c
int tps_init(int segv);
int tps_create(void);
int tps_destroy(void);
int tps_read(size_t offset, size_t length, char *buffer);
int tps_write(size_t offset, size_t length, char *buffer);
int tps_clone(pthread_t tid);
```

In our `tps.c`, we have two global variables:
```c
static char ini = 0; // to indicate the initialization state
queue_t tps_q; // a queue to store our tps information
```

Before implementing Copy-on-Write clonging, we only need one simple structure  
like this:
```c
struct node
{
    pthread_t tid;
    void * tps;  //pointer to the tps memory space allocated to tid thread
};
```
Copy-on-Write cloning requires one more structure and the original organization  
becomes a nested one. The two structures are important in TPS implementation:  
```c
struct mem_page
{
    int ref_counter; // numbers of thread connected to this memory page
    void * memAdd;   // pointer to the address 
};
```
```c
struct node
{
    pthread_t tid;
    mp_t tps; // pointer to the structure mem_page
};
```
So the whole structure is like:  
For example, thread of tid2 clone tps from tps1 and hasn't do any writing yet.  
```c
[node1] --> [node2] --> [node3] --> D[tail]   |The Queue |  
 tid1          tid2       tid3
 tps1          tps2       tps3
   |           |           |  
   |___________|           |  
   |                       |  
   V                       V  
[mem_page1]            [mem_page2]            |The Memory Page structure|  
 ref_counter = 2         ref_counter = 1
 memAdd                  memAdd
   |           |           |  
   |___________|           |  
   |                       |  
   V                       V
[memory address]       [memory address] 
```


We also create two helper functions `findThread` and `findAddress`, which  
cooperate with `queue_iterate`. The former function is needed when we do the  
reading or writing operation; the latter one is needed by `segv_handler` to  
distinguish between the usual programming errors and accessing a protected  
TPS error.
```c
/* to find the tps existence in our tps_queue via thread_id */
int findThread(void *node, void * tid); 
/* to find the memory address existence in our tps queue */ 
int findAddress(void *node, void * add); 
```

In `tps_init()`, we set up the signal handler for the segment fault, initialize  
the global queue `tps_q`.

In `tps_create()`, at first we get the tid of the current running thread via  
`pthread_self()`, and check whether it exists in `tps_q`. If not, we create  
a new node and enqueue it. The node contains a pointer to the `mem_page`  
structure, which has its variable `ref_counter = 1` and the memory space of  
4096 allocated by `mmap` with flag `PROT_NONE` to protect it with no read/write  
permissions.  

In `tps_destroy()`, we find and delete the relative node from `tps_q` and free  
all the space allocated to it, which including space for the memory page and  
the structure itself. One thing, we need to care about after implementing Copy  
-on-Write cloning, is that if the memory page's `ref_counter` is bigger  
than 1, we just decrease the counter, dequeue and delete the node in the `tps_q  
queue without free the memory address.  

`tps_read()` is comparatively simple. At first we check if the current thread  
has a TPS, if the reading operation is out of bound or if the buffer is NULL.  
If everything is legal, `mprotect` is invoked to change the reading permission  
temporarily and `memcpy` to write.  

`tps_clone()` is very straightforward, we find the relative memory page via the  
input thread id argument, create a new tps node poiting to the same memory page  
space and push it to the `tps_q`, and increase the memory page's `ref_counter`.

`tps_write()` is a little bit complex comparing to the other functions. At  
first, we check if the current thread has a TPS, if the writing operation is  
out of bound or if the buffer is NULL. If everything is legal, we find it's  
memory page and check it's `ref_counter`. If it's 1, like reading, `mmap` is  
invoked to change the writing permission temporarily and `memcpy` to write  
directly. If not, we create a new memory page structure like what `tps_create()`  
does, decrease the original memory page structure's `ref_counter`, copy the  
original content to the new memory space and write to it. `mprotect` is invoked  
to forbid read/write at last.

###  Unprotected TPS with naive cloning
It's the first version of the API, which has no fancy things. And as we  
implement more functions, the version is abandoned.

###  Protected TPS, Copy-on-Write cloning
The implementation for these part have been described in the function  
implementation above. 

### Tester
I print the relative erro information when we test it. The way I test it is  
illustrated in the comments.   

* tps.x  
```c
circle@pc32:~/ecs150psGit/p3_thread/test$ ./tps.x

thread2: read OK!
thread1: read OK!
thread2: read OK!
thread1: read OK!
```
  
* tps_protection.x  
I wrap the `mmap` and get the address via `char * tps_addr = latest_mmap_addr`  
and try to read and write function outside `tps_read()` and `tps_write`.
```c
circle@pc32:~/ecs150psGit/p3_thread/test$ tps_protection.x

thread1: read OK! The content is : Hello world!
thread1: read after change. The content is : Hello Project # 3
thread1: read partial. The second half content is : Project # 3
try to read outside tps_read() 
TPS protection error!           // printf("%s\n", tps_addr)
Segmentation fault (core dumped)
...
try to write outside tps_read() // tps_addr[5] = 'X';
TPS protection error!
Segmentation fault (core dumped)
```
  
* tps_checkAdd.x  
A simple version of `tps_offsets`.  
```c
circle@pc32:~/ecs150psGit/p3_thread/test$ tps_checkAdd.x

thread: read OK. The conent : Hello world!
thread: reading with offset 6. The content is : world!
```
  
* tps_offsets.x  
Here I modify the `tps.c` so it will print the error information and check all  
the possibility when I read and write the memory page.  
You can see those from the print-out messages.  
```c
circle@pc32:~/ecs150psGit/p3_thread/test$ tps_offsets.x

tps does not exist // invoke tps_read() before tps_create()
writing operation is out of bound // invoke tps_write(5, TPS_SIZE, msg1) 
reading operation is out of bound // tps_read(5, TPS_SIZE, msg1)
buffer is NULL
thread2: read OK. The content : Hello world!
thread2: reading with offset 6. The content is : world! 
// tps_read(6, TPS_SIZE-6, buffer)
thread2: after write with offset. The content is : Hello ECS150  
//tps_write(6, 7, offset1);
thread1: read OK. The content is cloned from thread2: Hello ECS150
thread2: read again. The content is: Hello ECS150
thread1: read after change. The content is : Hello Project # 3
thread1: read partial. The second half content is : Project # 3
```
  
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
