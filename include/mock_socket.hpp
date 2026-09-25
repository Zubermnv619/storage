#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

struct WireMessage {
    int fromNode;
    std::string payload;
};

class MockInbox {
public:
    void send(int fromNode, const std::string& payload);
    bool recv(WireMessage& out);
    void close();

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<WireMessage> queue_;
    bool closed_ = false;
};

class NetworkHub {
public:
    explicit NetworkHub(int numNodes);

    bool connect(int targetNode) const;
    void send(int fromNode, int toNode, const std::string& payload);
    bool recv(int nodeId, WireMessage& out);
    void closeInbox(int nodeId);
    int nodeCount() const;

private:
    std::vector<MockInbox> inboxes_;
};
