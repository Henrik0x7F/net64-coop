//
// Created by henrik on 05.06.20.
// Copyright 2020 Net64 Project
// Licensed under GPLv3
// Refer to the LICENSE file included
//

#pragma once

#include <cstdint>
#include <memory>

#include <cereal/archives/portable_binary.hpp>

#include "common/automatic_factory.hpp"


/**
 * Interface template for message objects
 * @tparam Identifier Type used to distinguish different message types
 */
template<typename Identifier>
struct IMessage : Factory<IMessage<Identifier>, Identifier, IMessage<Identifier>*>
{
    /// Type used to distinguish different message types
    using id_t = Identifier;

    virtual ~IMessage() = default;

    /// Serialize message into a byte stream
    virtual void serialize_msg(cereal::PortableBinaryOutputArchive& ar) = 0;

    /// Parse byte stream and construct message object
    static IMessage<Identifier>* parse_message(std::istream& strm)
    {
        id_t message_id{};
        std::unique_ptr<IMessage<Identifier>> message;

        try
        {
            cereal::PortableBinaryInputArchive ar(strm);

            // Read message type
            ar(message_id);

            // Construct message object
            message.reset(make(message_id));

            // Parse message
            message->parse_msg(ar);
        }
        catch(const std::exception& e)
        {
            return nullptr;
        }

        message.release();
    }

    /// Return message id
    [[nodiscard]] id_t id() const { return id_; }

    /// Base for all message objects
    template<typename Derived, id_t MESSAGE_ID>
    struct Derive : IMessage<Identifier>::template RegisterConst<Derived, MESSAGE_ID>
    {
        static constexpr IMessage<Identifier>::id_t ID{MESSAGE_ID};

        using Base = typename IMessage<Identifier>::template RegisterConst<Derived, ID>;


        /// Implement serialization
        void serialize_msg(cereal::PortableBinaryOutputArchive& ar) final
        {
            ar(MESSAGE_ID, static_cast<Derived&>(*this));
        }

        /// Implement parsing
        void parse_msg(cereal::PortableBinaryInputArchive& ar) final
        {
            // Don't read message id. Base already took care of that
            ar(static_cast<Derived&>(*this));
        }

    protected:
        Derive(): Base(ID) {}
    };

    template<typename T, id_t>
    friend struct Derive;

private:
    /// Parse message data from byte stream (without message id)
    virtual void parse_msg(cereal::PortableBinaryInputArchive& ar) = 0;

    explicit IMessage(id_t id): id_{id} {}

    id_t id_;
};
