//
// Created by henrik on 05.06.20.
// Copyright 2020 Net64 Project
// Licensed under GPLv3
// Refer to the LICENSE file included
//

#pragma once

#include "net64/net/net_message.hpp"


namespace Net64::Net
{
struct S_ChatMessage : INetMessage::Derive<S_ChatMessage, NetMessageId::SERVER_CHAT_MESSAGE>
{
    std::string sender, message;

    NET_SERIALIZE_(sender, message)
};

} // namespace Net64::Net
