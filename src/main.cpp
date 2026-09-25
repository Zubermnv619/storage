#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "cluster_config.hpp"
#include "mock_socket.hpp"
#include "node.hpp"
#include "partitioner.hpp"

namespace {

// Verification prints at most this many per-key lines; the totals below it are
// always exhaustive. Without the cap, "verify everything" on a 100 MB/node
// dataset would emit millions of lines and drown the real summary.
constexpr uint64_t kMaxVerifyDetailLines = 20;

constexpr const char* kVerifySamplePrefix = "--verify-sample=";
constexpr const char* kBatchBytesPrefix = "--batch-bytes=";

struct Options {
    std::string configPath = "config/cluster.conf";
    uint64_t verifySamplePerNode = 0;  // 0 => verify every stored key
    std::size_t batchBytes = kDefaultBatchBytes;
    bool showHelp = false;
};

struct VerifyResult {
    uint64_t keysChecked = 0;
    uint64_t mismatches = 0;
};

void printBanner(const std::string& text) {
    std::cout << "\n== " << text << " ==\n";
}

void printUsage(const char* argv0) {
    std::cout
        << "usage: " << argv0 << " [config_path] [options]\n\n"
        << "options:\n"
        << "  --verify-sample=N  verify at most N keys per node (default: all keys)\n"
        << "  --batch-bytes=N    flush a peer batch once it reaches N bytes\n"
        << "                     (default " << kDefaultBatchBytes
        << "; pass 1 to send one message per record)\n"
        << "  -h, --help         show this help\n";
}

bool startsWith(const std::string& value, const char* prefix) {
    return value.rfind(prefix, 0) == 0;
}

std::string stripPrefix(const std::string& value, const char* prefix) {
    return value.substr(std::string(prefix).size());
}

// Parses an unsigned CLI integer, rejecting non-numeric input and trailing
// junk (so "--batch-bytes=12abc" is an error rather than silently 12).
uint64_t parseUnsigned(const std::string& value, const char* flagName) {
    std::size_t consumed = 0;
    unsigned long long parsed = 0;
    try {
        parsed = std::stoull(value, &consumed);
    } catch (const std::exception& ex) {
        throw std::runtime_error("invalid value for " + std::string(flagName) + " ('"
                                 + value + "'): " + ex.what());
    }
    if (consumed != value.size()) {
        throw std::runtime_error("invalid value for " + std::string(flagName) + " ('"
                                 + value + "'): trailing characters");
    }
    return static_cast<uint64_t>(parsed);
}

void parseOptions(int argc, char** argv, Options& opts) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            opts.showHelp = true;
        } else if (startsWith(arg, kVerifySamplePrefix)) {
            opts.verifySamplePerNode = parseUnsigned(stripPrefix(arg, kVerifySamplePrefix),
                                                     "--verify-sample");
        } else if (startsWith(arg, kBatchBytesPrefix)) {
            opts.batchBytes = static_cast<std::size_t>(
                parseUnsigned(stripPrefix(arg, kBatchBytesPrefix), "--batch-bytes"));
        } else if (!arg.empty() && arg[0] == '-') {
            throw std::runtime_error("unknown option: " + arg);
        } else {
            opts.configPath = arg;
        }
    }
}

// Recomputes every stored key's owner straight from the partitioner -- i.e.
// independently of the routing path that placed it there -- and counts
// mismatches. `samplePerNode == 0` checks every key.
VerifyResult verifyOwnership(const std::deque<Node>& nodes, const Partitioner& partitioner,
                             uint64_t samplePerNode, uint64_t maxDetailLines) {
    VerifyResult result;
    uint64_t detailLines = 0;
    const uint64_t perNodeLimit = (samplePerNode == 0)
                                      ? std::numeric_limits<uint64_t>::max()
                                      : samplePerNode;

    for (const auto& node : nodes) {
        uint64_t checkedOnNode = 0;
        node.store().forEachKey([&](const std::string& key) {
            if (checkedOnNode >= perNodeLimit) return false;
            ++checkedOnNode;
            ++result.keysChecked;

            const int expectedOwner = partitioner.ownerOf(key);
            const bool ok = (expectedOwner == node.id());
            if (!ok) ++result.mismatches;

            if (detailLines < maxDetailLines) {
                ++detailLines;
                std::cout << "  key=" << key
                          << " storedOn=node" << node.id()
                          << " expectedOwner=node" << expectedOwner
                          << (ok ? "  [OK]" : "  [MISMATCH]") << "\n";
            }
            return true;
        });
    }
    return result;
}

}  // namespace

