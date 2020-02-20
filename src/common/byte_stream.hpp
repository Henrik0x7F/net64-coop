//
// Created by henrik on 06.11.19.
// Copyright 2020 Net64 Coop Project
// Licensed under GPLv3
// Refer to the LICENSE file included
//

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <streambuf>
#include <sstream>
#include <string>
#include <vector>


using IByteBuffer = std::basic_streambuf<std::uint8_t>;
using ByteBuffer = std::basic_stringbuf<std::uint8_t>;


template<typename T, typename Action, typename Strm>
void serialize_info(T&, Action, Strm&);
template<typename T>
void serialized_size(std::size_t& size, const T& val);

template<typename Action, typename Strm, typename... Args>
void stream(Action a, Strm& strm, Args&... args);

enum struct SerializeAction
{
    PARSE,
    SERIALIZE,
    CALC_SIZE
};

namespace Impl
{

template<SerializeAction T_TYPE>
struct ActionType
{
    static constexpr SerializeAction TYPE{T_TYPE};
};

} // Impl

struct InByteStream
{
    explicit InByteStream(IByteBuffer& buf);

    [[nodiscard]]
    IByteBuffer& buffer() const;

    template<typename T>
    InByteStream& operator>>(T& val)
    {
        serialize_info(val, Parse(), *this);
        return *this;
    }

private:
    struct Parse : Impl::ActionType<SerializeAction::PARSE>{};

    template<typename T>
    friend void stream_field(Parse, InByteStream&, T&);

    IByteBuffer* streambuf_;
};

struct OutByteStream
{
    explicit OutByteStream(IByteBuffer& buf);

    [[nodiscard]]
    IByteBuffer& buffer() const;

    template<typename T>
    OutByteStream& operator<<(const T& val)
    {
        serialize_info(const_cast<T&>(val), Serialize(), *this);
        return *this;
    }

    template<typename T>
    friend void serialized_size(std::size_t& size, const T& val);

private:
    struct Serialize : Impl::ActionType<SerializeAction::SERIALIZE>{};
    struct CalcSize : Impl::ActionType<SerializeAction::CALC_SIZE>{};

    template<typename T>
    friend void stream_field(Serialize, OutByteStream&, const T&);
    template<typename T>
    friend void stream_field(CalcSize, std::size_t&, const T&);

    IByteBuffer* streambuf_;
};

struct ByteStream : OutByteStream, InByteStream
{
    explicit ByteStream(IByteBuffer& buf);
};


// Read / write overloads

template<typename T>
void serialized_size(std::size_t& size, const T& val)
{
    serialize_info(const_cast<T&>(val), OutByteStream::CalcSize(), size);
}

InByteStream& operator>>(InByteStream& strm, char& val);
OutByteStream& operator<<(OutByteStream& strm, char val);
void serialized_size(std::size_t& size, char val);

InByteStream& operator>>(InByteStream& strm, std::uint8_t& val);
OutByteStream& operator<<(OutByteStream& strm, std::uint8_t val);
void serialized_size(std::size_t& size, std::uint8_t val);

InByteStream& operator>>(InByteStream& strm, std::int8_t& val);
OutByteStream& operator<<(OutByteStream& strm, std::int8_t val);
void serialized_size(std::size_t& size, std::int8_t val);

InByteStream& operator>>(InByteStream& strm, std::uint16_t& val);
OutByteStream& operator<<(OutByteStream& strm, std::uint16_t val);
void serialized_size(std::size_t& size, std::uint16_t val);

InByteStream& operator>>(InByteStream& strm, std::int16_t& val);
OutByteStream& operator<<(OutByteStream& strm, std::int16_t val);
void serialized_size(std::size_t& size, std::int16_t val);

InByteStream& operator>>(InByteStream& strm, std::uint32_t& val);
OutByteStream& operator<<(OutByteStream& strm, std::uint32_t val);
void serialized_size(std::size_t& size, std::uint32_t val);

InByteStream& operator>>(InByteStream& strm, std::int32_t& val);
OutByteStream& operator<<(OutByteStream& strm, std::int32_t val);
void serialized_size(std::size_t& size, std::int32_t val);

InByteStream& operator>>(InByteStream& strm, std::uint64_t& val);
OutByteStream& operator<<(OutByteStream& strm, std::uint64_t val);
void serialized_size(std::size_t& size, std::uint64_t val);

InByteStream& operator>>(InByteStream& strm, std::int64_t& val);
OutByteStream& operator<<(OutByteStream& strm, std::int64_t val);
void serialized_size(std::size_t& size, std::int64_t val);

InByteStream& operator>>(InByteStream& strm, float& val);
OutByteStream& operator<<(OutByteStream& strm, float val);
void serialized_size(std::size_t& size, float val);

InByteStream& operator>>(InByteStream& strm, double& val);
OutByteStream& operator<<(OutByteStream& strm, double val);
void serialized_size(std::size_t& size, double val);

InByteStream& operator>>(InByteStream& strm, bool& val);
OutByteStream& operator<<(OutByteStream& strm, bool val);
void serialized_size(std::size_t& size, bool val);


template<typename T>
void stream_field(ByteStream::Serialize, OutByteStream& strm, const T& val)
{
    strm << val;
}

template<typename T>
void stream_field(ByteStream::Parse, InByteStream& strm, T& val)
{
    strm >> val;
}

template<typename T>
void stream_field(ByteStream::CalcSize, std::size_t& size, const T& val)
{
    serialized_size(size, val);
}

template<typename Action, typename Strm, typename... Args>
void stream(Action a, Strm& strm, Args&... args)
{
    (stream_field(a, strm, args), ...);
}

template<typename Action, typename Strm>
void serialize_info(std::string& obj, Action a, Strm& strm)
{
    std::uint64_t s{obj.size()};
    stream_field(a, strm, s);
    if constexpr(Action::TYPE == SerializeAction::PARSE)
        obj.resize(s);
    for(std::uint64_t i{}; i < obj.size(); ++i)
    {
        stream_field(a, strm, obj.at(i));
    }
}
