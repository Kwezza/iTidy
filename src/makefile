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
    $(SC) link to ITidy $(CFLAGS) main.o  Settings/IControlPrefs.o  Settings/WorkbenchPrefs.o icon_management.o window_management.o file_directory_handling.o icon_types.o utilities.o

main.o: main.c
    $(SC) $(CFLAGS) main.c Settings/IControlPrefs.c  Settings/WorkbenchPrefs.c icon_management.c window_management.c window_management.c file_directory_handling.c icon_types.c utilities.c

clean:
    delete main.o main main.info main.lnk Settings/IControlPrefs.o  Settings/WorkbenchPresmake cleafs.o icon_management.o window_management.o window_management.o file_directory_handling.o icon_types.o utilities.o