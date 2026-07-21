#include "server/infrastructure/transport/WebSocketTransport.hpp"

#include <sstream>

WebSocketTransport::WebSocketTransport() {
    server_.clear_access_channels(websocketpp::log::alevel::all);
    server_.clear_error_channels(websocketpp::log::elevel::all);
    server_.init_asio();

    server_.set_open_handler([this](ConnectionHandle hdl) {
        const std::string id = idFor(hdl);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            connections_[id] = hdl;
        }
        if (onOpen_) onOpen_(id);
    });

    server_.set_close_handler([this](ConnectionHandle hdl) {
        const std::string id = idFor(hdl);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            connections_.erase(id);
        }
        if (onClose_) onClose_(id);
    });

    server_.set_message_handler([this](ConnectionHandle hdl, Server::message_ptr msg) {
        const std::string id = idFor(hdl);
        if (onMessage_) onMessage_(id, msg->get_payload());
    });
}

std::string WebSocketTransport::idFor(const ConnectionHandle& hdl) const {
    std::ostringstream oss;
    oss << hdl.lock().get();
    return oss.str();
}

void WebSocketTransport::send(const std::string& connectionId, const std::string& rawJson) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = connections_.find(connectionId);
    if (it == connections_.end()) return;  // Already disconnected - silently drop.
    server_.send(it->second, rawJson, websocketpp::frame::opcode::text);
}

void WebSocketTransport::broadcast(const std::string& rawJson) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [id, hdl] : connections_) {
        server_.send(hdl, rawJson, websocketpp::frame::opcode::text);
    }
}

void WebSocketTransport::setOnOpen(OnOpenHandler handler) { onOpen_ = std::move(handler); }
void WebSocketTransport::setOnClose(OnCloseHandler handler) { onClose_ = std::move(handler); }
void WebSocketTransport::setOnMessage(OnMessageHandler handler) { onMessage_ = std::move(handler); }

void WebSocketTransport::run(uint16_t port) {
    server_.set_reuse_addr(true);
    server_.listen(port);
    server_.start_accept();
    server_.run();  // Blocks the calling thread until stop() is invoked.
}

void WebSocketTransport::stop() {
    server_.stop_listening();
    server_.stop();
}
