CXX      ?= g++
CXXSTD   ?= -std=c++17
CXXFLAGS ?= $(CXXSTD) -O2 -Wall -Wextra -pthread -Iinclude

BIN_DIR  := bin
LOADER   := $(BIN_DIR)/kv_loader
GEN      := $(BIN_DIR)/generate_data

APP_SRCS := src/main.cpp src/types.cpp src/cluster_config.cpp src/schema.cpp src/record.cpp src/mock_socket.cpp src/partitioner.cpp src/kv_store.cpp src/node.cpp
GEN_SRCS := src/generate_data.cpp src/types.cpp src/cluster_config.cpp src/schema.cpp

.PHONY: all clean run gen

all: $(LOADER) $(GEN)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

HEADERS := $(wildcard include/*.hpp)

$(LOADER): $(APP_SRCS) $(HEADERS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(APP_SRCS) -o $(LOADER)

$(GEN): $(GEN_SRCS) $(HEADERS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(GEN_SRCS) -o $(GEN)

# Convenience targets, mirrored by scripts/*.sh for anyone who prefers
# not to invoke make directly.
gen: $(GEN)
	./$(GEN) config/cluster.conf 104857600

run: $(LOADER)
	./$(LOADER) config/cluster.conf

clean:
	rm -rf $(BIN_DIR) data/*.dat
