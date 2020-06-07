//
// Created by henrik on 06.06.20.
// Copyright 2020 Net64 Project
// Licensed under GPLv3
// Refer to the LICENSE file included
//

#pragma once

#include "net64/server.hpp"


namespace Net64
{
struct Player
{
    Player(Server& server, ENetPeer& peer);

    Server::Clock::time_point connect_time{};
    Net::player_id_t player_id{};
    std::string name{};

    bool handshaked() const;
    void disconnect(Net::S_DisconnectCode reason);
    void send(const INetMessage& msg);
    void broadcast(const INetMessage& msg);

private:
    Server* server_{};
    ENetPeer* peer_{};
};

} // namespace Net64
