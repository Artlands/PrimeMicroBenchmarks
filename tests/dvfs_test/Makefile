MPICC ?= mpicc
CFLAGS ?= -O3 -march=native -Wall -Wextra
LDFLAGS ?= -lm

BIN_DIR ?= ../../bin/tests

TARGETS = mpi_pattern_app dvfs_test_app

all: $(TARGETS)

mpi_pattern_app: mpi_pattern_app.c | $(BIN_DIR)
	$(MPICC) $(CFLAGS) -o $(BIN_DIR)/$@ $< $(LDFLAGS)

dvfs_test_app: dvfs_test_app.c | $(BIN_DIR)
	$(MPICC) $(CFLAGS) -o $(BIN_DIR)/$@ $< $(LDFLAGS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -f $(BIN_DIR)/mpi_pattern_app $(BIN_DIR)/dvfs_test_app

.PHONY: all clean
