//
// Created by henrik on 10.01.20.
// Copyright 2020 Net64 Project
// Licensed under GPLv3
// Refer to the LICENSE file included.
//

#include "qt_gui/net64_thread.hpp"
#if 0

static std::string error_msg(std::error_code ec)
{
    return "[" + std::string(ec.category().name()) + ':' + std::to_string(ec.value()) + ']' + ec.message();
}

static std::string error_msg(const std::system_error& e)
{
    return e.what() + std::string(" (") + error_msg(e.code()) + ")";
}

namespace Frontend
{
Net64Obj::Net64Obj(AppSettings& config): settings_{&config}
{
    timer_->setTimerType(Qt::PreciseTimer);
    timer_->setInterval(CLIENT_INTERV.count());
    QObject::connect(timer_, &QTimer::timeout, this, &Net64Obj::tick);
    timer_->start();
}

Net64Obj::~Net64Obj()
{
}

void Net64Obj::set_config(AppSettings* config)
{
    settings_ = config;
}

void Net64Obj::initialize_net64(Net64::Emulator::IEmulator* emu)
{
    memory_hdl_ = Net64::Memory::MemHandle(*emu);
    initializing_net64_ = true;
}

void Net64Obj::connect(std::string username, std::string ip, std::uint16_t port)
{
    assert(client_.has_value() && client_->disconnected());

    client_->connect(ip.c_str(), port, std::move(username), std::chrono::seconds(5), [this](auto ec)
    {
                         connected(ec);
    },
    [this](auto)
    {
            disconnected();
    });
}

void Net64Obj::disconnect()
{
    assert(client_.has_value());

    if(client_->connecting())
    {
        //client_->abort_connect();
    }
    else if(client_->disconnected())
    {
        disconnected();
    }
    else if(client_->connected())
    {
        client_->disconnect(std::chrono::seconds(5));
    }
}

void Net64Obj::destroy_net64()
{
    client_ = {};
    memory_hdl_ = {};
    if(initializing_net64_)
    {
        initializing_net64_ = false;
        net64_initialized(make_error_code(std::errc::timed_out));
    }
    net64_destroyed();
}

void Net64Obj::tick()
{
    if(initializing_net64_)
    {
        //if(Net64::Client::game_initialized(*memory_hdl_))
        {
            std::error_code ec;
            try
            {
                client_ = Net64::Client(*memory_hdl_);
                client_->set_chat_callback([this](auto sender, auto msg)
                                           {
                                               chat_message(std::move(sender), std::move(msg));
                                           });
            }
            catch(const std::system_error& e)
            {
                logger()->error("Error while starting Net64 client: {}", error_msg(e));
                ec = e.code();
            }
            catch(const std::exception& e)
            {
                logger()->error("Error while starting Net64 client: {}", e.what());
                ec = make_error_code(Net64::ErrorCode::UNKNOWN);
            }
            initializing_net64_ = false;
            net64_initialized(ec);
        }
    }
    if(client_.has_value())
    {
        client_->tick();
    }
    if(server_.has_value())
    {
        server_->tick();

        if(stopping_server_ && (Clock::now() - stop_server_time_) > std::chrono::seconds(3))
        {
            stopping_server_ = false;
            server_ = {};
            server_stopped({});
        }
    }
}

void Net64Obj::start_server(std::uint16_t max_slots, std::uint16_t port)
{
    assert(!server_.has_value());

    try
    {
        server_ = Net64::Server(port, max_slots);
    }
    catch(const std::system_error& e)
    {
        server_started(e.code());
    }
    catch(...)
    {
        server_started(make_error_code(Net64::ErrorCode::UNKNOWN));
    }

    server_started({});
}

void Net64Obj::stop_server()
{
    if(server_.has_value())
    {
        server_->accept_new(false);
        server_->disconnect_all(Net64::Net::S_DisconnectCode::SERVER_SHUTDOWN);

        stop_server_time_ = Clock::now();
        stopping_server_ = true;
    }
    else
    {
        server_stopped({});
    }
}


Net64Thread::Net64Thread(AppSettings& config)
{
    qRegisterMetaType<std::error_code>("std::error_code");
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<std::uint16_t>("std::uint16_t");
    qRegisterMetaType<std::vector<std::byte>>("std::vector<std::byte>");

    auto* obj{new Net64Obj(config)};

    obj->moveToThread(&thread_);

    QObject::connect(&thread_, &QThread::finished, obj, &QObject::deleteLater);
    QObject::connect(this, &Net64Thread::s_set_config, obj, &Net64Obj::set_config);
    QObject::connect(this, &Net64Thread::s_start_server, obj, &Net64Obj::start_server);
    QObject::connect(this, &Net64Thread::s_initialize_net64, obj, &Net64Obj::initialize_net64);
    QObject::connect(this, &Net64Thread::s_connect, obj, &Net64Obj::connect);
    QObject::connect(this, &Net64Thread::s_disconnect, obj, &Net64Obj::disconnect);
    QObject::connect(this, &Net64Thread::s_destroy_net64, obj, &Net64Obj::destroy_net64);
    QObject::connect(this, &Net64Thread::s_stop_server, obj, &Net64Obj::stop_server);

    QObject::connect(obj, &Net64Obj::server_started, this, &Net64Thread::o_server_started);
    QObject::connect(obj, &Net64Obj::net64_initialized, this, &Net64Thread::o_net64_initialized);
    QObject::connect(obj, &Net64Obj::connected, this, &Net64Thread::o_connected);
    QObject::connect(obj, &Net64Obj::disconnected, this, &Net64Thread::o_disconnected);
    QObject::connect(obj, &Net64Obj::net64_destroyed, this, &Net64Thread::o_net64_destroyed);
    QObject::connect(obj, &Net64Obj::chat_message, this, &Net64Thread::o_chat_message);
    QObject::connect(obj, &Net64Obj::server_stopped, this, &Net64Thread::o_server_stopped);

    thread_.start();
}

Net64Thread::~Net64Thread()
{
    thread_.quit();
    thread_.wait();
}

void Net64Thread::set_config(AppSettings& config)
{
    s_set_config(&config);
}

bool Net64Thread::is_initializing() const
{
    return is_initializing_;
}

bool Net64Thread::is_initialized() const
{
    return is_initialized_;
}

bool Net64Thread::is_connected() const
{
    return is_connected_;
}

void Net64Thread::initialize_net64(Net64::Emulator::IEmulator* emu)
{
    assert(emu);

    is_initializing_ = true;
    s_initialize_net64(emu);
}

void Net64Thread::connect(std::string username, std::string ip, std::uint16_t port)
{
    s_connect(std::move(username), std::move(ip), port);
}

void Net64Thread::disconnect()
{
    s_disconnect();
}

void Net64Thread::destroy_net64()
{
    s_destroy_net64();
}

void Net64Thread::o_net64_initialized(std::error_code ec)
{
    if(!ec)
        is_initialized_ = true;

    is_initializing_ = false;
    net64_initialized(ec);
}

void Net64Thread::o_connected(std::error_code ec)
{
    if(!ec)
        is_connected_ = true;

    connected(ec);
}

void Net64Thread::o_chat_message(std::string sender, std::string message)
{
    chat_message(std::move(sender), std::move(message));
}

void Net64Thread::o_disconnected()
{
    is_connected_ = false;

    disconnected();
}

void Net64Thread::o_net64_destroyed()
{
    is_initialized_ = false;
    is_initializing_ = false;

    net64_destroyed();
}

bool Net64Thread::is_server_running() const
{
    return is_server_running_;
}

void Net64Thread::start_server(std::uint16_t max_slots, std::uint16_t port)
{
    s_start_server(max_slots, port);
}

void Net64Thread::stop_server()
{
    s_stop_server();
}

void Net64Thread::o_server_started(std::error_code ec)
{
    is_server_running_ = !static_cast<bool>(ec);
    server_started(ec);
}

void Net64Thread::o_server_stopped(std::error_code ec)
{
    is_server_running_ = false;
    server_stopped(ec);
}

} // namespace Frontend

#endif
