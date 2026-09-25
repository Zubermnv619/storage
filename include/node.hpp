#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>

#include "kv_store.hpp"
#include "mock_socket.hpp"
#include "partitioner.hpp"
#include "record.hpp"
#include "schema.hpp"

// A node accumulates records destined for the same peer into a single buffer
// and only sends when that buffer reaches this size. Without it, every remote
// record would cost one mutex lock + condition_variable notify on the
// destination inbox (see the README's batching measurements).
inline constexpr std::size_t kDefaultBatchBytes = 64 * 1024;

// One simulated cluster member: owns a KvStore, reads its local data file,
// routes each record to that record's owner, and drains its inbox on a
// background receiver thread.
class Node {
public:
    Node(int id, const Schema& schema, const Partitioner& partitioner, NetworkHub& hub,
         std::size_t batchBytes = kDefaultBatchBytes);

    [[nodiscard]] int id() const;
    [[nodiscard]] KvStore& store();
    [[nodiscard]] const KvStore& store() const;
    [[nodiscard]] std::size_t batchBytes() const;

    // Blocks until the inbox is closed and drained; intended to run on its own
    // thread. Every batch it receives is parsed, stored, and counted as
    // network-sourced.
    void receiveLoop();

    // Reads `path` line by line, routing each parsed record to its owner.
    // Malformed lines are logged and skipped; a missing file is a warning, not
    // a failure. Flushes any partial batches before returning.
    void loadLocalFile(const std::string& path);

    [[nodiscard]] uint64_t linesRead() const;

private:
    void routeRecord(const Record& rec, std::unordered_map<int, std::string>& pending);
    void flushPending(const std::unordered_map<int, std::string>& pending);

    int id_;
    const Schema& schema_;
    const Partitioner& partitioner_;
    NetworkHub& hub_;
    std::size_t batchBytes_;
    KvStore store_;
    std::atomic<uint64_t> linesRead_{0};
};
