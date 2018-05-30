# Target library
lib := libfs.a

all: $(lib)

CUR_PWD := $(shell pwd)

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

objs := $(patsubst %.c, %.o, $(wildcard *.c))
# objs := disk.o fs.o
deps := $(objs:.o=.h)

$(lib): $(objs)
	@echo "AR $@"
	ar rcs $@ $^

%.o: %.c
	@echo "CC	$@"
	$(Q)$(CC) $(CFLAGS) -O -c $< $(deps)


clean: 
	@echo "CLEAN $(CUR_PWD)"
	$(Q)rm -f $(lib) $(objs) $(deps)
	# $(Q)rm -f *.a *.gch *.o



