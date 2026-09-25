# Distributed in-memory KV loader + data generator.
#
# Targets:
#   make all     build bin/kv_loader and bin/generate_data
#   make test    build and run the unit/integration tests, then the smoke test
#   make smoke   just the end-to-end shell smoke test
#   make run     build, then load config/cluster.conf
#   make gen     build, then generate a default dataset
#   make clean   remove binaries and generated data
#   make help    list targets

CXX      ?= g++
CXXSTD   ?= -std=c++17
WARNINGS ?= -Wall -Wextra
OPT      ?= -O2
CXXFLAGS ?= $(CXXSTD) $(OPT) $(WARNINGS) -pthread -Iinclude
LDLIBS   ?= -pthread

BIN_DIR  := bin

LOADER   := $(BIN_DIR)/kv_loader
GEN      := $(BIN_DIR)/generate_data
TEST_BIN := $(BIN_DIR)/run_tests

# Everything except main()/generator entry points, shared by the loader and the
# test binary so both link the exact same routing/storage code.
LIB_SRCS := \
	src/types.cpp \
	src/cluster_config.cpp \
	src/schema.cpp \
	src/record.cpp \
	src/mock_socket.cpp \
	src/partitioner.cpp \
	src/kv_store.cpp \
	src/node.cpp

LOADER_SRCS := src/main.cpp $(LIB_SRCS)
GEN_SRCS    := src/generate_data.cpp src/types.cpp src/cluster_config.cpp src/schema.cpp
TEST_SRCS   := $(wildcard tests/*.cpp) $(LIB_SRCS)

HEADERS := $(wildcard include/*.hpp)

.PHONY: all clean run gen test smoke help

all: $(LOADER) $(GEN)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(LOADER): $(LOADER_SRCS) $(HEADERS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(LOADER_SRCS) -o $@ $(LDLIBS)

$(GEN): $(GEN_SRCS) $(HEADERS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(GEN_SRCS) -o $@ $(LDLIBS)

# -isystem tests keeps doctest.h's own warnings out of our build output.
$(TEST_BIN): $(TEST_SRCS) $(HEADERS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -isystem tests $(TEST_SRCS) -o $@ $(LDLIBS)

test: $(TEST_BIN) $(LOADER) $(GEN)
	./$(TEST_BIN)
	./scripts/test.sh

smoke: $(LOADER) $(GEN)
	./scripts/test.sh

gen: $(GEN)
	./$(GEN) config/cluster.conf 104857600

run: $(LOADER)
	./$(LOADER) config/cluster.conf

clean:
	rm -rf $(BIN_DIR) data/*.dat data/test_cluster

help:
	@echo "targets: all test smoke run gen clean help"
