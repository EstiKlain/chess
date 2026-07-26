#include "client_net/infrastructure/WebSocketClientLink.hpp"

#include <condition_variable>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <utility>

WebSocketClientLink::WebSocketClientLink() {
    client_.clear_access_channels(websocketpp::log::alevel::all);
    client_.clear_error_channels(websocketpp::log::elevel::all);
    client_.init_asio();

    client_.set_message_handler([this](ConnectionHandle, Client::message_ptr msg) {
        if (onMessage_) {
            onMessage_(msg->get_payload());
        }
    });
}

WebSocketClientLink::~WebSocketClientLink() {
    stop();
}

void WebSocketClientLink::connect(const std::string& host, uint16_t port) {
    std::ostringstream uri;
    uri << "ws://" << host << ":" << port;

    websocketpp::lib::error_code ec;
    const Client::connection_ptr con = client_.get_connection(uri.str(), ec);
    if (ec) {
        throw std::runtime_error("failed to create connection to " + uri.str() + ": " + ec.message());
    }
    handle_ = con->get_handle();

    // Handshake completion is only known asynchronously, on the IO thread -
    // block the caller (connect() is meant to behave synchronously) until
    // the open/fail handler fires.
    std::mutex openMutex;
    std::condition_variable openCv;
    bool opened = false;
    bool failed = false;

    con->set_open_handler([&](ConnectionHandle) {
        {
            std::lock_guard<std::mutex> lock(openMutex);
            opened = true;
        }
        openCv.notify_all();
    });
    con->set_fail_handler([&](ConnectionHandle) {
        {
            std::lock_guard<std::mutex> lock(openMutex);
            failed = true;
        }
        openCv.notify_all();
    });

    client_.connect(con);
    ioThread_ = std::thread([this]() { client_.run(); });

    std::unique_lock<std::mutex> lock(openMutex);
    openCv.wait(lock, [&] { return opened || failed; });
    if (failed) {
        throw std::runtime_error("failed to connect to " + uri.str());
    }
}

void WebSocketClientLink::send(const std::string& rawJson) {
    websocketpp::lib::error_code ec;
    client_.send(handle_, rawJson, websocketpp::frame::opcode::text, ec);
    // Silently drop send errors (e.g. connection already closed) - matches
    // WebSocketTransport::send's own "already disconnected - silently drop"
    // precedent for the analogous server-side case.
}

void WebSocketClientLink::setOnMessage(OnMessageHandler handler) {
    onMessage_ = std::move(handler);
}

void WebSocketClientLink::stop() {
    client_.stop();
    if (ioThread_.joinable()) {
        ioThread_.join();
    }
}
