#include "kv_store.hpp"

#include <mutex>
#include <optional>
#include <shared_mutex>
#include <utility>

bool KvStore::put(const Record& rec) {
    std::unique_lock lock(mutex_);
    std::string key = rec.keyAsString();
    const bool inserted = map_.insert_or_assign(std::move(key), rec).second;
    if (inserted) {
        ++stats_.recordsInserted;
    } else {
        ++stats_.recordsOverwritten;
    }
    return inserted;
}

void KvStore::noteReceivedFromNetwork() {
    std::unique_lock lock(mutex_);
    ++stats_.recordsReceivedFromNetwork;
}

std::optional<Record> KvStore::get(const std::string& key) const {
    std::shared_lock lock(mutex_);
    const auto it = map_.find(key);
    if (it == map_.end()) return std::nullopt;
    return it->second;
}

bool KvStore::contains(const std::string& key) const {
    std::shared_lock lock(mutex_);
    return map_.find(key) != map_.end();
}

std::size_t KvStore::size() const {
    std::shared_lock lock(mutex_);
    return map_.size();
}

StoreStats KvStore::stats() const {
    std::shared_lock lock(mutex_);
    return stats_;
}
