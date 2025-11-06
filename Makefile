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
    # VBCC MIGRATION NOTE (Stage 4): Added -DDEBUG for debug logging
    # DEBUG BUILD: Added -g for debug symbols, -hunkdebug for symbol table
    # Note: VBCC warnings from system headers (51: bitfield, 61: array size) cannot be suppressed
    # EASY_REQUEST_HELPER: -DBUILD_WITH_MOVEWINDOW disabled (causes flicker on slow Amigas)
    CC = vc
    CFLAGS = +aos68k -c99 -cpu=68020 -g -I$(INC_DIR) -Isrc -DPLATFORM_AMIGA=1 -D__AMIGA__ -DDEBUG
    LDFLAGS = +aos68k -cpu=68020 -g -hunkdebug -lamiga -lauto -lmieee
    OUT_DIR = $(BUILD_DIR)/amiga
    BIN_DIR = Bin/Amiga
    BIN = $(BIN_DIR)/$(PROJECT)
endif

################################################################################
# Source Files
################################################################################

# Core source files
# GUI MIGRATION NOTE: Changed from main.c to main_gui.c for GUI version
CORE_SRCS = \
	$(SRC_DIR)/main_gui.c \
	$(SRC_DIR)/icon_types.c \
	$(SRC_DIR)/icon_misc.c \
	$(SRC_DIR)/icon_management.c \
	$(SRC_DIR)/file_directory_handling.c \
	$(SRC_DIR)/window_management.c \
	$(SRC_DIR)/utilities.c \
	$(SRC_DIR)/spinner.c \
	$(SRC_DIR)/writeLog.c \
	$(SRC_DIR)/cli_utilities.c \
	$(SRC_DIR)/layout_preferences.c \
	$(SRC_DIR)/layout_processor.c \
	$(SRC_DIR)/aspect_ratio_layout.c

# Backup system source files
BACKUP_SRCS = \
	$(SRC_DIR)/backup_catalog.c \
	$(SRC_DIR)/backup_lha.c \
	$(SRC_DIR)/backup_marker.c \
	$(SRC_DIR)/backup_paths.c \
	$(SRC_DIR)/backup_runs.c \
	$(SRC_DIR)/backup_session.c \
	$(SRC_DIR)/backup_restore.c

# GUI source files
GUI_SRCS = \
	$(SRC_DIR)/GUI/main_window.c \
	$(SRC_DIR)/GUI/advanced_window.c \
	$(SRC_DIR)/GUI/restore_window.c \
	$(SRC_DIR)/GUI/folder_view_window.c \
	$(SRC_DIR)/GUI/easy_request_helper.c \
	$(SRC_DIR)/GUI/StatusWindows/progress_common.c \
	$(SRC_DIR)/GUI/StatusWindows/progress_window.c \
	$(SRC_DIR)/GUI/StatusWindows/recursive_progress.c \
	$(SRC_DIR)/GUI/test_simple_window.c

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

# Memory tracking implementation (conditional - only included if DEBUG_MEMORY_TRACKING is defined)
# To enable: Uncomment #define DEBUG_MEMORY_TRACKING in include/platform/platform.h
MEMORY_TRACKING_SRCS = $(INC_DIR)/platform/platform.c

# All sources
SRCS = $(CORE_SRCS) $(BACKUP_SRCS) $(GUI_SRCS) $(DOS_SRCS) $(SETTINGS_SRCS) $(PLATFORM_SRCS) $(MEMORY_TRACKING_SRCS)

# Object files (in build directory)
# Note: platform.c is in include/platform, needs special handling
CORE_OBJS = $(CORE_SRCS:$(SRC_DIR)/%.c=$(OUT_DIR)/%.o)
BACKUP_OBJS = $(BACKUP_SRCS:$(SRC_DIR)/%.c=$(OUT_DIR)/%.o)
GUI_OBJS = $(GUI_SRCS:$(SRC_DIR)/%.c=$(OUT_DIR)/%.o)
DOS_OBJS = $(DOS_SRCS:$(SRC_DIR)/%.c=$(OUT_DIR)/%.o)
SETTINGS_OBJS = $(SETTINGS_SRCS:$(SRC_DIR)/%.c=$(OUT_DIR)/%.o)
PLATFORM_OBJS = $(PLATFORM_SRCS:$(SRC_DIR)/%.c=$(OUT_DIR)/%.o)
MEMORY_TRACKING_OBJS = $(OUT_DIR)/platform_memory.o

OBJS = $(CORE_OBJS) $(BACKUP_OBJS) $(GUI_OBJS) $(DOS_OBJS) $(SETTINGS_OBJS) $(PLATFORM_OBJS) $(MEMORY_TRACKING_OBJS)

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
# GUI MIGRATION NOTE: Added GUI subdirectory for GUI window source files
directories:
	@if not exist "$(OUT_DIR)" mkdir "$(OUT_DIR)"
	@if not exist "$(OUT_DIR)\DOS" mkdir "$(OUT_DIR)\DOS"
	@if not exist "$(OUT_DIR)\Settings" mkdir "$(OUT_DIR)\Settings"
	@if not exist "$(OUT_DIR)\platform" mkdir "$(OUT_DIR)\platform"
	@if not exist "$(OUT_DIR)\GUI" mkdir "$(OUT_DIR)\GUI"
	@if not exist "$(OUT_DIR)\GUI\StatusWindows" mkdir "$(OUT_DIR)\GUI\StatusWindows"