int main(int argc, char** argv) {
    Options opts;
    try {
        parseOptions(argc, argv, opts);
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << "\n";
        printUsage(argv[0]);
        return 2;
    }

    if (opts.showHelp) {
        printUsage(argv[0]);
        return 0;
    }

    ClusterConfig cfg;
    try {
        cfg = loadClusterConfig(opts.configPath);
    } catch (const std::exception& ex) {
        std::cerr << "Failed to load config '" << opts.configPath << "': " << ex.what() << "\n";
        return 1;
    }

    std::cout << "Loaded config: " << cfg.numNodes << " node(s), "
              << cfg.schema.fieldCount() << " field(s), key field='"
              << cfg.schema.keyField().name << "'\n";
    std::cout << "Batch flush threshold: " << opts.batchBytes << " byte(s)\n";

    NetworkHub hub(cfg.numNodes);
    const Partitioner partitioner(cfg.numNodes);

    std::deque<Node> nodes;
    for (int i = 0; i < cfg.numNodes; ++i) {
        nodes.emplace_back(i, cfg.schema, partitioner, hub, opts.batchBytes);
    }

    printBanner("Starting cluster");

    std::vector<std::thread> receivers;
    receivers.reserve(nodes.size());
    for (auto& node : nodes) {
        receivers.emplace_back([&node] { node.receiveLoop(); });
    }

    const auto startTime = std::chrono::steady_clock::now();

    std::vector<std::thread> loaders;
    loaders.reserve(nodes.size());
    for (auto& node : nodes) {
        const std::string path = dataFilePath(cfg, node.id());
        loaders.emplace_back([&node, path] { node.loadLocalFile(path); });
    }
    for (auto& loader : loaders) loader.join();

    for (auto& node : nodes) hub.closeInbox(node.id());
    for (auto& receiver : receivers) receiver.join();

    const auto endTime = std::chrono::steady_clock::now();
    const double seconds = std::chrono::duration<double>(endTime - startTime).count();

    printBanner("Load statistics");
    uint64_t totalStored = 0;
    uint64_t totalLines = 0;
    for (const auto& node : nodes) {
        const StoreStats s = node.store().stats();
        totalStored += node.store().size();
        totalLines += node.linesRead();
        std::cout << "  node" << node.id()
                  << ": linesRead=" << node.linesRead()
                  << " storedRecords=" << node.store().size()
                  << " (inserted=" << s.recordsInserted
                  << ", overwritten=" << s.recordsOverwritten
                  << ", fromNetwork=" << s.recordsReceivedFromNetwork << ")\n";
    }
    std::cout << "  ---\n"
              << "  total lines read: " << totalLines << "\n"
              << "  total unique keys stored across cluster: " << totalStored << "\n";
    std::cout << "  elapsed: " << std::fixed << std::setprecision(3) << seconds << "s\n";

    const bool exhaustive = (opts.verifySamplePerNode == 0);
    printBanner(exhaustive ? "Ownership verification (all keys)"
                           : "Ownership verification (sample, "
                                 + std::to_string(opts.verifySamplePerNode) + " per node)");

    const VerifyResult verification =
        verifyOwnership(nodes, partitioner, opts.verifySamplePerNode, kMaxVerifyDetailLines);

    std::cout << "  checked " << verification.keysChecked
              << " key(s), mismatches=" << verification.mismatches << "\n";

    const bool ok = (verification.mismatches == 0);
    std::cout << (ok ? "All sampled keys are on their correct owner node.\n"
                     : "MISMATCHES FOUND -- see above.\n");
    return ok ? 0 : 1;
}
