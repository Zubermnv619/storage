#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <vector>

#include "cluster_config.hpp"

namespace fs = std::filesystem;

namespace {

std::string randomString(std::mt19937& rng, int minLen, int maxLen) {
    static const char alphabet[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::uniform_int_distribution<int> lenDist(minLen, maxLen);
    std::uniform_int_distribution<int> charDist(0, sizeof(alphabet) - 2);
    int len = lenDist(rng);
    std::string s;
    s.reserve(len);
    for (int i = 0; i < len; ++i) s.push_back(alphabet[charDist(rng)]);
    return s;
}

std::string makeFieldValue(const FieldDef& def, std::mt19937& rng) {
    if (def.type == FieldType::Int32) {
        std::uniform_int_distribution<int32_t> dist(0, 1000000);
        return std::to_string(dist(rng));
    }
    return randomString(rng, 4, 12);
}

}

int main(int argc, char** argv) {
    try {
        if (argc < 3) {
            std::cerr << "usage: " << argv[0]
                      << " <config_path> <target_bytes_per_file> [duplicate_ratio]\n";
            return 1;
        }

    std::string configPath = argv[1];
    size_t targetBytes = std::stoull(argv[2]);
    double dupRatio = (argc > 3) ? std::stod(argv[3]) : 0.15;

    ClusterConfig cfg = loadClusterConfig(configPath);
    fs::create_directories(cfg.dataDir);

    std::mt19937 rng(std::random_device{}());

    std::vector<std::string> keyPool;
    keyPool.reserve(targetBytes / 20); 

    const FieldDef& keyDef = cfg.schema.keyField();

    for (int nodeId = 0; nodeId < cfg.numNodes; ++nodeId) {
        std::string path = dataFilePath(cfg, nodeId);
        std::ofstream out(path, std::ios::trunc);
        if (!out) {
            std::cerr << "Could not open " << path << " for writing\n";
            return 1;
        }

        size_t bytesWritten = 0;
        std::uniform_real_distribution<double> chance(0.0, 1.0);

        while (bytesWritten < targetBytes) {
            std::ostringstream line;

            bool reuseKey = !keyPool.empty() && chance(rng) < dupRatio;
            std::string keyStr = reuseKey
                ? keyPool[std::uniform_int_distribution<size_t>(0, keyPool.size() - 1)(rng)]
                : makeFieldValue(keyDef, rng);
            if (!reuseKey) keyPool.push_back(keyStr);

            line << keyStr;
            for (size_t i = 1; i < cfg.schema.fieldCount(); ++i) {
                line << '|' << makeFieldValue(cfg.schema.fields()[i], rng);
            }
            line << '\n';

            std::string lineStr = line.str();
            out << lineStr;
            bytesWritten += lineStr.size();
        }

        std::cout << "wrote " << path << " (" << bytesWritten << " bytes)\n";
    }

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Data generation failed: " << ex.what() << "\n";
        return 1;
    }
}
