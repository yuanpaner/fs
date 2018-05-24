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

int main(int argc, char **argv)
{
	char *program;

	program = argv[0];

	if (argc == 1)
		printf("argc = 1, str = %s\n",program );
	else {
        printf("argc = %d\nlength of the first argument = %d\n", argc, (int)strlen(argv[1]));
    }


	return 0;
}
