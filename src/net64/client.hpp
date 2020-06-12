//
// Created by henrik on 03.09.19
// Copyright 2019 Net64 Coop Project
// Licensed under GPLv3
// Refer to the LICENSE file included
//

#pragma once

#include <memory>

#include <enet/enet.h>

#include "common/resource_handle.hpp"
#include "net/net_message.hpp"
#include "net64/game/game_logger.hpp"
#include "net64/game/msg_queue.hpp"
#include "net64/game/net64_header.hpp"
#include "net64/memory/pointer.hpp"
#include "net64/net/errors.hpp"
#include "net64/net/messages_server.hpp"
#include "net64/net/protocol.hpp"
#include "types.hpp"


namespace Net64
{
enum struct ClientError
{
    INCOMPATIBLE_GAME = 1
};

std::error_code make_error_code(Net64::ClientError e);

} // namespace Net64

namespace std
{
template<>
struct is_error_code_enum<Net64::ClientError> : std::true_type
{
};

} // namespace std

namespace Net64
{
struct Client
{
    using HostHandle = ResourceHandle<&enet_host_destroy>;
    using PeerHandle = ResourceHandle<&enet_peer_reset>;
    using PacketHandle = ResourceHandle<&enet_packet_destroy>;

    using ChatCallback = std::function<void(const std::string& sender, const std::string& msg)>;

    explicit Client(Memory::MemHandle mem_hdl);

    void set_chat_callback(ChatCallback fn);

    std::error_code connect(const char* ip, std::uint16_t port);
    void disconnect();

    void send(const INetMessage& msg);

    void tick();

    [[nodiscard]] bool connected() const;

    [[nodiscard]] std::uint32_t disconnect_code() const;

    static bool game_initialized(Memory::MemHandle mem_hdl);

private:
    void on_connect();
    void on_disconnect();
    void on_net_message(const ENetPacket& packet);
    void on_game_message(const Game::MsgQueue::n64_message_t& message);

    Memory::MemHandle mem_hdl_;
    Memory::Ptr<Game::net64_header_t> net64_header_;
    Game::MsgQueue::Receiver rcv_queue_;
    Game::MsgQueue::Sender snd_queue_;

    HostHandle host_;
    PeerHandle peer_;
    std::uint32_t disconnect_code_{};
    ChatCallback chat_callback_;

    Game::GameLogger game_logger_;

    CLASS_LOGGER_("client")
};

} // namespace Net64
