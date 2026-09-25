#include <cassert>
#include <deque>
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>

#include "kv_store.hpp"
#include "mock_socket.hpp"
#include "node.hpp"
#include "partitioner.hpp"
#include "record.hpp"

namespace {

void testRecordAndStore() {
    Schema schema;
    schema.addField("id", FieldType::Int32);
    schema.addField("name", FieldType::String);
    Record record = parseRecordLine("7|Ada", schema);
    assert(record.keyAsString() == "7");
    assert(serializeRecord(record) == "7|Ada");

    KvStore store;
    assert(store.put(record));
    assert(!store.put(record));
    assert(store.contains("7"));
    assert(store.size() == 1);
}

void testPartitionerAndSocket() {
    Partitioner partitioner(3);
    assert(partitioner.ownerOf("key") >= 0 && partitioner.ownerOf("key") < 3);

    MockInbox inbox;
    inbox.send(1, "payload");
    WireMessage message{};
    assert(inbox.recv(message));
    assert(message.fromNode == 1 && message.payload == "payload");
    inbox.close();
    assert(!inbox.recv(message));
}

void testThreeNodeIntegration() {
    const std::filesystem::path root = "data/test_cluster";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    Schema schema;
    schema.addField("id", FieldType::Int32);
    schema.addField("value", FieldType::String);
    for (int nodeId = 0; nodeId < 3; ++nodeId) {
        std::ofstream out(root / ("node" + std::to_string(nodeId) + ".dat"));
        out << "1|first\n2|second\n3|third\n1|last\n";
    }

    NetworkHub hub(3);
    Partitioner partitioner(3);
    std::deque<Node> nodes;
    for (int nodeId = 0; nodeId < 3; ++nodeId) {
        nodes.emplace_back(nodeId, schema, partitioner, hub);
    }
    std::vector<std::thread> receivers;
    for (auto& node : nodes) receivers.emplace_back([&node] { node.receiveLoop(); });
    std::vector<std::thread> loaders;
    for (auto& node : nodes) {
        loaders.emplace_back([&node, &root] {
            node.loadLocalFile((root / ("node" + std::to_string(node.id()) + ".dat")).string());
        });
    }
    for (auto& loader : loaders) loader.join();
    for (auto& node : nodes) hub.closeInbox(node.id());
    for (auto& receiver : receivers) receiver.join();

    size_t total = 0;
    for (const auto& node : nodes) {
        total += node.store().size();
        node.store().forEachKey([&](const std::string& key) {
            assert(partitioner.ownerOf(key) == node.id());
            return true;
        });
    }
    assert(total == 3);
    std::filesystem::remove_all(root);
}

}

int main() {
    testRecordAndStore();
    testPartitionerAndSocket();
    testThreeNodeIntegration();
    return 0;
}
