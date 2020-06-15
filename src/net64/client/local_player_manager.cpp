//
// Created by henrik on 15.06.20.
// Copyright 2020 Net64 Project
// Licensed under GPLv3
// Refer to the LICENSE file included
//

#include "local_player_manager.hpp"

#include "net64/net/messages_client.hpp"
#include "net64/client.hpp"
#include "build_info.hpp"


namespace Net64
{
void LocalPlayerManager::on_connect(Client& client, LocalPlayer&)
{
    Net::C_Handshake snd_msg;

    snd_msg.name = client.username();
    snd_msg.client_version_str = BuildInfo::GIT_DESC;

    client.send(snd_msg);
}

void LocalPlayerManager::on_disconnect(Client&, LocalPlayer& player)
{
    player = {};
}

void LocalPlayerManager::on_message(const Net::S_Handshake& msg, Client&, LocalPlayer& player)
{
    player.id = msg.local_player_id;

    logger()->info("Received handshake from server (player_id={})", player.id);
}

}
