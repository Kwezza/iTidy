################################################################################
# Amiga GadTools resize template - single source build script (vbcc +aos68k)
################################################################################

TARGET  = amiga_window_resize_template
OBJS    = $(TARGET).o

# NOTE: Do not use the name "VBCC" here—vc relies on the VBCC env var
# to locate its config files. Overriding it breaks the compiler setup.
CC      = vc
CFLAGS  = +aos68k -c99 -cpu=68020 -g -DDEBUG -D__AMIGA__
LDFLAGS = +aos68k -cpu=68020 -g -hunkdebug -lamiga -lauto -lmieee

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
