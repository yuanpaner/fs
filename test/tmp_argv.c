#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int fd[16] = {-1};

//gcc -o tmp_argv tmp_argv.c 
int main(int argc, char **argv)
{
	char *program;

	program = argv[0];

	if (argc == 1)
		printf("argc = 1, str = %s\n",program );
	else {
        printf("argc = %d\nlength of the first argument = %d\n", argc, (int)strlen(argv[1]));
    }

    printf("test empty char, neither is allocated...\n");
    char *str1;// = malloc(10);
    char str2[8];// = malloc(10);
    // str[0] = '\0';
    printf("char* str\t=%s\n", str1); // null
    printf("char str[]\t=%s\n", str2); //???

    printf("allocated...\n");
    char * str3 = malloc(10);
    char * str4 = malloc(10);
    str4[0] = '\0';
    printf("char* str\t=%s\n", str3);
    printf("set null at first, char* str\t=%s\n", str4); //???


    static char msg1[3] = "AAA";
    char chararr[4];
    memcpy(chararr, msg1, 4);
    printf("char array\t=%s\n", chararr);
    memset(chararr, 0, 4);
    printf("after memset 0 array\t=%s\n", chararr);

    for (int i = 0; i < 16; ++i)
    {
        printf("fd[%d] = %d\n", i, fd[i]);
    }

	return 0;
}
