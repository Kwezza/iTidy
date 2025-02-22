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
    $(SC) link to iTidy $(CFLAGS) icon_misc.o main.o dos/getDiskDetails.o writeLog.o Settings/IControlPrefs.o  Settings/WorkbenchPrefs.o icon_management.o window_management.o file_directory_handling.o icon_types.o utilities.o spinner.o settings/get_fonts.o

main.o: main.c
    $(SC) $(CFLAGS) main.c icon_misc.c dos/getDiskDetails.c writeLog.c Settings/IControlPrefs.c  Settings/WorkbenchPrefs.c icon_types.c icon_management.c window_management.c window_management.c file_directory_handling.c  utilities.c spinner.c settings/get_fonts.c

clean:
    delete main.o main iTidy main.info main.lnk icon_misc.o icon_misc.lnk dos/getDiskDetails.o dos/getDiskDetails.lnk writeLog.o writeLog.lnk Settings/IControlPrefs.o  Settings/WorkbenchPresmake cleafs.o icon_management.o window_management.o window_management.o file_directory_handling.o icon_types.o utilities.o spinner.o settings/get_fonts.o settings/get_fonts.lnk