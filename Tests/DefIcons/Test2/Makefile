# Makefile for DefIcons Creator Test Program
# Target: AmigaOS / Workbench 3.2
# Compiler: VBCC

CC = vc
CFLAGS = +aos68k -c99 -O2 -I. -I../../../include -DAMIGA
LDFLAGS = +aos68k -lamiga -lauto

TARGET = deficons_creator
SOURCES = deficons_creator.c
OBJECTS = deficons_creator_obj.o

# Output directory
OUTPUT_DIR = ../../../Bin/Amiga/Tests/DefIcons

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJECTS)
	@if not exist "$(OUTPUT_DIR)" mkdir "$(OUTPUT_DIR)"
	@copy $(TARGET) "$(OUTPUT_DIR)" >nul
	@echo "Built $(TARGET) -> $(OUTPUT_DIR)/$(TARGET)"

deficons_creator_obj.o: deficons_creator.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo "Cleaning Test2..."
	@if exist *.o del *.o
	@if exist $(TARGET) del $(TARGET)
	@if exist "$(OUTPUT_DIR)\$(TARGET)" del "$(OUTPUT_DIR)\$(TARGET)"
	@echo "Clean complete"

.PHONY: all clean
