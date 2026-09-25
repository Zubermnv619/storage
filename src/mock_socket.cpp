#include "mock_socket.hpp"

bool MockInbox::connect() const {
    return true;
}

void MockInbox::send(int fromNode, const std::string& payload) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(WireMessage{fromNode, payload});
    }
    cv_.notify_one();
}

bool MockInbox::recv(WireMessage& out) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !queue_.empty() || closed_; });
    if (queue_.empty()) return false;
    out = std::move(queue_.front());
    queue_.pop();
    return true;
}

void MockInbox::close() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
    }
    cv_.notify_all();
}

NetworkHub::NetworkHub(int numNodes) : inboxes_(numNodes) {}

bool NetworkHub::connect(int targetNode) const {
    return targetNode >= 0 && targetNode < static_cast<int>(inboxes_.size())
        && inboxes_[targetNode].connect();
}

void NetworkHub::send(int fromNode, int toNode, const std::string& payload) {
    inboxes_.at(toNode).send(fromNode, payload);
}

bool NetworkHub::recv(int nodeId, WireMessage& out) {
    return inboxes_.at(nodeId).recv(out);
}

void NetworkHub::closeInbox(int nodeId) {
    inboxes_.at(nodeId).close();
}

int NetworkHub::nodeCount() const {
    return static_cast<int>(inboxes_.size());
}
