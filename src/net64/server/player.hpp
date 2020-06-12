//
// Created by henrik on 06.06.20.
// Copyright 2020 Net64 Project
// Licensed under GPLv3
// Refer to the LICENSE file included
//

#pragma once

#include "common/id_manager.hpp"
#include "net64/server.hpp"


namespace Net64
{
struct Player
{
    Player(Server& server, ENetPeer& peer);

    bool handshaked() const;
    void disconnect(Net::S_DisconnectCode reason);
    void send(const INetMessage& msg);
    void broadcast(const INetMessage& msg);

    ENetPeer& peer();


    Server::Clock::time_point connect_time{};
    IdHandle<Net::player_id_t> id{};
    std::string name{};

private:
    Server* server_{};
    ENetPeer* peer_{};
};

} // namespace Net64
