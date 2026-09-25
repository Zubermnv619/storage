#pragma once

#include <cstdint>
#include <string>

uint64_t fnv1a64(const std::string& data);

class Partitioner {
public:
    explicit Partitioner(int numNodes);
    int ownerOf(const std::string& key) const;

private:
    int numNodes_;
};
