//
// Created by henrik on 05.06.20.
// Copyright 2020 Net64 Project
// Licensed under GPLv3
// Refer to the LICENSE file included
//

#pragma once

#include "net64/logging.hpp"
#include "net64/net/messages_client.hpp"

namespace Net64
{
struct PlayerManager : INetMessageHandler::Derive<PlayerManager>::Receive<Net::C_Handshake>
{
    void on_message(const Net::C_Handshake& msg, ENetPeer& sender);

    CLASS_LOGGER_("server")
};

} // namespace Net64
