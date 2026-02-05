# Makefile for hexdump utility
# VBCC cross-compiler for AmigaOS

# Compiler and tools
CC = vc
RM = del /Q

# Compiler flags
CFLAGS = +aos68k -c99 -O2

# Linker flags
LDFLAGS = -lamiga -lauto

# Target executables
TARGET1 = hexdump
TARGET2 = deficonsdump
TARGET3 = deficontree

# Source files
SOURCES1 = hexdump.c
SOURCES2 = deficonsdump.c
SOURCES3 = deficontree.c

# Object files
OBJECTS1 = hexdump.o
OBJECTS2 = deficonsdump.o
OBJECTS3 = deficontree.o

# Default target
all: $(TARGET1) $(TARGET2) $(TARGET3)

# Link the executables
$(TARGET1): $(OBJECTS1)
	$(CC) $(CFLAGS) -o $(TARGET1) $(OBJECTS1) $(LDFLAGS)
	@echo.
	@echo Build complete: $(TARGET1)

$(TARGET2): $(OBJECTS2)
	$(CC) $(CFLAGS) -o $(TARGET2) $(OBJECTS2) $(LDFLAGS)
$(TARGET3): $(OBJECTS3)
	$(CC) $(CFLAGS) -o $(TARGET3) $(OBJECTS3) $(LDFLAGS)
	@echo.
	@echo Build complete: $(TARGET3)

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	-$(RM) $(OBJECTS1) $(OBJECTS2) $(OBJECTS3) $(TARGET1) $(TARGET2) $(TARGET3) 2>nul

# Rebuild everything
rebuild: clean all

.PHONY: all clean rebuild
