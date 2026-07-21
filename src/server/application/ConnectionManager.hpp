#pragma once

#include <cstddef>
#include <string>
#include <unordered_set>

class ConnectionManager {
public:
    void onConnected(const std::string& connectionId);
    void onDisconnected(const std::string& connectionId);

    bool isConnected(const std::string& connectionId) const;
    std::size_t connectionCount() const;

private:
    std::unordered_set<std::string> connections_;
};
