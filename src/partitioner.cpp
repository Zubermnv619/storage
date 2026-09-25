#include "partitioner.hpp"

uint64_t fnv1a64(const std::string& data) {
    uint64_t hash = 14695981039346656037ULL;
    constexpr uint64_t prime = 1099511628211ULL;
    for (unsigned char c : data) {
        hash ^= c;
        hash *= prime;
    }
    return hash;
}

Partitioner::Partitioner(int numNodes) : numNodes_(numNodes) {}

int Partitioner::ownerOf(const std::string& key) const {
    return static_cast<int>(fnv1a64(key) % static_cast<uint64_t>(numNodes_));
}