ifeq ($(TARGET),amiga)
	@if not exist "$(BIN_DIR)" mkdir "$(BIN_DIR)"
endif

# Link executable
# VBCC MIGRATION NOTE: For Amiga builds, copy to Bin/Amiga/ after linking
# GUI MIGRATION NOTE: Executable now named iTidy (GUI version)
# DEBUG BUILD: Executable contains embedded debug symbols via -hunkdebug
$(BIN): $(OBJS)
	@echo Linking $(TARGET) executable: $(BIN)
	$(CC) $(LDFLAGS) -o $@ $^ 
ifeq ($(TARGET),amiga)
	@echo Build complete: $(BIN)
	@echo Debug symbols embedded in executable (use WinUAE debugger or MonAm)
endif

# Compile core source files
$(OUT_DIR)/%.o: $(SRC_DIR)/%.c
	@echo Compiling [$@] from $<
	$(CC) $(CFLAGS) -c $< -o $@

# Compile memory tracking (from include directory)
$(OUT_DIR)/platform_memory.o: $(INC_DIR)/platform/platform.c
	@echo Compiling memory tracking: $@
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
# GUI MIGRATION NOTE: Changed from main.o to main_gui.o
$(OUT_DIR)/main_gui.o: $(SRC_DIR)/main_gui.c $(SRC_DIR)/GUI/main_window.h
$(OUT_DIR)/icon_types.o: $(SRC_DIR)/icon_types.c $(SRC_DIR)/icon_types.h
$(OUT_DIR)/icon_misc.o: $(SRC_DIR)/icon_misc.c $(SRC_DIR)/icon_misc.h
$(OUT_DIR)/icon_management.o: $(SRC_DIR)/icon_management.c $(SRC_DIR)/icon_management.h
$(OUT_DIR)/file_directory_handling.o: $(SRC_DIR)/file_directory_handling.c $(SRC_DIR)/file_directory_handling.h
$(OUT_DIR)/window_management.o: $(SRC_DIR)/window_management.c $(SRC_DIR)/window_management.h
$(OUT_DIR)/utilities.o: $(SRC_DIR)/utilities.c $(SRC_DIR)/utilities.h
$(OUT_DIR)/spinner.o: $(SRC_DIR)/spinner.c $(SRC_DIR)/spinner.h
$(OUT_DIR)/writeLog.o: $(SRC_DIR)/writeLog.c $(SRC_DIR)/writeLog.h
$(OUT_DIR)/cli_utilities.o: $(SRC_DIR)/cli_utilities.c $(SRC_DIR)/cli_utilities.h
$(OUT_DIR)/layout_preferences.o: $(SRC_DIR)/layout_preferences.c $(SRC_DIR)/layout_preferences.h
$(OUT_DIR)/layout_processor.o: $(SRC_DIR)/layout_processor.c $(SRC_DIR)/layout_processor.h
$(OUT_DIR)/aspect_ratio_layout.o: $(SRC_DIR)/aspect_ratio_layout.c $(SRC_DIR)/aspect_ratio_layout.h

# DOS subdirectory
$(OUT_DIR)/DOS/getDiskDetails.o: $(SRC_DIR)/DOS/getDiskDetails.c $(SRC_DIR)/DOS/getDiskDetails.h

# Settings subdirectory
$(OUT_DIR)/Settings/IControlPrefs.o: $(SRC_DIR)/Settings/IControlPrefs.c $(SRC_DIR)/Settings/IControlPrefs.h
$(OUT_DIR)/Settings/WorkbenchPrefs.o: $(SRC_DIR)/Settings/WorkbenchPrefs.c $(SRC_DIR)/Settings/WorkbenchPrefs.h
$(OUT_DIR)/Settings/get_fonts.o: $(SRC_DIR)/Settings/get_fonts.c $(SRC_DIR)/Settings/get_fonts.h

# GUI subdirectory
# GUI MIGRATION NOTE: Added dependency for GUI window module
$(OUT_DIR)/GUI/main_window.o: $(SRC_DIR)/GUI/main_window.c $(SRC_DIR)/GUI/main_window.h
$(OUT_DIR)/GUI/advanced_window.o: $(SRC_DIR)/GUI/advanced_window.c $(SRC_DIR)/GUI/advanced_window.h
$(OUT_DIR)/GUI/restore_window.o: $(SRC_DIR)/GUI/restore_window.c $(SRC_DIR)/GUI/restore_window.h
$(OUT_DIR)/GUI/folder_view_window.o: $(SRC_DIR)/GUI/folder_view_window.c $(SRC_DIR)/GUI/folder_view_window.h
$(OUT_DIR)/GUI/easy_request_helper.o: $(SRC_DIR)/GUI/easy_request_helper.c $(INC_DIR)/easy_request_helper.h

# Platform-specific
$(OUT_DIR)/platform/host_platform.o: $(SRC_DIR)/platform/host_platform.c $(INC_DIR)/platform/platform.h
$(OUT_DIR)/platform/amiga_platform.o: $(SRC_DIR)/platform/amiga_platform.c $(INC_DIR)/platform/platform.h

# All objects depend on platform headers
$(OBJS): $(INC_DIR)/platform/platform.h $(INC_DIR)/platform/platform_types.h

# End of Makefile
