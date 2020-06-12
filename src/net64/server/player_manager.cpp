//
// Created by henrik on 05.06.20.
// Copyright 2020 Net64 Project
// Licensed under GPLv3
// Refer to the LICENSE file included
//

#include "net64/net/messages_server.hpp"
#include "net64/server/player.hpp"
#include "player_manager.hpp"


using Clock = std::chrono::steady_clock;

namespace Net64
{
void PlayerManager::on_connect(Server& server, Player& player)
{
    player.connect_time = Clock::now();
}

void PlayerManager::on_disconnect(Server& server, Player& player, Net::C_DisconnectCode code)
{
    if(!player.handshaked())
        return;

    // Notify other players of disconnect
    Net::S_ClientDisconnected snd_msg;
    snd_msg.player = player.id.id();
    snd_msg.disconnect_code = code;
    player.broadcast(snd_msg);
}

void PlayerManager::on_message(const Net::C_Handshake& msg, Server& server, Player& sender)
{
    // Check if player already got a player id
    if(sender.id.has_id())
    {
        logger()->info("Received another handshake from {} (name={} id={}). Disconnecting...",
                       Net::format_ip4(sender.peer().address),
                       sender.name,
                       sender.id.id());
        sender.disconnect(Net::S_DisconnectCode::PROTOCOL_VIOLATION);
        return;
    }

    // Check if handshake is valid. @todo Check for valid username
    if(msg.client_version_str.empty() || msg.name.empty())
    {
        logger()->info("Received invalid handshake from {}. Disconnecting...", Net::format_ip4(sender.peer().address));
        sender.disconnect(Net::S_DisconnectCode::PROTOCOL_VIOLATION);
        return;
    }

    // Store username and get an id
    sender.name = msg.name;
    sender.id = std::move(IdHandle<Net::player_id_t>(player_ids_));

    logger()->info("Received handshake from {} (name={} id={} version={})",
                   Net::format_ip4(sender.peer().address),
                   msg.name,
                   sender.id.id(),
                   msg.client_version_str);

    // Send id back
    {
        Net::S_Handshake snd_msg;
        snd_msg.local_player_id = sender.id.id();
        sender.send(snd_msg);
    }

    // Notify other clients
    {
        Net::S_ClientConnected snd_msg;
        snd_msg.id = sender.id.id();
        snd_msg.name = sender.name;
        sender.broadcast(snd_msg);
    }
}

} // namespace Net64
