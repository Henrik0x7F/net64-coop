//
// Created by henrik on 12.07.19
// Copyright 2019 Net64 Coop Project
// Licensed under GPLv3
// Refer to the LICENSE file included
//

#pragma once

#include "net64/client/msg_queue.hpp"
#include "types.hpp"


namespace Net64::Game
{
struct Message
{
    enum : MsgQueue::message_type_t
    {
        CUSTOM = 0,
        LOG_APPEND, ///< Append text to next log message
        LOG_END,    ///< Append text to log message and print it

        COUNT
    };
};

} // namespace Net64::Game
