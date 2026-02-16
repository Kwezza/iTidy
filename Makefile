################################################################################
# iTidy - Amiga Build Makefile (vbcc +aos68k only)
################################################################################

# Console output: Set CONSOLE=1 to enable printf output (opens console window)
# Default: DISABLED for release builds
CONSOLE ?= 0

# Project name
PROJECT = iTidy

# Directories
SRC_DIR = src
BUILD_DIR = build

# Console output flag
ifeq ($(CONSOLE),1)
    CONSOLE_FLAG = -DENABLE_CONSOLE
else
    CONSOLE_FLAG =
endif

# Amiga build configuration (vbcc cross-compiler)
# VBCC MIGRATION NOTE: Changed to use VBCC v0.9x with Workbench 3.2 SDK
# Output goes to Bin/Amiga/ for clean separation
# RELEASE BUILD: Optimized for size (-O2 -size), no debug symbols, no math library (uses fixed-point)
# Note: VBCC warnings from system headers (51: bitfield, 61: array size) cannot be suppressed
# EASY_REQUEST_HELPER: -DBUILD_WITH_MOVEWINDOW disabled (causes flicker on slow Amigas)
# CPU TARGET: 68000 for maximum compatibility (A500/A600/A1200)
# CONSOLE: Add -DENABLE_CONSOLE via CONSOLE=1 to open console window for debugging
CC = vc
CFLAGS = +aos68k -c99 -cpu=68000 -O2 -size -Isrc -DPLATFORM_AMIGA=1 -D__AMIGA__ -DDEBUG $(CONSOLE_FLAG)
LDFLAGS = +aos68k -cpu=68000 -O2 -size -final -lamiga -lauto
OUT_DIR = $(BUILD_DIR)/amiga
BIN_DIR = Bin/Amiga
BIN = $(BIN_DIR)/$(PROJECT)

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
	$(SRC_DIR)/path_utilities.c \
	$(SRC_DIR)/spinner.c \
	$(SRC_DIR)/writeLog.c \
	$(SRC_DIR)/cli_utilities.c \
	$(SRC_DIR)/layout_preferences.c \
	$(SRC_DIR)/layout_processor.c \
	$(SRC_DIR)/aspect_ratio_layout.c \
	$(SRC_DIR)/folder_scanner.c \
	$(SRC_DIR)/string_functions.c

# DefIcons source files
DEFICONS_SRCS = \
	$(SRC_DIR)/deficons/deficons_parser.c \
	$(SRC_DIR)/deficons/deficons_identify.c \
	$(SRC_DIR)/deficons/deficons_templates.c \
	$(SRC_DIR)/deficons/deficons_filters.c \
	$(SRC_DIR)/deficons/deficons_creation.c

# Icon editing / content-aware preview source files
ICON_EDIT_SRCS = \
	$(SRC_DIR)/icon_edit/icon_image_access.c \
	$(SRC_DIR)/icon_edit/icon_text_render.c \
	$(SRC_DIR)/icon_edit/icon_iff_render.c \
	$(SRC_DIR)/icon_edit/icon_content_preview.c \
	$(SRC_DIR)/icon_edit/palette/palette_mapping.c \
	$(SRC_DIR)/icon_edit/palette/palette_quantization.c \
	$(SRC_DIR)/icon_edit/palette/palette_dithering.c \
	$(SRC_DIR)/icon_edit/palette/palette_reduction.c \
	$(SRC_DIR)/icon_edit/palette/palette_grayscale.c \
	$(SRC_DIR)/icon_edit/palette/ultra_downsample.c

# Backup system source files
BACKUP_SRCS = \
	$(SRC_DIR)/backups/backup_catalog.c \
	$(SRC_DIR)/backups/backup_lha.c \
	$(SRC_DIR)/backups/backup_marker.c \
	$(SRC_DIR)/backups/backup_paths.c \
	$(SRC_DIR)/backups/backup_runs.c \
	$(SRC_DIR)/backups/backup_session.c \
	$(SRC_DIR)/backups/backup_restore.c

# Helper utilities source files
HELPERS_SRCS = \
	$(SRC_DIR)/helpers/listview_simple_columns.c \
	$(SRC_DIR)/helpers/list_formatter.c

