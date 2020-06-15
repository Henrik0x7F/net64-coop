//
// Created by henrik on 15.06.20.
// Copyright 2020 Net64 Project
// Licensed under GPLv3
// Refer to the LICENSE file included
//

#include "player_list_manager.hpp"


namespace Net64
{

PlayerListManager::PlayerListManager(std::unordered_map<Net::player_id_t, RemotePlayer>& list):
    player_list_(&list)
{
}

void PlayerListManager::on_message(const Net::S_ClientConnected& msg, Client&, LocalPlayer&)
{
    RemotePlayer player;
    player.id = msg.id;
    player.name = msg.name;
    player_list_->insert(std::pair(player.id, player));
}

void PlayerListManager::on_message(const Net::S_ClientDisconnected& msg, Client&, LocalPlayer&)
{
    player_list_->erase(msg.player);
}

void PlayerListManager::on_message(const Net::S_PlayerList& msg, Client&, LocalPlayer&)
{
    for(auto& iter : msg.players)
    {
        RemotePlayer player;
        player.id = iter.id;
        player.name = iter.name;
        player_list_->insert(std::pair(iter.id, player));
    }
}

}
