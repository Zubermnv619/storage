#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <unordered_map>

#include "record.hpp"
#include "schema.hpp"
#include "mock_socket.hpp"
#include "partitioner.hpp"
#include "kv_store.hpp"

class Node {
public:
    Node(int id, const Schema& schema, const Partitioner& partitioner, NetworkHub& hub);

    [[nodiscard]] int id() const;
    KvStore& store();
    const KvStore& store() const;

    void receiveLoop();
    void loadLocalFile(const std::string& path);
    [[nodiscard]] uint64_t linesRead() const;

private:
    void routeRecord(const Record& rec, std::unordered_map<int, std::string>& pending);
    void flushPending(std::unordered_map<int, std::string>& pending);

    int id_;
    const Schema& schema_;
    const Partitioner& partitioner_;
    NetworkHub& hub_;
    KvStore store_;
    std::atomic<uint64_t> linesRead_{0};
};
