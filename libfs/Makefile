# Target library
lib := libfs.a

all: $(lib)

objs = disk.o fs.o
deps = $(objs:.o=.h)

CC := gcc
CFLAGS	:= -Wall -Werror
ifneq ($(D),1)
CFLAGS	+= -O2
else
CFLAGS	+= -O0 #capital  o + zero
CFLAGS	+= -g
endif

ifneq ($(V),1)
Q = @
V = 0
endif


%.o: %.c
	@echo "CC	$@"
	$(Q)$(CC) $(CFLAGS) -O -c $< $(deps)

$(lib): $(objs)
	@echo "AR $@"
	ar rcs $@ $^


clean: 
	@echo "clean"
	$(Q)rm -f *.a *.gch *.o



