################################################################################
# iTidy - Cross-Platform Makefile
# Supports: HOST (GCC) and AMIGA (vbcc +aos68k)
################################################################################

# Target selection: host or amiga
TARGET ?= amiga

# Project name
PROJECT = iTidy

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build

ifeq ($(TARGET),host)
    # Host build configuration (GCC on Windows/Linux/macOS)
    CC = gcc
    CFLAGS = -std=c99 -Wall -Wextra -pedantic -I$(INC_DIR) -Isrc -DPLATFORM_HOST=1
    LDFLAGS =
    OUT_DIR = $(BUILD_DIR)/host
    BIN = $(OUT_DIR)/$(PROJECT).exe
else
    # Amiga build configuration (vbcc cross-compiler)
    # VBCC MIGRATION NOTE: Changed to use VBCC v0.9x with Workbench 3.2 SDK
    # Output now goes to Bin/Amiga/ for clean separation
    CC = vc
    CFLAGS = +aos68k -c99 -cpu=68020 -I$(INC_DIR) -Isrc -DPLATFORM_AMIGA=1 -D__AMIGA__
    LDFLAGS = +aos68k -cpu=68020 -lamiga -lauto -lmieee
    OUT_DIR = $(BUILD_DIR)/amiga
    BIN_DIR = Bin/Amiga
    BIN = $(BIN_DIR)/$(PROJECT)
endif

################################################################################
# Source Files
################################################################################

# Core source files
CORE_SRCS = \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/icon_types.c \
	$(SRC_DIR)/icon_misc.c \
	$(SRC_DIR)/icon_management.c \
	$(SRC_DIR)/file_directory_handling.c \
	$(SRC_DIR)/window_management.c \
	$(SRC_DIR)/utilities.c \
	$(SRC_DIR)/spinner.c \
	$(SRC_DIR)/writeLog.c \
	$(SRC_DIR)/cli_utilities.c

# DOS subdirectory sources
DOS_SRCS = \
	$(SRC_DIR)/DOS/getDiskDetails.c

# Settings subdirectory sources
SETTINGS_SRCS = \
	$(SRC_DIR)/Settings/IControlPrefs.c \
	$(SRC_DIR)/Settings/WorkbenchPrefs.c \
	$(SRC_DIR)/Settings/get_fonts.c

# Platform-specific sources
ifeq ($(TARGET),host)
    PLATFORM_SRCS = $(SRC_DIR)/platform/host_platform.c
else
    PLATFORM_SRCS = $(SRC_DIR)/platform/amiga_platform.c
endif

# All sources
SRCS = $(CORE_SRCS) $(DOS_SRCS) $(SETTINGS_SRCS) $(PLATFORM_SRCS)

# Object files (in build directory)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OUT_DIR)/%.o)

################################################################################
# Build Rules
################################################################################

.PHONY: all clean help host amiga directories

# Default target
all: directories $(BIN)

# Host build shortcut
host:
	@$(MAKE) TARGET=host

# Amiga build shortcut
amiga:
	@$(MAKE) TARGET=amiga

# Create output directories
# VBCC MIGRATION NOTE: Added Bin/Amiga directory for final Amiga executables
directories:
	@if not exist "$(OUT_DIR)" mkdir "$(OUT_DIR)"
	@if not exist "$(OUT_DIR)\DOS" mkdir "$(OUT_DIR)\DOS"
	@if not exist "$(OUT_DIR)\Settings" mkdir "$(OUT_DIR)\Settings"
	@if not exist "$(OUT_DIR)\platform" mkdir "$(OUT_DIR)\platform"
ifeq ($(TARGET),amiga)
	@if not exist "$(BIN_DIR)" mkdir "$(BIN_DIR)"
endif

# Link executable
# VBCC MIGRATION NOTE: For Amiga builds, copy to Bin/Amiga/ after linking
$(BIN): $(OBJS)
	@echo Linking $(TARGET) executable: $(BIN)
	$(CC) $(LDFLAGS) -o $@ $^
ifeq ($(TARGET),amiga)
	@echo Copying to Bin/Amiga/
	@copy "$(OUT_DIR)\$(PROJECT)" "$(BIN_DIR)\$(PROJECT)" >nul
endif

