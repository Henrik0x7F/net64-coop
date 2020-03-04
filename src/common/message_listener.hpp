//
// Created by henrik on 20.02.20.
// Copyright 2020 Net64 Coop Project
// Licensed under GPLv3
// Refer to the LICENSE file included
//

#pragma once

#include "common/double_dispatch.hpp"
#include "common/message_interface.hpp"


/**
 * Interface for all message listeners
 * @tparam Identifier Type to identify message types
 */
template<typename Identifier>
struct IMessageListener
{
    using MessageType = IMessage<Identifier>;

    struct MessageIdPredicate
    {
        template<typename Concrete, typename Base>
        bool check(const Base& base)
        {
            return (base.id() == Concrete::ID);
        }
    };

    virtual ~IMessageListener() = default;

    /// React to message
    virtual void handle_message(const MessageType& msg) = 0;

private:
    template<typename Derived, typename... Messages>
    struct DeriveFilter : IMessageListener<Identifier>
    {
        struct Handler
        {
            Derived& obj_;

            template<typename T>
            void on_dispatch(const T& msg)
            {
                obj_.on_message(msg);
            }
        };

        void handle_message(const MessageType& msg) final
        {
            custom_dispatch<Handler, MessageIdPredicate, const MessageType&, Messages...>(Handler{static_cast<Derived&>(*this)}, msg);
        }
    };

public:
    /**
     * Base for message listeners
     * Forwards all message types to Derived::on_message(const MessageType&)
    */
    template<typename Derived>
    struct Derive : IMessageListener<Identifier>
    {
        /**
         * Base for message listeners
         * Forwards only specified message types via double dispatch
         * Calls Derived::on_message(const MessageType&) for every message type
         * @tparam Messages Message types to forward
        */
        template<typename... Messages>
        using Filter = DeriveFilter<Derived, Messages...>;

        void handle_message(const MessageType& msg) final
        {
            static_cast<Derived&>(*this).on_message(msg);
        }
    };

private:
    template<typename>
    friend struct Derive;
    template<typename T, typename...>
    friend struct DeriveFilter;

    IMessageListener() = default;
};
