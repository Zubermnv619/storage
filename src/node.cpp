#include "node.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

Node::Node(int id, const Schema& schema, const Partitioner& partitioner, NetworkHub& hub,
           std::size_t batchBytes)
    : id_(id),
      schema_(schema),
      partitioner_(partitioner),
      hub_(hub),
      batchBytes_(batchBytes == 0 ? 1 : batchBytes) {}

int Node::id() const {
    return id_;
}

KvStore& Node::store() {
    return store_;
}

const KvStore& Node::store() const {
    return store_;
}

std::size_t Node::batchBytes() const {
    return batchBytes_;
}

void Node::receiveLoop() {
    WireMessage msg;
    while (hub_.recv(id_, msg)) {
        std::istringstream batch(msg.payload);
        std::string line;
        while (std::getline(batch, line)) {
            if (line.empty()) continue;
            try {
                const Record rec = parseRecordLine(line, schema_);
                store_.put(rec);
                store_.noteReceivedFromNetwork();
            } catch (const std::exception& ex) {
                std::cerr << "[node " << id_ << "] skipping malformed network record: "
                          << ex.what() << "\n";
            }
        }
    }
}

void Node::loadLocalFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        std::cerr << "[node " << id_ << "] warning: no local data file at "
                  << path << " (skipping)\n";
        return;
    }

    std::string line;
    uint64_t lineNo = 0;
    std::unordered_map<int, std::string> pending;
    while (std::getline(in, line)) {
        ++lineNo;
        if (line.empty()) continue;
        try {
            const Record rec = parseRecordLine(line, schema_);
            routeRecord(rec, pending);
            ++linesRead_;
        } catch (const std::exception& ex) {
            std::cerr << "[node " << id_ << "] skipping malformed line "
                      << lineNo << " in " << path << ": " << ex.what() << "\n";
        }
    }
    flushPending(pending);
}

uint64_t Node::linesRead() const {
    return linesRead_;
}

void Node::routeRecord(const Record& rec, std::unordered_map<int, std::string>& pending) {
    const int owner = partitioner_.ownerOf(rec.keyAsString());
    if (owner == id_) {
        store_.put(rec);
        return;
    }

    std::string& batch = pending[owner];
    batch += serializeRecord(rec);
    batch.push_back('\n');
    if (batch.size() >= batchBytes_) {
        hub_.send(id_, owner, batch);
        batch.clear();
    }
}

void Node::flushPending(const std::unordered_map<int, std::string>& pending) {
    for (const auto& entry : pending) {
        if (!entry.second.empty()) {
            hub_.send(id_, entry.first, entry.second);
        }
    }
}
