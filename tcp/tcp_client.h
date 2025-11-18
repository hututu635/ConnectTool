#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <functional>

using boost::asio::ip::tcp;

// TCP Client class
class TCPClient {
public:
    TCPClient(const std::string& host, int port);
    ~TCPClient();

    bool connect();
    void disconnect();
    void send(const std::string& message);
    void send(const char* data, size_t size);
    void setReceiveCallback(std::function<void(const std::string&)> callback);
    void setReceiveCallback(std::function<void(const char*, size_t)> callback);
    void setDisconnectCallback(std::function<void()> callback);

private:
    void start_read();
    void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred);

    std::string host_;
    int port_;
    bool connected_;
    bool disconnected_;
    boost::asio::io_context io_context_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_;
    std::shared_ptr<tcp::socket> socket_;
    std::thread clientThread_;
    std::mutex socketMutex_;
    std::function<void(const std::string&)> receiveCallback_;
    std::function<void(const char*, size_t)> receiveCallbackBytes_;
    std::function<void()> disconnectCallback_;
    std::vector<char> buffer_;
};