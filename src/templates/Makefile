################################################################################
# Amiga GadTools Templates - Build Script (vbcc +aos68k)
################################################################################

# Output directory
BINDIR = bin

# Target executables (standalone programs with main())
TARGET1 = amiga_window_resize_template
TARGET2 = amiga_window_template
TARGETS = $(TARGET1) $(TARGET2)

# Compiler and flags
CC      = vc
CFLAGS  = +aos68k -c99 -cpu=68020 -size -D__AMIGA__
LDFLAGS = +aos68k -cpu=68020 -final -s -lamiga -lauto -lmieee

.PHONY: all clean help $(TARGET1) $(TARGET2)

# Default target - build all executables
all: $(TARGETS)

# Individual target rules
$(TARGET1): $(BINDIR)/$(TARGET1)

$(TARGET2): $(BINDIR)/$(TARGET2)

# Generic build rule
$(BINDIR)/%: %.o | $(BINDIR)
	@echo Linking $@
	$(CC) $(LDFLAGS) -o $@ $<

%.o: %.c
	@echo Compiling $<
	$(CC) $(CFLAGS) -c $< -o $@

# Create bin directory if it doesn't exist
$(BINDIR):
	@if not exist $(BINDIR) mkdir $(BINDIR)

clean:
	@echo Cleaning template artifacts
	@if exist *.o del /Q *.o
	@if exist $(BINDIR)\*.* del /Q $(BINDIR)\*.*

help:
	@echo ================================================================================
	@echo Amiga GadTools Templates - Build Targets
	@echo ================================================================================
	@echo.
	@echo Available targets:
	@echo   all                           Build all executables (default)
	@echo   $(TARGET1)     Build resize template example
	@echo   $(TARGET2)          Build window template example
	@echo   clean                         Remove all build artifacts
	@echo   help                          Show this help message
	@echo.
	@echo Executables are built to: $(BINDIR)/
	@echo.
	@echo ================================================================================
