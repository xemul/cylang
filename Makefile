OBJS := cyvm.o
OBJS += resolver.o
OBJS += tokenizer.o
OBJS += aryth.o
OBJS += syms.o
OBJS += declare.o
OBJS += show.o
OBJS += list.o
OBJS += map.o
OBJS += string.o
OBJS += cblock.o
OBJS += cond.o
OBJS += rbtree.o
OBJS += misc.o

CFLAGS += -Wall -Werror

cyvm: $(OBJS)
	gcc -o $@ $^

%.o: %.c cy.h
	$(CC) $(CFLAGS) -c -o $@ $<