# GUI source files
GUI_SRCS = \
	$(SRC_DIR)/GUI/main_window.c \
	$(SRC_DIR)/GUI/advanced_window.c \
	$(SRC_DIR)/GUI/beta_options_window.c \
	$(SRC_DIR)/GUI/deficons_settings_window.c \
	$(SRC_DIR)/GUI/exclude_paths_window.c \
	$(SRC_DIR)/GUI/easy_request_helper.c \
	$(SRC_DIR)/GUI/window_enumerator.c \
	$(SRC_DIR)/GUI/wb_classify.c \
	$(SRC_DIR)/GUI/gui_groupbox.c \
	$(SRC_DIR)/GUI/gui_utilities.c \
	$(SRC_DIR)/GUI/StatusWindows/progress_common.c \
	$(SRC_DIR)/GUI/StatusWindows/progress_window.c \
	$(SRC_DIR)/GUI/StatusWindows/recursive_progress.c \
	$(SRC_DIR)/GUI/StatusWindows/main_progress_window.c

# Default Tools subsystem source files
DEFAULT_TOOLS_SRCS = \
	$(SRC_DIR)/GUI/DefaultTools/tool_cache_window.c \
	$(SRC_DIR)/GUI/DefaultTools/tool_cache_reports.c \
	$(SRC_DIR)/GUI/DefaultTools/default_tool_update_window_reaction.c \
	$(SRC_DIR)/GUI/DefaultTools/default_tool_backup.c \
	$(SRC_DIR)/GUI/DefaultTools/default_tool_restore_window.c

# Restore/Backup window subsystem source files
RESTORE_BACKUP_SRCS = \
	$(SRC_DIR)/GUI/RestoreBackups/restore_window.c \
	$(SRC_DIR)/GUI/RestoreBackups/folder_view_window.c

# DOS subdirectory sources
DOS_SRCS = \
	$(SRC_DIR)/DOS/getDiskDetails.c

# Settings subdirectory sources
SETTINGS_SRCS = \
	$(SRC_DIR)/Settings/IControlPrefs.c \
	$(SRC_DIR)/Settings/WorkbenchPrefs.c \
	$(SRC_DIR)/Settings/get_fonts.c

# Platform-specific sources
PLATFORM_SRCS = $(SRC_DIR)/platform/amiga_platform.c

# Memory tracking implementation (conditional - only included if DEBUG_MEMORY_TRACKING is defined)
# To enable: Uncomment #define DEBUG_MEMORY_TRACKING in src/platform/platform.h
MEMORY_TRACKING_SRCS = $(SRC_DIR)/platform/platform.c

# All sources
SRCS = $(CORE_SRCS) $(ICON_EDIT_SRCS) $(BACKUP_SRCS) $(HELPERS_SRCS) $(GUI_SRCS) $(DEFAULT_TOOLS_SRCS) $(RESTORE_BACKUP_SRCS) $(DOS_SRCS) $(SETTINGS_SRCS) $(PLATFORM_SRCS) $(MEMORY_TRACKING_SRCS)

# Object files (in build directory)
# Note: platform.c is in include/platform, needs special handling
CORE_OBJS = $(CORE_SRCS:$(SRC_DIR)/%.c=$(OUT_DIR)/%.o)
DEFICONS_OBJS = $(DEFICONS_SRCS:$(SRC_DIR)/%.c=$(OUT_DIR)/%.o)
ICON_EDIT_OBJS = $(ICON_EDIT_SRCS:$(SRC_DIR)/%.c=$(OUT_DIR)/%.o)
BACKUP_OBJS = $(BACKUP_SRCS:$(SRC_DIR)/%.c=$(OUT_DIR)/%.o)
HELPERS_OBJS = $(HELPERS_SRCS:$(SRC_DIR)/%.c=$(OUT_DIR)/%.o)
GUI_OBJS = $(GUI_SRCS:$(SRC_DIR)/%.c=$(OUT_DIR)/%.o)
DEFAULT_TOOLS_OBJS = $(DEFAULT_TOOLS_SRCS:$(SRC_DIR)/%.c=$(OUT_DIR)/%.o)
RESTORE_BACKUP_OBJS = $(RESTORE_BACKUP_SRCS:$(SRC_DIR)/%.c=$(OUT_DIR)/%.o)
DOS_OBJS = $(DOS_SRCS:$(SRC_DIR)/%.c=$(OUT_DIR)/%.o)
SETTINGS_OBJS = $(SETTINGS_SRCS:$(SRC_DIR)/%.c=$(OUT_DIR)/%.o)
PLATFORM_OBJS = $(PLATFORM_SRCS:$(SRC_DIR)/%.c=$(OUT_DIR)/%.o)
MEMORY_TRACKING_OBJS = $(OUT_DIR)/platform_memory.o

OBJS = $(CORE_OBJS) $(DEFICONS_OBJS) $(ICON_EDIT_OBJS) $(BACKUP_OBJS) $(HELPERS_OBJS) $(GUI_OBJS) $(DEFAULT_TOOLS_OBJS) $(RESTORE_BACKUP_OBJS) $(DOS_OBJS) $(SETTINGS_OBJS) $(PLATFORM_OBJS) $(MEMORY_TRACKING_OBJS)

