# Compiler settings
# We use mpicc, which is a wrapper around gcc that automatically includes MPI headers/libs
CC      = mpicc
CFLAGS  = -O2 -Wall -Wextra

# Linker flags for BLAS (Linear Algebra)
# If you don't have OpenBLAS, you can try -lblas or -latlas
# Adjust LIBS if using Intel MKL (e.g., -lmkl_intel_lp64 -lmkl_sequential -lmkl_core)
BLAS_CFLAGS = $(shell pkg-config --cflags openblas)
BLAS_LIBS   = $(shell pkg-config --libs openblas)
LIBS    = -lopenblas -lm

# Project definition
TARGET  = test_app
PATTERN_ENERGY = pattern_energy
PATTERN_EDP = pattern_edp
SRCS    = dvfs_test_app.c
OBJS    = $(SRCS:.c=.o)

# Default target
all: $(TARGET) $(PATTERN_ENERGY) $(PATTERN_EDP)

# Rule to link the executable
$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	$(CC) $(CFLAGS) -o $@ $^ $(BLAS_CFLAGS) $(BLAS_LIBS) $(LIBS)

$(PATTERN_ENERGY): mpi_pattern_app.c
	@echo "Linking $(PATTERN_ENERGY)..."
	$(CC) $(CFLAGS) -DTARGET_DEFAULT=\"energy\" -o $@ $< $(BLAS_CFLAGS) $(BLAS_LIBS) $(LIBS)

$(PATTERN_EDP): mpi_pattern_app.c
	@echo "Linking $(PATTERN_EDP)..."
	$(CC) $(CFLAGS) -DTARGET_DEFAULT=\"edp\" -o $@ $< $(BLAS_CFLAGS) $(BLAS_LIBS) $(LIBS)

# Rule to compile source files
%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@ $(BLAS_CFLAGS) $(BLAS_LIBS) $(LIBS)

# Clean up
clean:
	@echo "Cleaning up..."
	rm -f $(OBJS) $(TARGET) $(PATTERN_ENERGY) $(PATTERN_EDP)

# Helper to run the test (runs on 4 cores by default)
run: $(TARGET)
	@echo "Running test app on 4 cores..."
	srun --ntasks=4 ./$(TARGET)

run-pattern-energy: $(PATTERN_ENERGY)
	@echo "Running pattern_energy on 256 cores..."
	srun --ntasks=256 ./$(PATTERN_ENERGY)

run-pattern-edp: $(PATTERN_EDP)
	@echo "Running pattern_edp on 256 cores..."
	srun --ntasks=256 ./$(PATTERN_EDP)

.PHONY: all clean run run-pattern-energy run-pattern-edp
