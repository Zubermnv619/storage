#pragma once

#include <cstdint>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "record.hpp"

struct StoreStats {
    uint64_t recordsInserted = 0;
    uint64_t recordsOverwritten = 0;
    uint64_t recordsReceivedFromNetwork = 0;
};

class KvStore {
public:
    bool put(const Record& rec);
    void noteReceivedFromNetwork();
    [[nodiscard]] bool contains(const std::string& key) const;
    [[nodiscard]] std::size_t size() const;
    [[nodiscard]] StoreStats stats() const;
    [[nodiscard]] std::vector<std::string> sampleKeys(std::size_t n = 0) const;

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, Record> map_;
    StoreStats stats_;
};
