//
// Created by henrik on 10.01.20.
// Copyright 2020 Net64 Project
// Licensed under GPLv3
// Refer to the LICENSE file included.
//

#pragma once

#include <chrono>
#include <functional>
#include <future>

#include <QThread>
#include <QTimer>

#include "net64/net64.hpp"
#include "qt_gui/app_settings.hpp"

#if 0
namespace Frontend
{
struct Net64Obj : QObject
{
    Q_OBJECT

public:
    using Clock = std::chrono::steady_clock;

    static constexpr std::chrono::milliseconds CLIENT_INTERV{25};

    explicit Net64Obj(AppSettings& config);
    ~Net64Obj() override;

public slots:
    void set_config(AppSettings* config);

    void start_server(std::uint16_t max_slots, std::uint16_t port);
    void initialize_net64(Net64::Emulator::IEmulator* emu);
    void connect(std::string username, std::string ip, std::uint16_t port);
    void disconnect();
    void destroy_net64();
    void stop_server();

signals:
    void server_started(std::error_code ec);
    void net64_initialized(std::error_code ec);
    void connected(std::error_code ec);
    void chat_message(std::string sender, std::string message);
    void disconnected();
    void net64_destroyed();
    void server_stopped(std::error_code ec);

private:
    void tick();

    QTimer* timer_{new QTimer(this)};
    std::optional<Net64::Client> client_;
    std::optional<Net64::Memory::MemHandle> memory_hdl_;
    AppSettings* settings_;
    std::optional<Net64::Server> server_;

    bool initializing_net64_{false};
    bool stopping_server_{false};
    Clock::time_point stop_server_time_;

    CLASS_LOGGER_("net64-thread")
};

struct Net64Thread : QObject
{
    Q_OBJECT

public:
    explicit Net64Thread(AppSettings& config);
    ~Net64Thread() override;

    void set_config(AppSettings& config);

    bool is_initializing() const;
    bool is_initialized() const;
    bool is_connected() const;
    bool is_server_running() const;

public slots:
    void initialize_net64(Net64::Emulator::IEmulator* emu);
    void start_server(std::uint16_t max_slots, std::uint16_t port);
    void connect(std::string username, std::string ip, std::uint16_t port);
    void disconnect();
    void destroy_net64();
    void stop_server();

signals:
    void net64_initialized(std::error_code ec);
    void server_started(std::error_code ec);
    void connected(std::error_code ec);
    void chat_message(std::string sender, std::string message);
    void disconnected();
    void net64_destroyed();
    void server_stopped(std::error_code ec);

private slots:
    void o_server_started(std::error_code ec);
    void o_net64_initialized(std::error_code ec);
    void o_connected(std::error_code ec);
    void o_chat_message(std::string sender, std::string message);
    void o_disconnected();
    void o_net64_destroyed();
    void o_server_stopped(std::error_code ec);

signals:
    void s_set_config(AppSettings*);
    void s_start_server(std::uint16_t max_slots, std::uint16_t port);
    void s_initialize_net64(Net64::Emulator::IEmulator*);
    void s_connect(std::string, std::string, std::uint16_t);
    void s_disconnect();
    void s_destroy_net64();
    void s_stop_server();

private:
    QThread thread_;

    bool is_initializing_{};
    bool is_initialized_{};
    bool is_connected_{};
    bool is_server_running_{};
};

} // namespace Frontend
#endif