################################################################################
# Build Rules
################################################################################

.PHONY: all clean help amiga directories

# Default target
all: directories $(BIN)

# Amiga build shortcut
amiga: all

# Create output directories
# VBCC MIGRATION NOTE: Added Bin/Amiga directory for final Amiga executables
# GUI MIGRATION NOTE: Added GUI subdirectory for GUI window source files
directories:
	@if not exist "$(OUT_DIR)" mkdir "$(OUT_DIR)"
	@if not exist "$(OUT_DIR)\deficons" mkdir "$(OUT_DIR)\deficons"
	@if not exist "$(OUT_DIR)\backups" mkdir "$(OUT_DIR)\backups"
	@if not exist "$(OUT_DIR)\DOS" mkdir "$(OUT_DIR)\DOS"
	@if not exist "$(OUT_DIR)\Settings" mkdir "$(OUT_DIR)\Settings"
	@if not exist "$(OUT_DIR)\platform" mkdir "$(OUT_DIR)\platform"
	@if not exist "$(OUT_DIR)\helpers" mkdir "$(OUT_DIR)\helpers"
	@if not exist "$(OUT_DIR)\GUI" mkdir "$(OUT_DIR)\GUI"
	@if not exist "$(OUT_DIR)\icon_edit" mkdir "$(OUT_DIR)\icon_edit"
	@if not exist "$(OUT_DIR)\icon_edit\palette" mkdir "$(OUT_DIR)\icon_edit\palette"
	@if not exist "$(OUT_DIR)\GUI\StatusWindows" mkdir "$(OUT_DIR)\GUI\StatusWindows"
	@if not exist "$(OUT_DIR)\GUI\DefaultTools" mkdir "$(OUT_DIR)\GUI\DefaultTools"
	@if not exist "$(OUT_DIR)\GUI\RestoreBackups" mkdir "$(OUT_DIR)\GUI\RestoreBackups"
	@if not exist "$(BIN_DIR)" mkdir "$(BIN_DIR)"

# Link executable
# VBCC MIGRATION NOTE: For Amiga builds, copy to Bin/Amiga/ after linking
# GUI MIGRATION NOTE: Executable now named iTidy (GUI version)
# RELEASE BUILD: Optimized executable with dead code elimination (-final)
$(BIN): $(OBJS)
	@echo Linking Amiga executable: $(BIN)
	$(CC) $(LDFLAGS) -o $@ $^ 
	@echo Build complete: $(BIN)
	@echo Release build optimized for size (-O2 -size -final)

# Compile core source files
$(OUT_DIR)/%.o: $(SRC_DIR)/%.c
	@echo Compiling [$@] from $<
	$(CC) $(CFLAGS) -c $< -o $@

# Compile memory tracking (from include directory)
$(OUT_DIR)/platform_memory.o: $(SRC_DIR)/platform/platform.c
	@echo Compiling memory tracking: $@
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts

clean:
	@echo Cleaning Amiga build...
	@if exist "$(OUT_DIR)" rmdir /S /Q "$(OUT_DIR)"
	@echo Clean complete.

# Clean all targets
clean-all:
	@echo Cleaning all builds...
	@if exist "$(BUILD_DIR)" rmdir /S /Q "$(BUILD_DIR)"
	@echo Clean complete.

help:
	@echo iTidy Amiga Build System
	@echo ========================
	@echo.
	@echo Targets:
	@echo   make                    - Build for Amiga (default, no console)
	@echo   make CONSOLE=1          - Build with console output enabled
	@echo   make amiga              - Shortcut alias for default build
	@echo   make clean              - Clean Amiga build artifacts
	@echo   make clean-all          - Clean all builds
	@echo   make help               - Show this help
	@echo.
	@echo Console Output:
	@echo   CONSOLE=0 (default)     - Release build, no console window opens
	@echo   CONSOLE=1               - Debug build, console window with output
	@echo.
	@echo Build Outputs:
	@echo   Amiga:  $(BUILD_DIR)/amiga/$(PROJECT)
	@echo.
	@echo Current Configuration:
	@echo   CONSOLE: $(CONSOLE)
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
$(OUT_DIR)/path_utilities.o: $(SRC_DIR)/path_utilities.c $(SRC_DIR)/path_utilities.h
$(OUT_DIR)/spinner.o: $(SRC_DIR)/spinner.c $(SRC_DIR)/spinner.h
$(OUT_DIR)/writeLog.o: $(SRC_DIR)/writeLog.c $(SRC_DIR)/writeLog.h
$(OUT_DIR)/cli_utilities.o: $(SRC_DIR)/cli_utilities.c $(SRC_DIR)/cli_utilities.h
$(OUT_DIR)/layout_preferences.o: $(SRC_DIR)/layout_preferences.c $(SRC_DIR)/layout_preferences.h
$(OUT_DIR)/layout_processor.o: $(SRC_DIR)/layout_processor.c $(SRC_DIR)/layout_processor.h
$(OUT_DIR)/aspect_ratio_layout.o: $(SRC_DIR)/aspect_ratio_layout.c $(SRC_DIR)/aspect_ratio_layout.h
$(OUT_DIR)/string_functions.o: $(SRC_DIR)/string_functions.c $(SRC_DIR)/string_functions.h

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
$(OUT_DIR)/GUI/easy_request_helper.o: $(SRC_DIR)/GUI/easy_request_helper.c $(SRC_DIR)/GUI/easy_request_helper.h
$(OUT_DIR)/GUI/window_enumerator.o: $(SRC_DIR)/GUI/window_enumerator.c $(SRC_DIR)/GUI/window_enumerator.h
$(OUT_DIR)/GUI/wb_classify.o: $(SRC_DIR)/GUI/wb_classify.c $(SRC_DIR)/GUI/wb_classify.h

