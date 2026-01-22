################################################################################
# ReAction Test Code - Amiga Build Makefile (vbcc +aos68k only)
################################################################################

# Project name
PROJECT = testcode

# Directories
BUILD_DIR = build
BIN_DIR = bin

# Amiga build configuration (vbcc cross-compiler)
# ReAction requires AmigaOS 3.5+ libraries (window.class, layout.gadget, etc.)
# CPU TARGET: 68000 for maximum compatibility
# Note: This builds ReAction GUI code (requires ClassAct/ReAction libraries)
CC = vc
CFLAGS = +aos68k -c99 -cpu=68000 -O2 -size -I. -DPLATFORM_AMIGA=1 -D__AMIGA__
LDFLAGS = +aos68k -cpu=68000 -O2 -size -final -lamiga -lauto
OUT_DIR = $(BUILD_DIR)/amiga
BIN = $(BIN_DIR)/$(PROJECT)

################################################################################
# Source Files
################################################################################

SRCS = testcode.c

# Object files (in build directory)
OBJS = $(OUT_DIR)/testcode.o

################################################################################
# Build Rules
################################################################################

.PHONY: all clean help directories

# Default target
all: directories $(BIN)

# Create output directories
directories:
	@if not exist "$(OUT_DIR)" mkdir "$(OUT_DIR)"
	@if not exist "$(BIN_DIR)" mkdir "$(BIN_DIR)"

# Link executable
$(BIN): $(OBJS)
	@echo Linking Amiga executable: $(BIN)
	$(CC) $(LDFLAGS) -o $@ $^ 
	@echo Build complete: $(BIN)
	@echo ReAction test code compiled successfully

# Compile source file
$(OUT_DIR)/%.o: %.c
	@echo Compiling [$@] from $<
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@echo Cleaning build...
	@if exist "$(OUT_DIR)" rmdir /S /Q "$(OUT_DIR)"
	@if exist "$(BIN_DIR)" rmdir /S /Q "$(BIN_DIR)"
	@echo Clean complete.

help:
	@echo.
	@echo iTidy ReAction Test Code Makefile
	@echo ==================================
	@echo.
	@echo Available targets:
	@echo   all         Build the ReAction test executable (default)
	@echo   clean       Remove all build artifacts
	@echo   help        Display this help message
	@echo.
	@echo Build configuration:
	@echo   Compiler:   VBCC +aos68k
	@echo   CPU:        68000 (maximum compatibility)
	@echo   Output:     bin/testcode
	@echo   GUI:        ReAction (requires AmigaOS 3.5+ libraries)
	@echo.
	@echo Usage:
	@echo   make            Build the test executable
	@echo   make clean      Clean and rebuild
	@echo.
	@echo Note: Requires ReAction/ClassAct libraries installed on target Amiga
	@echo       (window.class, layout.gadget, listview.gadget, etc.)
	@echo.
