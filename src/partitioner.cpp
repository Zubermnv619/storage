#include "partitioner.hpp"

#include <stdexcept>

uint64_t fnv1a64(const std::string& data) {
    uint64_t hash = 14695981039346656037ULL;  // FNV offset basis
    constexpr uint64_t kPrime = 1099511628211ULL;
    for (const unsigned char c : data) {
        hash ^= c;
        hash *= kPrime;
    }
    return hash;
}

Partitioner::Partitioner(int numNodes) : numNodes_(numNodes) {
    if (numNodes_ < 1) {
        throw std::invalid_argument("Partitioner requires at least one node");
    }
}

int Partitioner::ownerOf(const std::string& key) const {
    return static_cast<int>(fnv1a64(key) % static_cast<uint64_t>(numNodes_));
}

int Partitioner::nodeCount() const {
    return numNodes_;
}
