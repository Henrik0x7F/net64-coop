//
// Created by henrik on 15.06.20.
// Copyright 2020 Net64 Project
// Licensed under GPLv3
// Refer to the LICENSE file included
//

#include "chat_client.hpp"

#include "net64/client.hpp"
#include "net64/net/messages_client.hpp"


namespace Net64
{
void ChatClient::on_message(const Net::S_ChatMessage& msg, Client& client, Player&)
{
    if(!callback_)
        return;

    callback_(client.remote_players().at(msg.sender).name, msg.message);
}
void ChatClient::set_chat_callback(ChatClient::ChatCallback fn)
{
    callback_ = std::move(fn);
}

void ChatClient::send(Client& client, std::string message)
{
    Net::C_ChatMessage snd_msg;
    snd_msg.message = std::move(message);
    client.send(snd_msg);
}
}
