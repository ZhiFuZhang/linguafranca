
SRC = nfs_api.c main.c
OBJ = $(SRC:.c=.o)

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
	$(CC) $(INCLUDE) $(CFLAGS) -c $<

clean:
	-rm  -f $(OBJ) $(PROGRAM)

