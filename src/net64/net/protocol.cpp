//
// Created by henrik on 04.10.19
// Copyright 2019 Net64 Coop Project
// Licensed under GPLv3
// Refer to the LICENSE file included
//

#include "net64/net/protocol.hpp"


namespace
{
const struct Net64ServerDisconnectCodeCategory : std::error_category
{
    const char* name() const noexcept override { return "net64_server_disconnect_code"; }

    std::string message(int ev) const override
    {
        using namespace Net64::Net;

        switch(static_cast<S_DisconnectCode>(ev))
        {
        case S_DisconnectCode::INCOMPATIBLE:
            return "Server runs an incompatible version";
        case S_DisconnectCode::NOT_ACCEPTED:
            return "Server does not accept new connections";
        case S_DisconnectCode::BANNED:
            return "You are banned from this server";
        case S_DisconnectCode::KICKED:
            return "You have been kicked from this server";
        case S_DisconnectCode::SERVER_SHUTDOWN:
            return "Server is shutting down";
        }

        return "[Unkown Error]";
    }

} g_s_disconnect_code_category;

} // namespace

namespace Net64::Net
{
std::error_code make_error_code(Net64::Net::S_DisconnectCode e)
{
    return {static_cast<int>(e), g_s_disconnect_code_category};
}

std::uint8_t channel_count()
{
    return static_cast<std::uint8_t>(Channel::COUNT);
}
std::uint32_t channel_flags(Channel channel)
{
    return CHANNEL_FLAGS[static_cast<std::size_t>(channel)];
}

} // namespace Net64::Net
