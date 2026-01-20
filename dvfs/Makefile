# Compiler settings
CC      = gcc
CFLAGS  = -Wall -Wextra -O2
LDFLAGS = -llikwid

# Likwid Paths (from Lmod/spack). Uses CMAKE_PREFIX_PATH set by the module.
LIKWID_PREFIX ?= $(firstword $(subst :, ,$(CMAKE_PREFIX_PATH)))
LIKWID_INC = -I$(LIKWID_PREFIX)/include
LIKWID_LIB = -L$(LIKWID_PREFIX)/lib

# Add paths to flags
CFLAGS  += $(LIKWID_INC)
LDFLAGS += $(LIKWID_LIB)

# Project definitions
TARGET  = dvfs_controller
SRCS    = main.c config_loader.c dvfs_policy.c
OBJS    = $(SRCS:.c=.o)
HEADERS = dvfs_config.h dvfs_policy.h model.h config_loader.h

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	$(CC) -o $@ $^ $(LDFLAGS)

# Compile source files into object files
# This rule ensures that if a header changes, files are recompiled
%.o: %.c $(HEADERS)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	@echo "Cleaning up..."
	rm -f $(OBJS) $(TARGET)

# Helper to run the daemon (requires sudo for hardware counters/cpupower)
run: $(TARGET)
	@echo "Starting DVFS Controller (requires sudo)..."
	sudo ./$(TARGET)

.PHONY: all clean run
