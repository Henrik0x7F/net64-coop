//
// Created by henrik on 05.06.20.
// Copyright 2020 Net64 Project
// Licensed under GPLv3
// Refer to the LICENSE file included
//

#pragma once

#include "common/message_interface.hpp"
#include "net64/net/message_ids.hpp"


#define NET_SERIALIZE_(...) \
    template<typename T>    \
    void serialize(T& a)    \
    {                       \
        a(__VA_ARGS__);     \
    }

namespace Net64
{
using INetMessage = IMessage<NetMessageId>;

};
