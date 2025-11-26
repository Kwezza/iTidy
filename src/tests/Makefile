################################################################################
# iTidy - ListView Stress Test Makefile
# Builds standalone test binary for Amiga using VBCC
################################################################################

# Project name
PROJECT = listview_stress_test

# Directories
SRC_DIR = ..
HELPERS_DIR = ../helpers
SETTINGS_DIR = ../Settings
INC_DIR = ../../include
BUILD_DIR = ../../build/amiga
BIN_DIR = ../../Bin/Amiga

# Compiler and flags (VBCC for Amiga 68000)
# BUGFIX: Removed -final flag - causes position-dependent code on 68000
# Symptom: Works from DF0: but crashes from RAM: at same point (after sort, before width calc)
# Root cause: -final does whole-program optimization that can generate non-relocatable code
CC = vc
CFLAGS = +aos68k -c99 -cpu=68000 -O2 -speed -I$(INC_DIR) -I$(SRC_DIR) -DPLATFORM_AMIGA=1 -D__AMIGA__  -DDEBUG
LDFLAGS = +aos68k -cpu=68000 -s -lamiga -lauto -lmieee

# Output binary
BIN = $(BIN_DIR)/$(PROJECT)

# Source files needed for the stress test
TEST_SRCS = \
	listview_stress_test.c

# Helper dependencies (ListView API)
HELPER_SRCS = \
	$(HELPERS_DIR)/listview_columns_api.c

# Core dependencies
CORE_SRCS = \
	$(SRC_DIR)/writeLog.c \
	$(SRC_DIR)/path_utilities.c \
	$(SRC_DIR)/string_functions.c \
	$(SRC_DIR)/utilities.c

# Settings dependencies
SETTINGS_SRCS = \
	$(SETTINGS_DIR)/IControlPrefs.c \
	$(SETTINGS_DIR)/WorkbenchPrefs.c \
	$(SETTINGS_DIR)/get_fonts.c

# Platform abstraction
PLATFORM_SRCS = \
	../../include/platform/platform.c

# All sources
SRCS = $(TEST_SRCS) $(HELPER_SRCS) $(CORE_SRCS) $(SETTINGS_SRCS) $(PLATFORM_SRCS)

# Object files
OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(SRCS)))

################################################################################
# Build Rules
################################################################################

.PHONY: all clean

all: $(BIN)
	@echo ""
	@echo "Build complete: $(BIN)"
	@echo "Optimized for speed (-O2 -speed), debug symbols stripped (-s)"
	@echo "BUGFIX: -final removed to fix RAM: vs DF0: crash (position-dependent code issue)"

$(BIN): $(OBJS) | $(BIN_DIR)
	@echo "Linking Amiga executable: $(BIN)"
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

# Compile test source
$(BUILD_DIR)/listview_stress_test.o: listview_stress_test.c | $(BUILD_DIR)
	@echo "Compiling [$@] from $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compile helper sources
$(BUILD_DIR)/listview_columns_api.o: $(HELPERS_DIR)/listview_columns_api.c | $(BUILD_DIR)
	@echo "Compiling [$@] from $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compile core sources
$(BUILD_DIR)/writeLog.o: $(SRC_DIR)/writeLog.c | $(BUILD_DIR)
	@echo "Compiling [$@] from $<"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/path_utilities.o: $(SRC_DIR)/path_utilities.c | $(BUILD_DIR)
	@echo "Compiling [$@] from $<"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/string_functions.o: $(SRC_DIR)/string_functions.c | $(BUILD_DIR)
	@echo "Compiling [$@] from $<"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/utilities.o: $(SRC_DIR)/utilities.c | $(BUILD_DIR)
	@echo "Compiling [$@] from $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compile settings sources
$(BUILD_DIR)/IControlPrefs.o: $(SETTINGS_DIR)/IControlPrefs.c | $(BUILD_DIR)
	@echo "Compiling [$@] from $<"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/WorkbenchPrefs.o: $(SETTINGS_DIR)/WorkbenchPrefs.c | $(BUILD_DIR)
	@echo "Compiling [$@] from $<"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/get_fonts.o: $(SETTINGS_DIR)/get_fonts.c | $(BUILD_DIR)
	@echo "Compiling [$@] from $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compile platform sources
$(BUILD_DIR)/platform.o: ../../include/platform/platform.c | $(BUILD_DIR)
	@echo "Compiling [$@] from $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Create output directories if they don't exist
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

clean:
	@echo "Cleaning stress test build artifacts..."
	@rm -f $(BUILD_DIR)/listview_stress_test.o
	@rm -f $(BUILD_DIR)/listview_columns_api.o
	@rm -f $(BUILD_DIR)/writeLog.o
	@rm -f $(BUILD_DIR)/path_utilities.o
	@rm -f $(BUILD_DIR)/string_functions.o
	@rm -f $(BUILD_DIR)/utilities.o
	@rm -f $(BUILD_DIR)/IControlPrefs.o
	@rm -f $(BUILD_DIR)/WorkbenchPrefs.o
	@rm -f $(BUILD_DIR)/get_fonts.o
	@rm -f $(BUILD_DIR)/platform.o
	@rm -f $(BIN)
	@echo "Clean complete"

################################################################################
# Usage
################################################################################

help:
	@echo ""
	@echo "iTidy ListView Stress Test - Makefile"
	@echo "======================================"
	@echo ""
	@echo "Usage:"
	@echo "  make          Build the stress test binary"
	@echo "  make clean    Remove build artifacts"
	@echo "  make help     Show this help message"
	@echo ""
	@echo "Output:"
	@echo "  Binary: $(BIN)"
	@echo ""
	@echo "Requirements:"
	@echo "  - VBCC cross-compiler (vc +aos68k)"
	@echo "  - Amiga SDK 3.2"
	@echo ""
	@echo "Testing:"
	@echo "  Run in WinUAE or on real Amiga hardware"
	@echo "  Check Bin/Amiga/logs/ for timing output"
	@echo ""
