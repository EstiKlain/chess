#include "server/infrastructure/transport/WebSocketTransport.hpp"

#include <sstream>
#include <vector>

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
    auto hdl = it->second;
    // server_.send() is only safe to call from the io_service thread (server_.run()'s
    // thread); this method is also called from main_server.cpp's tick thread, so the
    // actual send must be posted onto the io_service rather than invoked directly here.
    server_.get_io_service().post([this, hdl, rawJson]() {
        server_.send(hdl, rawJson, websocketpp::frame::opcode::text);
    });
}

void WebSocketTransport::broadcast(const std::string& rawJson) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ConnectionHandle> handles;
    handles.reserve(connections_.size());
    for (const auto& [id, hdl] : connections_) {
        handles.push_back(hdl);
    }
    server_.get_io_service().post([this, handles = std::move(handles), rawJson]() {
        for (const auto& hdl : handles) {
            server_.send(hdl, rawJson, websocketpp::frame::opcode::text);
        }
    });
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
