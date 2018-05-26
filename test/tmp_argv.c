#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>      /* printf, scanf, puts */
#include <stdlib.h>     /* realloc, free, exit, NULL */
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

const char m1[9] = "message 1";
const char m2[6] = "info 1";

//gcc -o tmp_argv tmp_argv.c 
int main(int argc, char **argv)
{
    /* realloc */
    char * cstr = NULL;
    void *nullstr;
    printf("%lu\n", sizeof(m1));
    printf("%lu\n", sizeof(cstr));
    cstr = malloc(sizeof(m1));
    nullstr = cstr;
    memset(cstr, 0, sizeof(m1));
    printf("cstr len with all zeros = %lu\n", strlen(cstr));
    memset(cstr, 'a', 3);
    printf("cstr len with 3 a = %lu\n", strlen(cstr));
    printf("nullstr len with 3 a = %lu\n", strlen(nullstr));
    memcpy(cstr, m1, sizeof(m1));
    printf("cstr size = %lu; cstr = %s\n", strlen(cstr), cstr);
    if(realloc(cstr, sizeof(m1) + sizeof(m2)) == NULL) return -1;
    memcpy(cstr + sizeof(m1), m2, sizeof(m2));
    printf("cstr size = %lu; cstr = %s\n", strlen(cstr), cstr);
    return 0;

    /* input argc */
	char *program;

	program = argv[0];

	if (argc == 1)
		printf("argc = 1, str = %s\n",program );
	else {
        printf("argc = %d\nlength of the first argument = %d\n", argc, (int)strlen(argv[1]));
    }

    /* char *, char array print */
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


	return 0;
}
