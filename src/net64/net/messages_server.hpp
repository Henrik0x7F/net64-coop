//
// Created by henrik on 05.06.20.
// Copyright 2020 Net64 Project
// Licensed under GPLv3
// Refer to the LICENSE file included
//

#pragma once

#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

#include "net64/net/net_message.hpp"
#include "net64/net/protocol.hpp"


namespace Net64::Net
{
NET_MESSAGE_(S_ChatMessage, NetMessageId::SERVER_CHAT_MESSAGE, Channel::META)
{
    std::string message;
    player_id_t sender;

    NET_SERIALIZE_(sender, message)
};

NET_MESSAGE_(S_Handshake, NetMessageId::SERVER_HANDSHAKE, Channel::META)
{
    player_id_t local_player_id{};

    NET_SERIALIZE_(local_player_id)
};

NET_MESSAGE_(S_PlayerList, NetMessageId::SERVER_PLAYER_LIST, Channel::META)
{
    struct Player
    {
        player_id_t id;
        std::string name;

        NET_SERIALIZE_(id, name)
    };
    std::vector<Player> players;

    NET_SERIALIZE_(players)
};

} // namespace Net64::Net
