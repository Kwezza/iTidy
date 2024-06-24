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
    $(SC) link to main $(CFLAGS) main.o  Settings/IControlPrefs.o  Settings/WorkbenchPrefs.o icon_management.o

main.o: main.c
    $(SC) $(CFLAGS) main.c Settings/IControlPrefs.c  Settings/WorkbenchPrefs.c icon_management.c

clean:
    delete main.o main main.info main.lnk Settings/IControlPrefs.o  Settings/WorkbenchPrefs.o icon_management.o