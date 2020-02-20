//
// Created by henrik on 20.02.20.
// Copyright 2020 Net64 Coop Project
// Licensed under GPLv3
// Refer to the LICENSE file included.
//

#pragma once

#include <cstdint>
#include <memory>
#include "common/byte_stream.hpp"
#include "common/generic_factory.hpp"


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

    /// Size of message after being serialized
    [[nodiscard]]
    virtual std::size_t serialized_size() const = 0;

    /// Serialize message into a byte stream
    virtual void serialize(OutByteStream& strm) const = 0;

    /// Parse message data from byte stream (without message id)
    virtual void parse(InByteStream& strm) = 0;

    /// Parse byte stream and construct message object
    static IMessage<Identifier>* parse_message(InByteStream& strm)
    {
        id_t message_id{};
        std::unique_ptr<IMessage<Identifier>> message;

        try
        {
            strm >> message_id;
            message.reset(make(message_id));
        }
        catch(const std::out_of_range& e)
        {
            message.reset();
        }

        try
        {
            message->parse(strm);
        }
        catch(const std::exception& e)
        {
            message.reset();
        }

        return message.release();
    }

    /// Return message id
    [[nodiscard]]
    id_t id() const
    {
        return id_;
    }

    /// Base for all message objects
    template<typename Derived, id_t MESSAGE_ID>
    struct Derive : IMessage<Identifier>::template RegisterConst<Derived, MESSAGE_ID>
    {
        static constexpr IMessage<Identifier>::id_t ID{MESSAGE_ID};

        using Base = typename IMessage<Identifier>::template RegisterConst<Derived, ID>;

        /// Implement serialization
        void serialize(OutByteStream& strm) const final
        {
            strm << ID << static_cast<const Derived&>(*this);
        }

        /// Implement parsing
        void parse(InByteStream& strm) final
        {
            strm >> static_cast<Derived&>(*this);
        }

        /// Implement size calculation
        [[nodiscard]]
        std::size_t serialized_size() const final
        {
            std::size_t size{sizeof(id_t)};
            ::serialized_size(size, static_cast<const Derived&>(*this));
            return size;
        }

    protected:
        Derive():
            Base(ID)
        {}
    };

    template<typename T, id_t>
    friend struct Derive;

private:
    explicit IMessage(id_t id):
        id_{id}
    {}

    id_t id_;
};
