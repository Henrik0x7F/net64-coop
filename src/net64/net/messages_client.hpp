//
// Created by henrik on 05.06.20.
// Copyright 2020 Net64 Project
// Licensed under GPLv3
// Refer to the LICENSE file included
//

#pragma once

#include <cereal/types/string.hpp>

#include "net64/net/net_message.hpp"


namespace Net64::Net
{
struct C_ChatMessage : INetMessage::Derive<C_ChatMessage, NetMessageId::CLIENT_CHAT_MESSAGE>
{
    std::string message;

    NET_SERIALIZE_(message)
};

} // namespace Net64::Net
