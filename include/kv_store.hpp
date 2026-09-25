#pragma once

#include <cstdint>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include "record.hpp"

struct StoreStats {
    uint64_t recordsInserted = 0;
    uint64_t recordsOverwritten = 0;
    uint64_t recordsReceivedFromNetwork = 0;
};

// Thread-safe, in-memory key -> Record map owned by exactly one node.
//
// Duplicate keys keep the last value written (unordered_map::insert_or_assign)
// and are surfaced through the inserted/overwritten counters rather than being
// rejected, which is how the loader demonstrates its duplicate handling.
class KvStore {
public:
    // Stores `rec`. Returns true when the key was new, false when an existing
    // value was overwritten.
    bool put(const Record& rec);

    // Records that one accepted record arrived over the mock network rather
    // than from this node's own data file.
    void noteReceivedFromNetwork();

    // Returns the stored record for `key`, or std::nullopt when absent.
    [[nodiscard]] std::optional<Record> get(const std::string& key) const;

    [[nodiscard]] bool contains(const std::string& key) const;
    [[nodiscard]] std::size_t size() const;
    [[nodiscard]] StoreStats stats() const;

    // Visits every stored key under a shared lock, in unspecified order.
    // `visit` returns false to stop early: that lets callers bound a sample
    // verification at N keys without first materialising the whole key set.
    template <typename Visit>
    void forEachKey(Visit&& visit) const {
        std::shared_lock lock(mutex_);
        for (const auto& entry : map_) {
            if (!visit(entry.first)) return;
        }
    }

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, Record> map_;
    StoreStats stats_;
};
