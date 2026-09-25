#include "kv_store.hpp"

#include <mutex>
#include <shared_mutex>

bool KvStore::put(const Record& rec) {
    std::unique_lock lock(mutex_);
    std::string key = rec.keyAsString();
    auto [it, inserted] = map_.insert_or_assign(key, rec);
    (void)it;
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

std::vector<std::string> KvStore::sampleKeys(std::size_t n) const {
    std::shared_lock lock(mutex_);
    std::vector<std::string> out;
    out.reserve(n == 0 ? map_.size() : n);
    for (const auto& [k, v] : map_) {
        if (n != 0 && out.size() >= n) break;
        out.push_back(k);
    }
    return out;
}
