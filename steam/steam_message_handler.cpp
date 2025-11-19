#include "steam_message_handler.h"
#include <iostream>
#include <cstring>
#include <chrono>
#include <thread>
#include <steam_api.h>
#include <isteamnetworkingsockets.h>

SteamMessageHandler::SteamMessageHandler(boost::asio::io_context& io_context, ISteamNetworkingSockets* interface, std::vector<HSteamNetConnection>& connections, std::map<HSteamNetConnection, std::shared_ptr<TCPClient>>& clientMap, std::mutex& clientMutex, std::mutex& connectionsMutex, std::unique_ptr<TCPServer>& server, bool& g_isHost, int& localPort)
    : io_context_(io_context), m_pInterface_(interface), connections_(connections), clientMap_(clientMap), clientMutex_(clientMutex), connectionsMutex_(connectionsMutex), server_(server), g_isHost_(g_isHost), localPort_(localPort), running_(false) {}

SteamMessageHandler::~SteamMessageHandler() {
    stop();
}

void SteamMessageHandler::start() {
    if (running_) return;
    running_ = true;
    thread_ = std::thread([this]() { run(); });
}

void SteamMessageHandler::stop() {
    if (!running_) return;
    running_ = false;
    if (thread_.joinable()) {
        thread_.join();
    }
}

void SteamMessageHandler::run() {
    while (running_) {
        // Poll networking
        m_pInterface_->RunCallbacks();

        // Receive messages
        pollMessages();

        // Sleep a bit to avoid busy loop
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void SteamMessageHandler::pollMessages() {
    std::vector<HSteamNetConnection> currentConnections;
    {
        std::lock_guard<std::mutex> lockConn(connectionsMutex_);
        currentConnections = connections_;
    }
    std::lock_guard<std::mutex> lock(clientMutex_);
    for (auto conn : currentConnections) {
        ISteamNetworkingMessage* pIncomingMsgs[10];
        int numMsgs = m_pInterface_->ReceiveMessagesOnConnection(conn, pIncomingMsgs, 10);
        for (int i = 0; i < numMsgs; ++i) {
            ISteamNetworkingMessage* pIncomingMsg = pIncomingMsgs[i];
            const char* data = (const char*)pIncomingMsg->m_pData;
            size_t size = pIncomingMsg->m_cbSize;
            // Handle tunnel packets with multiplexing
            if (server_ && server_->getMultiplexManager()) {
                server_->getMultiplexManager()->handleTunnelPacket(data, size);
            }
            pIncomingMsg->Release();
        }
    }
}