# Platform-specific
$(OUT_DIR)/platform/host_platform.o: $(SRC_DIR)/platform/host_platform.c $(SRC_DIR)/platform/platform.h
$(OUT_DIR)/platform/amiga_platform.o: $(SRC_DIR)/platform/amiga_platform.c $(SRC_DIR)/platform/platform.h

# Icon editing module
$(OUT_DIR)/icon_edit/icon_image_access.o: $(SRC_DIR)/icon_edit/icon_image_access.c $(SRC_DIR)/icon_edit/icon_image_access.h
$(OUT_DIR)/icon_edit/icon_text_render.o: $(SRC_DIR)/icon_edit/icon_text_render.c $(SRC_DIR)/icon_edit/icon_text_render.h $(SRC_DIR)/icon_edit/icon_image_access.h
$(OUT_DIR)/icon_edit/icon_iff_render.o: $(SRC_DIR)/icon_edit/icon_iff_render.c $(SRC_DIR)/icon_edit/icon_iff_render.h $(SRC_DIR)/icon_edit/icon_image_access.h
$(OUT_DIR)/icon_edit/icon_content_preview.o: $(SRC_DIR)/icon_edit/icon_content_preview.c $(SRC_DIR)/icon_edit/icon_content_preview.h $(SRC_DIR)/icon_edit/icon_image_access.h $(SRC_DIR)/icon_edit/icon_text_render.h

# Palette reduction module
$(OUT_DIR)/icon_edit/palette/palette_mapping.o: $(SRC_DIR)/icon_edit/palette/palette_mapping.c $(SRC_DIR)/icon_edit/palette/palette_mapping.h $(SRC_DIR)/icon_edit/palette/palette_dithering.h
$(OUT_DIR)/icon_edit/palette/palette_quantization.o: $(SRC_DIR)/icon_edit/palette/palette_quantization.c $(SRC_DIR)/icon_edit/palette/palette_quantization.h
$(OUT_DIR)/icon_edit/palette/palette_dithering.o: $(SRC_DIR)/icon_edit/palette/palette_dithering.c $(SRC_DIR)/icon_edit/palette/palette_dithering.h $(SRC_DIR)/icon_edit/palette/palette_mapping.h
$(OUT_DIR)/icon_edit/palette/palette_reduction.o: $(SRC_DIR)/icon_edit/palette/palette_reduction.c $(SRC_DIR)/icon_edit/palette/palette_reduction.h $(SRC_DIR)/icon_edit/palette/palette_mapping.h $(SRC_DIR)/icon_edit/palette/palette_quantization.h $(SRC_DIR)/icon_edit/palette/palette_dithering.h $(SRC_DIR)/icon_edit/palette/palette_grayscale.h
$(OUT_DIR)/icon_edit/palette/palette_grayscale.o: $(SRC_DIR)/icon_edit/palette/palette_grayscale.c $(SRC_DIR)/icon_edit/palette/palette_grayscale.h $(SRC_DIR)/icon_edit/palette/palette_mapping.h $(SRC_DIR)/icon_edit/palette/palette_dithering.h
$(OUT_DIR)/icon_edit/palette/ultra_downsample.o: $(SRC_DIR)/icon_edit/palette/ultra_downsample.c $(SRC_DIR)/icon_edit/palette/ultra_downsample.h $(SRC_DIR)/icon_edit/palette/palette_mapping.h $(SRC_DIR)/icon_edit/palette/palette_quantization.h

# All objects depend on platform headers
$(OBJS): $(SRC_DIR)/platform/platform.h $(SRC_DIR)/platform/platform_types.h

# End of Makefile
