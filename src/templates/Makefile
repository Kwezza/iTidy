################################################################################
# Amiga GadTools Templates - Build Script (vbcc +aos68k)
################################################################################

# Target executable
TARGET  = amiga_window_resize_template
OBJS    = $(TARGET).o

# Compiler and flags
CC      = vc
CFLAGS  = +aos68k -c99 -cpu=68020 -size -D__AMIGA__
LDFLAGS = +aos68k -cpu=68020 -final -s -lamiga -lauto -lmieee

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo Linking $@
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.c
	@echo Compiling $<
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo Cleaning template artifacts
	@if exist $(TARGET).o del /Q $(TARGET).o
	@if exist $(TARGET) del /Q $(TARGET)
