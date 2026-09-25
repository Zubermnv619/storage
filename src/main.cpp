#include <chrono>
#include <deque>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include "cluster_config.hpp"
#include "mock_socket.hpp"
#include "node.hpp"
#include "partitioner.hpp"

namespace {
void printBanner(const std::string& text) {
    std::cout << "\n== " << text << " ==\n";
}

bool verifyOwnership(const std::deque<Node>& nodes, const Partitioner& partitioner, size_t limit) {
    bool allOk = true;
    for (const auto& node : nodes) {
        for (const auto& key : node.store().sampleKeys(limit)) {
            int expectedOwner = partitioner.ownerOf(key);
            bool ok = (expectedOwner == node.id());
            if (!ok) allOk = false;
            std::cout << "  key=" << key
                      << " storedOn=node" << node.id()
                      << " expectedOwner=node" << expectedOwner
                      << (ok ? "  [OK]" : "  [MISMATCH]") << "\n";
        }
    }
    return allOk;
}

} 

int main(int argc, char** argv) {
    std::string configPath = "config/cluster.conf";
    size_t verifyLimit = 0;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--verify-sample=", 0) == 0) {
            try {
                verifyLimit = std::stoull(arg.substr(16));
            } catch (const std::exception&) {
                std::cerr << "Invalid --verify-sample value: " << arg << "\n";
                return 1;
            }
        } else {
            configPath = arg;
        }
    }

    ClusterConfig cfg;
    try {
        cfg = loadClusterConfig(configPath);
    } catch (const std::exception& ex) {
        std::cerr << "Failed to load config '" << configPath << "': " << ex.what() << "\n";
        return 1;
    }

    std::cout << "Loaded config: " << cfg.numNodes << " node(s), "
              << cfg.schema.fieldCount() << " field(s), key field='"
              << cfg.schema.keyField().name << "'\n";

    NetworkHub  hub(cfg.numNodes);
    Partitioner partitioner(cfg.numNodes);

    std::deque<Node> nodes;
    for (int i = 0; i < cfg.numNodes; ++i) {
        nodes.emplace_back(i, cfg.schema, partitioner, hub);
    }

    printBanner("Starting cluster");

    std::vector<std::thread> receivers;
    for (auto& node : nodes) {
        receivers.emplace_back([&node] { node.receiveLoop(); });
    }

    auto startTime = std::chrono::steady_clock::now();
    std::vector<std::thread> loaders;
    for (auto& node : nodes) {
        std::string path = dataFilePath(cfg, node.id());
        loaders.emplace_back([&node, path] { node.loadLocalFile(path); });
    }
    for (auto& t : loaders) t.join();

    for (auto& node : nodes) hub.closeInbox(node.id());
    for (auto& t : receivers) t.join();

    auto endTime = std::chrono::steady_clock::now();
    double seconds = std::chrono::duration<double>(endTime - startTime).count();

    printBanner("Load statistics");
    uint64_t totalStored = 0;
    for (const auto& node : nodes) {
        StoreStats s = node.store().stats();
        totalStored += node.store().size();
        std::cout << "  node" << node.id()
                  << ": linesRead=" << node.linesRead()
                  << " storedRecords=" << node.store().size()
                  << " (inserted=" << s.recordsInserted
                  << ", overwritten=" << s.recordsOverwritten
                  << ", fromNetwork=" << s.recordsReceivedFromNetwork << ")\n";
    }
    std::cout << "  ---\n  total unique keys stored across cluster: " << totalStored << "\n";
    std::cout << "  elapsed: " << std::fixed << std::setprecision(3) << seconds << "s\n";

    printBanner(verifyLimit == 0 ? "Ownership verification (all keys)"
                                 : "Ownership verification (sample)");
    bool ok = verifyOwnership(nodes, partitioner, verifyLimit);
    std::cout << (ok ? "All sampled keys are on their correct owner node.\n"
                      : "MISMATCHES FOUND -- see above.\n");

    return ok ? 0 : 1;
}
