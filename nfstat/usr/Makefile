
SRC = nfs_api.c main.c
OBJ = $(SRC:.c=.o)
MIDDLe = $($SRC:.c=.sc)
PROGRAM = nfstest
CC = gcc

INCLUDE =  
LINKPARAM = 
CFLAGS = -g -Wall -Werror

default:$(PROGRAM)

$(PROGRAM):$(OBJ)
	$(CC) -o $(PROGRAM)  $(LINKPARAM) $(OBJ)

.SUFFIXES : .c

.c.o:
	$(CC) $(INCLUDE) $(CFLAGS) -c  $<

middle:
	$(CC) $(INCLUDE) $(CFLAGS) -E $(SRC) > middle.c.s
clean:
	-rm  -f $(OBJ) $(PROGRAM) middle.c.s

