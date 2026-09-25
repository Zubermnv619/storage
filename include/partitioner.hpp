#pragma once

#include <cstdint>
#include <string>

// FNV-1a 64-bit hash. Deterministic and dependency-free so that every node
// derives the same owner for the same key.
[[nodiscard]] uint64_t fnv1a64(const std::string& data);

// Maps a record key to its owner node via hash(key) % numNodes.
//
// NOTE: plain modulo, not consistent hashing. Changing nodeCount reassigns
// almost every key, which the README's Scalability section discusses.
class Partitioner {
public:
    explicit Partitioner(int numNodes);

    // Returns an owner id in [0, numNodes). Throws std::invalid_argument when
    // numNodes < 1.
    [[nodiscard]] int ownerOf(const std::string& key) const;

    [[nodiscard]] int nodeCount() const;

private:
    int numNodes_;
};
