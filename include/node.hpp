#pragma once

#include <atomic>
#include <cstdint>
#include <string>

#include "record.hpp"
#include "schema.hpp"
#include "mock_socket.hpp"
#include "partitioner.hpp"
#include "kv_store.hpp"

class Node {
public:
    Node(int id, const Schema& schema, const Partitioner& partitioner, NetworkHub& hub);

    int id() const;
    KvStore& store();
    const KvStore& store() const;

    void receiveLoop();
    void loadLocalFile(const std::string& path);
    uint64_t linesRead() const;

private:
    void routeRecord(const Record& rec);

    int id_;
    const Schema& schema_;
    const Partitioner& partitioner_;
    NetworkHub& hub_;
    KvStore store_;
    std::atomic<uint64_t> linesRead_{0};
};
