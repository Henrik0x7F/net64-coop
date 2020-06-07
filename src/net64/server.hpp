//
// Created by henrik on 01.10.19
// Copyright 2019 Net64 Coop Project
// Licensed under GPLv3
// Refer to the LICENSE file included
//

#pragma once

#include <chrono>
#include <map>
#include <memory>

#include <common/resource_handle.hpp>
#include <enet/enet.h>

#include "net64/logging.hpp"
#include "net64/net/net_message.hpp"
#include "net64/net/protocol.hpp"


namespace Net64
{
struct Player;

// @todo: error codes
struct Server
{
    using Clock = std::chrono::steady_clock;
    using peer_id_t = std::uintptr_t;
    using HostHandle = ResourceHandle<&enet_host_destroy>;
    using PacketHandle = ResourceHandle<&enet_packet_destroy>;

    Server(std::uint16_t port, std::size_t max_clients);

    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void set_port(std::uint16_t port);

    void set_max_clients(std::size_t max_clients);

    void tick(std::chrono::milliseconds max_tick_time = std::chrono::milliseconds(0));

    [[nodiscard]] bool& accept_new_peers();
    [[nodiscard]] const bool& accept_new_peers() const;

    void broadcast(const INetMessage& msg);

    void disconnect_all(Net::S_DisconnectCode code);
    void reset_all();

    [[nodiscard]] std::size_t connected_peers() const;

    [[nodiscard]] std::size_t max_peers() const;

    [[nodiscard]] std::uint16_t port() const;

    static void send(ENetPeer& peer, const INetMessage& msg);

    static void disconnect(ENetPeer& peer, Net::S_DisconnectCode reason);

    static Player*& player(ENetPeer& peer);

private:
    void on_connect(ENetPeer& peer, std::uint32_t userdata);
    void on_disconnect(ENetPeer& peer, std::uint32_t userdata);
    void on_net_message(const ENetPacket& packet);

    void destroy_client(ENetPeer& peer);

    HostHandle host_;
    bool accept_new_{true};

    CLASS_LOGGER_("server")
};

} // namespace Net64
