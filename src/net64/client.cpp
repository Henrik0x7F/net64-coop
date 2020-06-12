//
// Created by henrik on 03.09.19
// Copyright 2019 Net64 Coop Project
// Licensed under GPLv3
// Refer to the LICENSE file included
//

#include <net64/net/errors.hpp>

#include "client.hpp"

namespace Net64
{
Client::Client(Memory::MemHandle mem_hdl):
    mem_hdl_{mem_hdl},
    net64_header_{mem_hdl_, mem_hdl_.read<n64_addr_t>(Game::FixedAddr::HEADER_PTR)},
    rcv_queue_{net64_header_->field(&Game::net64_header_t::receive_queue)},
    snd_queue_{net64_header_->field(&Game::net64_header_t::send_queue)},
    host_{enet_host_create(nullptr, 1, Net::channel_count(), 0, 0)}
{
    if(!host_)
        throw std::system_error(make_error_code(Net::Error::ENET_HOST_CREATION));

    if(net64_header_->field(&Game::net64_header_t::compat_version) != Game::CLIENT_COMPAT_VER)
    {
        logger()->error("Incompatible patch version. Patch version: {}, Client version: {}",
                        net64_header_->field(&Game::net64_header_t::compat_version).read(),
                        Game::CLIENT_COMPAT_VER);
        throw std::system_error(make_error_code(Net64::ClientError::INCOMPATIBLE_GAME));
    }

    logger()->info("Initialized Net64 client version {} (Compatibility: {})",
                   net64_header_->field(&Game::net64_header_t::version).read(),
                   net64_header_->field(&Game::net64_header_t::compat_version).read());
}

void Client::set_chat_callback(std::function<void(const std::string&, const std::string&)> fn)
{
}

std::error_code Client::connect(const char* ip, std::uint16_t port)
{
    if(peer_)
        return make_error_code(Net::Error::ALREADY_CONNECTED);

    disconnect_code_ = 0;

    ENetAddress addr;
    // @todo: check if ip is a domain or ip address
    if(enet_address_set_host(&addr, ip) != 0)
    {
        return make_error_code(Net::Error::UNKOWN_HOSTNAME);
    }
    addr.port = port;

    PeerHandle peer{enet_host_connect(host_.get(), &addr, Net::channel_count(), Net::PROTO_VER)};
    if(!peer)
    {
        return make_error_code(Net::Error::ENET_PEER_CREATION);
    }

    ENetEvent evt;
    if(enet_host_service(host_.get(), &evt, Net::CONNECT_TIMEOUT) > 0 && evt.type == ENET_EVENT_TYPE_CONNECT)
    {
        peer_ = std::move(peer);
        on_connect();

        return {};
    }

    return make_error_code(Net::Error::TIMED_OUT);
}

void Client::disconnect()
{
    if(!peer_)
        return;

    enet_peer_disconnect(peer_.get(), 0);

    ENetEvent evt;
    while(enet_host_service(host_.get(), &evt, Net::DISCONNECT_TIMEOUT) > 0)
    {
        switch(evt.type)
        {
        case ENET_EVENT_TYPE_RECEIVE:
            enet_packet_destroy(evt.packet);
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            // ENet already took care of deallocation
            (void)peer_.release();
            return;
        }
    }

    // Force disconnect
    peer_.reset();

    on_disconnect();
}

void Client::tick()
{
    if(peer_)
    {
        for(ENetEvent evt; enet_host_service(host_.get(), &evt, Net::CLIENT_SERVICE_WAIT) > 0;)
        {
            switch(evt.type)
            {
            case ENET_EVENT_TYPE_RECEIVE:
                on_net_message(*evt.packet);
                enet_packet_destroy(evt.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                // ENet already took care of deallocation
                (void)peer_.release();
                disconnect_code_ = evt.data;
                break;
            }
        }
    }

    for(Game::MsgQueue::n64_message_t msg{}; rcv_queue_.poll(msg);)
    {
        on_game_message(msg);
    }
}

bool Client::connected() const
{
    return (peer_ != nullptr);
}

std::uint32_t Client::disconnect_code() const
{
    return disconnect_code_;
}

bool Client::game_initialized(Memory::MemHandle mem_hdl)
{
    return (mem_hdl.read<u32>(Game::FixedAddr::MAGIC_NUMBER) == Game::MAGIC_NUMBER);
}

void Client::on_connect()
{
}

void Client::on_disconnect()
{
}

void Client::on_net_message(const ENetPacket& packet)
{
    std::istringstream strm;

    strm.str({reinterpret_cast<const char*>(packet.data), packet.dataLength});

    std::unique_ptr<INetMessage> msg{INetMessage::parse_message(strm)};

    if(!msg)
        return;
}

void Client::on_game_message(const Game::MsgQueue::n64_message_t& message)
{
    game_logger_.push_message(message);
}

void Client::send(const INetMessage& msg)
{
    if(!peer_)
        return;

    std::ostringstream strm;
    {
        cereal::PortableBinaryOutputArchive ar(strm);

        msg.serialize_msg(ar);
    }

    PacketHandle packet(enet_packet_create(strm.str().data(), strm.str().size(), Net::channel_flags(msg.channel())));

    enet_peer_send(peer_.get(), static_cast<std::uint8_t>(msg.channel()), packet.release());
}

} // namespace Net64

namespace
{
static const struct Net64ClientErrorCategory : std::error_category
{
    const char* name() const noexcept override { return "net64_client_error"; }

    std::string message(int ev) const override
    {
        using namespace Net64;

        switch(static_cast<ClientError>(ev))
        {
        case ClientError::INCOMPATIBLE_GAME:
            return "Game is incompatible with client";
        }

        return "[Unkown Error]";
    }

} net64_client_error_category_l;

} // namespace

namespace Net64
{
std::error_code make_error_code(Net64::ClientError e)
{
    return {static_cast<int>(e), net64_client_error_category_l};
}

} // namespace Net64
