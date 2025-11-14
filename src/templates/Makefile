################################################################################
# iTidy - Template Files Makefile
# Cross-compilation for Amiga using vbcc +aos68k
# 
# This Makefile compiles standalone template demonstration programs
# for testing and validation purposes. These templates are completely
# self-contained with no dependencies on the main iTidy project.
################################################################################

# Project configuration
PROJECT = amiga_window_template

# Directories (only for NDK includes)
BUILD_DIR = ../../build/templates
BIN_DIR = ../../Bin/Amiga/Templates

# Amiga cross-compiler configuration (vbcc)
# Note: Standalone template - no project includes needed
CC = vc
CFLAGS = +aos68k -c99 -cpu=68020 -g
LDFLAGS = +aos68k -cpu=68020 -g -hunkdebug -lamiga -lauto -lmieee

################################################################################
# Source Files
################################################################################

# Template window sources
TEMPLATE_SRCS = \
	amiga_window_template.c \
	test_window_template.c

# All sources
SRCS = $(TEMPLATE_SRCS)

# Object files
OBJS = $(TEMPLATE_SRCS:.c=.o)

# Final executable
BIN = $(PROJECT)

################################################################################
# Build Rules
################################################################################

.PHONY: all clean directories help

# Default target
all: directories $(BIN)

# Create output directories
directories:
	@if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
	@if not exist "$(BIN_DIR)" mkdir "$(BIN_DIR)"

# Link executable
$(BIN): $(OBJS)
	@echo Linking Amiga executable: $(BIN)
	$(CC) $(LDFLAGS) -o $@ $^
	@echo Build complete: $(BIN)
	@echo Debug symbols embedded in executable

# Compile template source files
%.o: %.c
	@echo Compiling [$@] from $<
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@echo Cleaning template build...
	@if exist "*.o" del /Q *.o
	@if exist "$(BIN)" del /Q "$(BIN)"
	@echo Clean complete.

# Help target
help:
	@echo Template Build System
	@echo =====================
	@echo.
	@echo Targets:
	@echo   make          - Build template test program
	@echo   make clean    - Clean build artifacts
	@echo   make help     - Show this help
	@echo.
	@echo Current Configuration:
	@echo   CC:      $(CC)
	@echo   BIN:     $(BIN)
	@echo   CFLAGS:  $(CFLAGS)

################################################################################
# Dependencies
################################################################################

# Template dependencies - self-contained, only depend on own header
amiga_window_template.o: amiga_window_template.c amiga_window_template.h
test_window_template.o: test_window_template.c amiga_window_template.h

# End of Makefile
