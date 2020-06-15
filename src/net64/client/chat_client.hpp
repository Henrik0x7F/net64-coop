//
// Created by henrik on 15.06.20.
// Copyright 2020 Net64 Project
// Licensed under GPLv3
// Refer to the LICENSE file included
//

#pragma once

#include "net64/logging.hpp"
#include "net64/net/messages_server.hpp"


namespace Net64
{
struct ChatClient : ClientMessageHandler::Derive<ChatClient>::Receive<Net::S_ChatMessage>
{
    using ChatCallback = std::function<void(const std::string& sender, const std::string& msg)>;

    void set_chat_callback(ChatCallback fn);

    void send(Client& client, std::string message);

    void on_message(const Net::S_ChatMessage& msg, Client& client, Player& player);

private:
    ChatCallback callback_;
};

}
