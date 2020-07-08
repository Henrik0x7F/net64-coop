//
// Created by henrik on 03.09.19
// Copyright 2019 Net64 Coop Project
// Licensed under GPLv3
// Refer to the LICENSE file included
//

#include "net64/net/errors.hpp"

#include "client.hpp"

#include "net64/client/game_logger.hpp"
#include "net64/client/local_player_manager.hpp"
#include "net64/client/player_list_manager.hpp"


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

    // Components
    chat_client_ = new ChatClient;
    add_component(chat_client_);
    add_component(new Game::Logger);
    add_component(new LocalPlayerManager);
    add_component(new PlayerListManager);
    add_component(new ChatClient);
}

Client::~Client()
{
    net_message_handlers_.clear();
    connection_event_handlers_.clear();
    tick_handlers_.clear();
    game_message_handlers_.clear();

    for(auto& iter : components_)
    {
        iter.deleter(iter.component);
    }
}

void Client::set_chat_callback(ChatCallback fn)
{
    chat_client_->set_chat_callback(fn);
}

void Client::connect(const char* ip, std::uint16_t port, std::string username, std::chrono::seconds timeout, ConnectCallback callback)
{
    assert(!peer_);

    connect_callback_ = std::move(callback);
    connect_timeout_ = timeout;
    username_ = std::move(username);

    ENetAddress addr;
    // @todo: check if ip is a domain or ip address
    if(enet_address_set_host(&addr, ip) != 0)
    {
        connect_callback_(make_error_code(Net::Error::UNKOWN_HOSTNAME));
        return;
    }
    addr.port = port;

    PeerHandle peer(enet_host_connect(host_.get(), &addr, Net::channel_count(), Net::PROTO_VER));
    if(!peer)
    {
        connect_callback_(make_error_code(Net::Error::ENET_PEER_CREATION));
        return;
    }

    connection_start_time_ = Clock::now();
    peer_ = std::move(peer);
}

void Client::disconnect(std::chrono::seconds timeout, DisconnectCallback callback)
{
    if(!connected())
        return;

    disconnect_callback_ = std::move(callback);
    disconnect_timeout_ = timeout;

    enet_peer_disconnect(peer_.get(), static_cast<std::uint32_t>(Net::C_DisconnectCode::USER_DISCONNECT));
    disconnection_start_time_ = Clock::now();
}

void Client::abort_connect()
{
    if(!connecting())
        return;

    peer_.reset();
    connect_callback_(make_error_code(Net::Error::TIMED_OUT));
}

void Client::tick()
{
    if(Clock::now() - connection_start_time_ > connect_timeout_)
    {
        // Connection timed out
        abort_connect();
    }
    if(disconnecting() && (Clock::now() - disconnection_start_time_) > disconnect_timeout_)
    {
        // Disconnecting timed out
        on_disconnect();
        peer_.reset();
        disconnect_callback_({});
    }

    for(ENetEvent evt; enet_host_service(host_.get(), &evt, Net::CLIENT_SERVICE_WAIT) > 0;)
    {
        switch(evt.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
            connect_callback_({});
            on_connect();
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            disconnect_callback_({});
            on_disconnect();
            // ENet already took care of deallocation
            (void)peer_.release();
            break;
        case ENET_EVENT_TYPE_RECEIVE: {
            PacketHandle packet(evt.packet);
            on_net_message(*packet);
            break;
        }
        }
    }

    for(Game::MsgQueue::n64_message_t msg{}; rcv_queue_.poll(msg);)
    {
        on_game_message(msg);
    }
}

bool Client::connecting() const
{
    return (peer_ != nullptr && !connected());
}

bool Client::connected() const
{
    return (peer_ != nullptr && peer_->state == ENET_PEER_STATE_CONNECTED);
}

bool Client::disconnecting() const
{
    return (peer_ != nullptr && peer_->state == ENET_PEER_STATE_DISCONNECTING);
}

bool Client::disconnected() const
{
    return !peer_;
}

bool Client::game_initialized(Memory::MemHandle mem_hdl)
{
    return (mem_hdl.read<u32>(Game::FixedAddr::MAGIC_NUMBER) == Game::MAGIC_NUMBER);
}

void Client::on_connect()
{
    for(auto iter : connection_event_handlers_)
    {
        iter->on_connect(*this);
    }
}

void Client::on_disconnect()
{
    username_.clear();

    for(auto iter : connection_event_handlers_)
    {
        iter->on_disconnect(*this);
    }
}

void Client::on_net_message(const ENetPacket& packet)
{
    std::istringstream strm;

    strm.str({reinterpret_cast<const char*>(packet.data), packet.dataLength});
    try
    {
        std::unique_ptr<INetMessage> msg{INetMessage::parse_message(strm)};

        for(auto handler : net_message_handlers_)
        {
            handler->handle_message(*msg, *this);
        }
    }
    catch(const std::exception& e)
    {
        logger()->warn("Failed to parse network message: {}", e.what());
    }
}

void Client::on_game_message(const Game::MsgQueue::n64_message_t& message)
{
    for(auto iter : game_message_handlers_)
    {
        iter->push_message(*this, message);
    }
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

ClientSharedData Client::get_client_shared_data(Badge<ClientDataAccess>)
{
    return ClientSharedData{mem_hdl_, snd_queue_, player_, remote_players_};
}

std::string Client::username() const
{
    return username_;
}

const std::unordered_map<Net::player_id_t, RemotePlayer>& Client::remote_players() const
{
    return remote_players_;
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
