//
// Created by henrik on 05.06.20.
// Copyright 2020 Net64 Project
// Licensed under GPLv3
// Refer to the LICENSE file included
//

#include "net64/server/player.hpp"
#include "player_manager.hpp"


namespace Net64
{
void PlayerManager::on_message(const Net::C_Handshake& msg, ENetPeer& sender)
{
    if(msg.client_version_str.empty() || msg.name.empty())
    {
        logger()->info("Received invalid handshake from client {}", sender.address.host);
    }
    logger()->info("Received handshake from player {}, client version: {}", msg.name, msg.client_version_str);
    Server::player(sender)->name = msg.name;
}

} // namespace Net64
