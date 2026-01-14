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
SRCS    = dvfs_test_app.c
OBJS    = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Rule to link the executable
$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	$(CC) $(CFLAGS) -o $@ $^ $(BLAS_CFLAGS) $(BLAS_LIBS) $(LIBS)

# Rule to compile source files
%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@ $(BLAS_CFLAGS) $(BLAS_LIBS) $(LIBS)

# Clean up
clean:
	@echo "Cleaning up..."
	rm -f $(OBJS) $(TARGET)

# Helper to run the test (runs on 4 cores by default)
run: $(TARGET)
	@echo "Running test app on 4 cores..."
	mpirun -np 4 --bind-to core ./$(TARGET)

.PHONY: all clean run