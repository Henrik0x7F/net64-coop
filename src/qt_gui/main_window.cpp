﻿#include <fstream>
#include <string>

#include <QDesktopServices>
#include <QMessageBox>

#include "main_window.hpp"
#include "ui_main_window.h"


namespace Frontend
{
using namespace std::string_literals;

static QString format_error_msg(std::error_code ec)
{
    return "[" + QString::fromStdString(ec.category().name()) + ":" +
           QString::fromStdString(std::to_string(ec.value())) + "] " + QString::fromStdString(ec.message());
}

static void error_popup(const char* action, const QString& reason)
{
    QMessageBox box;
    box.setIcon(QMessageBox::Critical);
    box.setWindowTitle(action);
    box.setText(reason);
    box.exec();
}

MainWindow::MainWindow(AppSettings& settings, QWidget* parent):
    QMainWindow(parent), ui(new Ui::MainWindow), settings_(&settings) //, net64_thread_(*settings_)
{
    qRegisterMetaType<Net64::Emulator::State>("Net64::Emulator::State");

    ui->setupUi(this);
    setWindowIcon(QIcon{":/icons/net64-icon128.png"});
    setFixedSize(size());
    ui->statusbar->setSizeGripEnabled(false);
    ui->statusbar->addWidget(statustext_ = new QLabel);
    update_statustext();

    set_page(Page::SETUP);


    emu_settings_win_ = new EmulatorSettings(*settings_, this);

    setup_menus();
    setup_signals();

    setWindowTitle(QCoreApplication::applicationName() + " " + QCoreApplication::applicationVersion());

    client_.set_chat_callback([this](auto a, auto b) { on_chat_message(std::move(a), std::move(b)); });

    // Start SDL event handling
    sdl_event_timer_ = new QTimer(this);
    connect(sdl_event_timer_, &QTimer::timeout, this, &MainWindow::on_handle_sdl_events);
    sdl_event_timer_->start(30);

    // temp @todo: Put these on a startup thread
    create_emulator();

    std::ifstream rom_file(settings_->rom_file_path.string(), std::ios::ate | std::ios::binary);
    if(!rom_file)
        return;

    rom_image_.resize(static_cast<std::size_t>(rom_file.tellg()));
    rom_file.seekg(0);
    rom_file.read(reinterpret_cast<char*>(rom_image_.data()), static_cast<long>(rom_image_.size()));
    rom_file.close();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::reload_emulator()
{
    if(emulator_ && emulator_->state() != Net64::Emulator::State::STOPPED)
    {
        reload_emulator_after_stop_ = true;
        return;
    }

    reload_emulator_after_stop_ = false;
    destroy_emulator();
    create_emulator();
}

void MainWindow::on_handle_sdl_events()
{
    SDL_EventHandler::poll_events();

    client_.tick();

    if(server_.has_value())
        server_->tick();
}

void MainWindow::on_emulator_settings()
{
}

void MainWindow::on_connect_btn_pressed()
{
    ui->btn_connect_ip->setDisabled(true);

    if(ui->cbx_host->isChecked())
    {
        // Start server
        logger()->debug("Starting Net64 server");

        try
        {
            server_ =
                Net64::Server((std::uint16_t)ui->sbx_host_port->value(), (std::uint16_t)ui->sbx_max_players->value());
        }
        catch(const std::system_error& e)
        {
            logger()->error("Failed to start server:  {}", format_error_msg(e.code()).toStdString());
            error_popup("Failed to start server", format_error_msg(e.code()));
            ui->btn_connect_ip->setDisabled(false);
            return;
        }
    }

    connect_net64();
}

void MainWindow::on_disconnect_btn_pressed()
{
}

void MainWindow::on_stop_server_btn_pressed()
{
    if(server_.has_value())
    {
        server_->accept_new(false);
        server_->disconnect_all(Net64::Net::S_DisconnectCode::SERVER_SHUTDOWN);
    }
    else
    {
        client_.disconnect(std::chrono::seconds(3));
    }
}

void MainWindow::on_host_server_checked(int state)
{
    bool c{state == Qt::Checked};

    ui->grp_server_settings->setEnabled(c);
    ui->tbx_join_ip->setDisabled(c);
    ui->sbx_join_port->setDisabled(c);
    ui->tbx_join_ip->setText(c ? "localhost" : "");
    ui->btn_connect_ip->setText(c ? "Host" : "Join");
    ui->btn_stop->setText(c ? "Stop server" : "Disconnect");
}

void MainWindow::on_host_port_changed(int port)
{
    ui->sbx_join_port->setValue(port);
}

void MainWindow::on_send_chat_message()
{
    client_.send_chat_message(ui->tbx_chat_input->text().toStdString());
    ui->tbx_chat_input->clear();
}

void MainWindow::on_emulator_state(Net64::Emulator::State state)
{
    auto old_state{emu_state_.load()};
    emu_state_ = state;

    switch(state)
    {
    case Net64::Emulator::State::RUNNING:
        if(old_state == Net64::Emulator::State::PAUSED)
            emulator_unpaused();
        else
            emulator_started();
        break;
    case Net64::Emulator::State::PAUSED:
        emulator_paused();
        break;
    case Net64::Emulator::State::JOINABLE:
        emulator_joinable();
        break;
    }
}

void MainWindow::on_emulator_started()
{
    update_statustext();
}

void MainWindow::on_emulator_paused()
{
}

void MainWindow::on_emulator_unpaused()
{
}

void MainWindow::on_emulator_joinable()
{
    if(client_.hooked())
    {
        client_.unhook();
    }
    if(server_.has_value())
    {
        server_ = {};
    }

    ui->btn_connect_ip->setEnabled(true);

    std::error_code ec;
    emulator_->join(ec);
    emulator_->unload_rom();

    if(ec)
        error_popup("Emulation halted due to an error", format_error_msg(ec));

    if(reload_emulator_after_stop_)
        reload_emulator();

    update_statustext();
}

void MainWindow::on_hooked(std::error_code)
{
    update_statustext();
}

void MainWindow::on_chat_message(std::string sender, std::string message)
{
    if(sender.empty())
    {
        // Server message
        ui->tbx_chat_history->appendPlainText(QString::fromStdString("[Server] " + message));
    }
    else
    {
        ui->tbx_chat_history->appendPlainText(QString::fromStdString('<' + sender + "> " + message));
    }
}

void MainWindow::on_connected(std::error_code ec)
{
    update_statustext();

    if(ec)
    {
        ui->btn_connect_ip->setEnabled(true);
        error_popup("Failed to connect", format_error_msg(ec));
        return;
    }

    set_page(Page::IN_GAME);
}

void MainWindow::on_disconnected(std::error_code ec)
{
    update_statustext();
    ui->tbx_chat_history->clear();

    if(ec)
    {
        error_popup("Connection lost", format_error_msg(ec));
    }

    set_page(Page::SETUP);
    ui->btn_connect_ip->setEnabled(true);

    if(server_.has_value())
    {
        // Local server, destroy it
        server_ = {};
    }
}

void MainWindow::setup_menus()
{
    auto settings_menu{ui->menubar->addMenu("Settings")};
    auto info_menu{ui->menubar->addMenu("Info")};

    connect(settings_menu->addAction("Multiplayer"), &QAction::triggered, this, [this]() {
        show_window(multiplayer_cfg_win_, *settings_);
    });
    connect(settings_menu->addAction("Emulator"), &QAction::triggered, this, [this]() {
        show_window(emu_settings_win_, *settings_);
    });

    connect(info_menu->addAction("Website"), &QAction::triggered, this, []() {
        QDesktopServices::openUrl(QUrl("https://net64-mod.github.io"));
    });
    connect(info_menu->addAction("GitHub"), &QAction::triggered, this, []() {
        QDesktopServices::openUrl(QUrl("https://github.com/net64-mod/net64-coop"));
    });
    connect(info_menu->addAction("Discord"), &QAction::triggered, this, []() {
        QDesktopServices::openUrl(QUrl("https://discord.gg/aUmWKaw"));
    });
    connect(info_menu->addAction("About Net64"), &QAction::triggered, this, []() {

    });
    connect(info_menu->addAction("About Qt"), &QAction::triggered, this, &QApplication::aboutQt);
}

void MainWindow::setup_signals()
{
    connect(this, &MainWindow::emulator_object_changed, emu_settings_win_, &EmulatorSettings::set_emulator_object);
    connect(emu_settings_win_, &EmulatorSettings::reload_emulator, this, &MainWindow::reload_emulator);

    connect(this, &MainWindow::emulator_started, this, &MainWindow::on_emulator_started);
    connect(this, &MainWindow::emulator_paused, this, &MainWindow::on_emulator_paused);
    connect(this, &MainWindow::emulator_unpaused, this, &MainWindow::on_emulator_unpaused);
    connect(this, &MainWindow::emulator_joinable, this, &MainWindow::on_emulator_joinable);
    connect(ui->cbx_host, &QCheckBox::stateChanged, this, &MainWindow::on_host_server_checked);
    connect(ui->sbx_host_port, qOverload<int>(&QSpinBox::valueChanged), this, &MainWindow::on_host_port_changed);

    connect(ui->btn_connect_ip, &QPushButton::clicked, this, &MainWindow::on_connect_btn_pressed);
    connect(ui->tbx_join_ip, &QLineEdit::returnPressed, this, &MainWindow::on_connect_btn_pressed);
    connect(ui->btn_stop, &QPushButton::clicked, this, &MainWindow::on_stop_server_btn_pressed);
    connect(ui->btn_send_chat, &QPushButton::clicked, this, &MainWindow::on_send_chat_message);
    connect(ui->tbx_chat_input, &QLineEdit::returnPressed, this, &MainWindow::on_send_chat_message);
}

void MainWindow::set_page(Page page)
{
    last_page_ = static_cast<Page>(ui->stackedWidget->currentIndex());
    ui->stackedWidget->setCurrentIndex(static_cast<int>(page));
}

bool MainWindow::create_emulator()
{
    if(emulator_)
        return true;

    try
    {
        auto emu{std::make_unique<Net64::Emulator::Mupen64Plus>(
            (settings_->m64p_plugin_dir() / settings_->m64p_core_plugin).string(),
            settings_->m64p_dir().string(),
            settings_->m64p_plugin_dir().string())};

        emulator_ = std::move(emu);
    }
    catch(const std::system_error& e)
    {
        error_popup("Failed to construct emulator", format_error_msg(e.code()));
        return false;
    }

    emulator_object_changed(emulator_.get());
    return true;
}

void MainWindow::start_emulation()
{
    if(!create_emulator())
        return;

    try
    {
        auto add_plugin{
            [this](const std::string& str) { emulator_->add_plugin((settings_->m64p_plugin_dir() / str).string()); }};

        add_plugin(settings_->m64p_video_plugin);
        add_plugin(settings_->m64p_audio_plugin);
        add_plugin(settings_->m64p_rsp_plugin);
        add_plugin(settings_->m64p_input_plugin);

        emulator_->load_rom(reinterpret_cast<void*>(rom_image_.data()), rom_image_.size());

        emulator_->start([this](auto state) { on_emulator_state(state); });
    }
    catch(const std::system_error& e)
    {
        error_popup("Failed to start emulation", format_error_msg(e.code()));
        emulator_.reset();
    }
}

void MainWindow::stop_emulation()
{
    if(emulator_)
        emulator_->stop();
}

void MainWindow::destroy_emulator()
{
    if(emulator_ && emulator_->state() == Net64::Emulator::State::STOPPED)
    {
        emulator_.reset();
        emulator_object_changed(emulator_.get());
    }
}

void MainWindow::connect_net64()
{
    // Start emulation
    if(!emulator_ || emulator_->state() == Net64::Emulator::State::STOPPED)
    {
        connect_once(this, &MainWindow::emulator_started, [this] { connect_net64(); });
        start_emulation();
        return;
    }
    // Hook
    if(!client_.hooked())
    {
        // Net64 not yet initialized
        client_.hook(Net64::Memory::MemHandle(*emulator_), [this](auto ec) {
            if(ec)
            {
                error_popup("Failed to initialize client", format_error_msg(ec));
                return;
            }
            else
            {
                connect_net64();
            }
            on_hooked(ec);
        });
        return;
    }
    // Connect to server
    if(client_.disconnected())
    {
        update_statustext();
        client_.connect(
            ui->tbx_join_ip->text().toStdString().c_str(),
            (std::uint16_t)ui->sbx_join_port->value(),
            settings_->username,
            std::chrono::seconds(5),
            [this](auto ec) {
                // On connect
                on_connected(ec);
            },
            [this](auto ec) {
                // On disconnect
                on_disconnected(ec);
            });
        return;
    }

    // Connected
    logger()->info("Connected");
}

void MainWindow::update_statustext()
{
    if(client_.connected())
    {
        statustext_->setText("Connected");
    }
    else if(client_.connecting())
    {
        statustext_->setText("Connecting...");
    }
    else if(client_.disconnecting())
    {
        statustext_->setText("Disconnecting...");
    }
    else if(client_.hooked())
    {
        statustext_->setText("Hooked");
    }
    else if(emulator_ && emulator_->state() == Net64::Emulator::State::RUNNING)
    {
        statustext_->setText("Hooking...");
    }
    else
    {
        statustext_->setText("Ready");
    }
}

} // namespace Frontend
