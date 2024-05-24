# Makefile for SAS/C on Amiga
SC = sc
LINK = slink

# Compilation flags
CFLAGS = noicon StackExtend

# Linker flags
LFLAGS = 

# Default target
all: main

main: main.o
    $(SC) link to main $(CFLAGS) main.o 

main.o: main.c
    $(SC) $(CFLAGS) main.c 

clean:
    delete main.o main main.info main.lnk 