# Compile core source files
$(OUT_DIR)/%.o: $(SRC_DIR)/%.c
	@echo Compiling [$@] from $<
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@echo Cleaning $(TARGET) build...
	@if exist "$(OUT_DIR)" rmdir /S /Q "$(OUT_DIR)"
	@echo Clean complete.

# Clean all targets
clean-all:
	@echo Cleaning all builds...
	@if exist "$(BUILD_DIR)" rmdir /S /Q "$(BUILD_DIR)"
	@echo Clean complete.

# Help target
help:
	@echo iTidy Build System
	@echo ==================
	@echo.
	@echo Targets:
	@echo   make                    - Build for Amiga (default)
	@echo   make TARGET=host        - Build for host PC (GCC)
	@echo   make TARGET=amiga       - Build for Amiga (vbcc)
	@echo   make host               - Shortcut for host build
	@echo   make amiga              - Shortcut for Amiga build
	@echo   make clean              - Clean current target build
	@echo   make clean-all          - Clean all builds
	@echo   make help               - Show this help
	@echo.
	@echo Build Outputs:
	@echo   Host:   $(BUILD_DIR)/host/$(PROJECT).exe
	@echo   Amiga:  $(BUILD_DIR)/amiga/$(PROJECT)
	@echo.
	@echo Current Configuration:
	@echo   TARGET:  $(TARGET)
	@echo   CC:      $(CC)
	@echo   OUT_DIR: $(OUT_DIR)
	@echo   BIN:     $(BIN)

################################################################################
# Dependencies
################################################################################

# Core dependencies (simplified - expand as needed)
$(OUT_DIR)/main.o: $(SRC_DIR)/main.c $(SRC_DIR)/main.h
$(OUT_DIR)/icon_types.o: $(SRC_DIR)/icon_types.c $(SRC_DIR)/icon_types.h
$(OUT_DIR)/icon_misc.o: $(SRC_DIR)/icon_misc.c $(SRC_DIR)/icon_misc.h
$(OUT_DIR)/icon_management.o: $(SRC_DIR)/icon_management.c $(SRC_DIR)/icon_management.h
$(OUT_DIR)/file_directory_handling.o: $(SRC_DIR)/file_directory_handling.c $(SRC_DIR)/file_directory_handling.h
$(OUT_DIR)/window_management.o: $(SRC_DIR)/window_management.c $(SRC_DIR)/window_management.h
$(OUT_DIR)/utilities.o: $(SRC_DIR)/utilities.c $(SRC_DIR)/utilities.h
$(OUT_DIR)/spinner.o: $(SRC_DIR)/spinner.c $(SRC_DIR)/spinner.h
$(OUT_DIR)/writeLog.o: $(SRC_DIR)/writeLog.c $(SRC_DIR)/writeLog.h
$(OUT_DIR)/cli_utilities.o: $(SRC_DIR)/cli_utilities.c $(SRC_DIR)/cli_utilities.h

# DOS subdirectory
$(OUT_DIR)/DOS/getDiskDetails.o: $(SRC_DIR)/DOS/getDiskDetails.c $(SRC_DIR)/DOS/getDiskDetails.h

# Settings subdirectory
$(OUT_DIR)/Settings/IControlPrefs.o: $(SRC_DIR)/Settings/IControlPrefs.c $(SRC_DIR)/Settings/IControlPrefs.h
$(OUT_DIR)/Settings/WorkbenchPrefs.o: $(SRC_DIR)/Settings/WorkbenchPrefs.c $(SRC_DIR)/Settings/WorkbenchPrefs.h
$(OUT_DIR)/Settings/get_fonts.o: $(SRC_DIR)/Settings/get_fonts.c $(SRC_DIR)/Settings/get_fonts.h

# Platform-specific
$(OUT_DIR)/platform/host_platform.o: $(SRC_DIR)/platform/host_platform.c $(INC_DIR)/platform/platform.h
$(OUT_DIR)/platform/amiga_platform.o: $(SRC_DIR)/platform/amiga_platform.c $(INC_DIR)/platform/platform.h

# All objects depend on platform headers
$(OBJS): $(INC_DIR)/platform/platform.h $(INC_DIR)/platform/platform_types.h

# End of Makefile
