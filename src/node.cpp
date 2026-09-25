#include "node.hpp"

#include <fstream>
#include <iostream>

Node::Node(int id, const Schema& schema, const Partitioner& partitioner, NetworkHub& hub)
    : id_(id), schema_(schema), partitioner_(partitioner), hub_(hub) {}

int Node::id() const {
    return id_;
}

KvStore& Node::store() {
    return store_;
}

const KvStore& Node::store() const {
    return store_;
}

void Node::receiveLoop() {
    WireMessage msg;
    while (hub_.recv(id_, msg)) {
        Record rec = parseRecordLine(msg.payload, schema_);
        store_.put(rec);
        store_.noteReceivedFromNetwork();
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
    while (std::getline(in, line)) {
        ++lineNo;
        if (line.empty()) continue;
        try {
            Record rec = parseRecordLine(line, schema_);
            routeRecord(rec);
            ++linesRead_;
        } catch (const std::exception& ex) {
            std::cerr << "[node " << id_ << "] skipping malformed line "
                      << lineNo << " in " << path << ": " << ex.what() << "\n";
        }
    }
}

uint64_t Node::linesRead() const {
    return linesRead_;
}

void Node::routeRecord(const Record& rec) {
    int owner = partitioner_.ownerOf(rec.keyAsString());
    if (owner == id_) {
        store_.put(rec);
    } else {
        hub_.connect(owner);
        hub_.send(id_, owner, serializeRecord(rec));
    }
}
