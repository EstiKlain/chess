#include "client_net/infrastructure/WebSocketClientLink.hpp"

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {
// How long a single handshake attempt is allowed to hang before being
// treated as a failure - named, not inline, per this project's convention
// for timing constants (e.g. server_config::kTickIntervalMs).
constexpr std::chrono::seconds kConnectTimeout{3};
}  // namespace

WebSocketClientLink::WebSocketClientLink() {
    client_.clear_access_channels(websocketpp::log::alevel::all);
    client_.clear_error_channels(websocketpp::log::elevel::all);
    client_.init_asio();

    client_.set_message_handler([this](ConnectionHandle, Client::message_ptr msg) {
        if (onMessage_) {
            onMessage_(msg->get_payload());
        }
    });

    // Perpetual mode keeps client_.run() from returning on its own once the
    // last open connection closes - without this, ioThread_ would silently
    // finish after the first disconnect, and a later reconnect attempt would
    // schedule work on an io_service nothing is servicing anymore (it would
    // just hang forever, never firing open/fail). Verified with an isolated
    // spike against a real server before relying on this.
    client_.start_perpetual();

    // Started exactly once, here - NOT in connect(). Its lifetime is this
    // object's lifetime, independent of how many times connect() is called.
    // (Before perpetual mode existed here, connect() spawned this thread
    // itself, which was safe only because client_.run() always returned on
    // its own before a second connect() could ever happen. Perpetual mode
    // breaks that: the first thread never dies, so re-spawning it on a later
    // connect() would assign over a still-joinable std::thread and call
    // std::terminate(). Moving it here removes the possibility entirely.)
    ioThread_ = std::thread([this]() { client_.run(); });
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
    // the open/fail handler fires, or kConnectTimeout elapses.
    //
    // Held by shared_ptr, captured BY VALUE into both handlers (not by
    // reference to these locals) - deliberately, not an oversight. con is
    // kept alive by websocketpp internally for as long as this connection
    // attempt exists, independent of this stack frame; with a bounded wait,
    // connect() can now return (via timeout) before a handler has fired. A
    // handler capturing these locals by reference would then dereference
    // already-destroyed stack memory if the handshake resolved late. Each
    // handler holding its own reference-counted copy of the same struct
    // means it stays alive as long as either side needs it - a late-firing
    // handler after a timeout just updates a struct nobody is waiting on
    // anymore, which is harmless.
    struct OpenSync {
        std::mutex mutex;
        std::condition_variable cv;
        bool opened = false;
        bool failed = false;
    };
    auto sync = std::make_shared<OpenSync>();

    con->set_open_handler([sync](ConnectionHandle) {
        {
            std::lock_guard<std::mutex> lock(sync->mutex);
            sync->opened = true;
        }
        sync->cv.notify_all();
    });
    con->set_fail_handler([sync](ConnectionHandle) {
        {
            std::lock_guard<std::mutex> lock(sync->mutex);
            sync->failed = true;
        }
        sync->cv.notify_all();
    });

    // Persistent close handler (captures only `this`, unlike the local
    // open/fail handlers above) - fires for the entire lifetime of this
    // connection, including long after connect() has returned. Suppressed
    // during an intentional stop() via stopping_, so shutting the game down
    // normally never triggers a spurious "reconnect!" signal.
    con->set_close_handler([this](ConnectionHandle) {
        if (onClose_ && !stopping_.load()) {
            onClose_();
        }
    });

    client_.connect(con);

    std::unique_lock<std::mutex> lock(sync->mutex);
    const bool resolved = sync->cv.wait_for(lock, kConnectTimeout, [sync] { return sync->opened || sync->failed; });
    if (!resolved) {
        // Abandon the in-flight handshake outright - terminate(), not
        // close(): close() assumes an already-open connection and performs
        // a graceful WS close handshake, but a timed-out attempt may not
        // even have reached "open" yet. terminate() tears the connection
        // down regardless of what state it was in, which also guarantees
        // the open/fail handler above can no longer fire late at all -
        // without this, a handshake that succeeds moments after the
        // timeout would leave an orphaned open socket that nothing holds a
        // reference to and that never sends LOGIN/RECONNECT, leaking a
        // connection on both ends.
        websocketpp::lib::error_code terminateEc;
        con->terminate(terminateEc);
        throw std::runtime_error("timed out connecting to " + uri.str());
    }
    if (sync->failed) {
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

void WebSocketClientLink::setOnClose(OnCloseHandler handler) {
    onClose_ = std::move(handler);
}

void WebSocketClientLink::stop() {
    stopping_ = true;
    client_.stop_perpetual();
    client_.stop();
    if (ioThread_.joinable()) {
        ioThread_.join();
    }
}
