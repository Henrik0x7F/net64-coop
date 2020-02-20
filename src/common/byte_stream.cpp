//
// Created by henrik on 06.11.19.
// Copyright 2020 Net64 Coop Project
// Licensed under GPLv3
// Refer to the LICENSE file included
//

#include "byte_stream.hpp"


InByteStream::InByteStream(IByteBuffer& buf):
    streambuf_{&buf}
{}

IByteBuffer& InByteStream::buffer() const
{
    return *streambuf_;
}

OutByteStream::OutByteStream(IByteBuffer& buf):
    streambuf_{&buf}
{}

IByteBuffer& OutByteStream::buffer() const
{
    return *streambuf_;
}

ByteStream::ByteStream(IByteBuffer& buf):
OutByteStream(buf), InByteStream(buf)
{}


// Read / write overloads

InByteStream& operator>>(InByteStream& strm, char& val)
{
    strm >> reinterpret_cast<std::uint8_t&>(val);
    return strm;
}

OutByteStream& operator<<(OutByteStream& strm, char val)
{
    strm << static_cast<std::uint8_t>(val);
    return strm;
}

void serialized_size(std::size_t& size, char val)
{
    size += sizeof(val);
}

InByteStream& operator>>(InByteStream& strm, std::uint8_t& val)
{
    auto tmp{strm.buffer().sbumpc()};
    if(static_cast<int>(tmp) == std::char_traits<char>::eof())
        throw std::range_error("Attempted to read from empty buffer");
    val = static_cast<std::uint8_t>(tmp);
    return strm;
}

OutByteStream& operator<<(OutByteStream& strm, std::uint8_t val)
{
    strm.buffer().sputc(static_cast<char>(val));
    return strm;
}

void serialized_size(std::size_t& size, std::uint8_t val)
{
    size += sizeof(val);
}

InByteStream& operator>>(InByteStream& strm, std::int8_t& val)
{
    strm >> reinterpret_cast<std::uint8_t&>(val);
    return strm;
}


OutByteStream& operator<<(OutByteStream& strm, std::int8_t val)
{
    strm << static_cast<std::uint8_t>(val);
    return strm;
}

void serialized_size(std::size_t& size, std::int8_t val)
{
    size += sizeof(val);
}

InByteStream& operator>>(InByteStream& strm, std::uint16_t& val)
{
    std::uint8_t tmp;
    strm >> tmp;
    val = tmp;
    val <<= 8u;
    strm >> tmp;
    val |= tmp;
    return strm;
}

OutByteStream& operator<<(OutByteStream& strm, std::uint16_t val)
{
    strm << static_cast<std::uint8_t>(val >> 8u);
    strm << static_cast<std::uint8_t>(val);
    return strm;
}

void serialized_size(std::size_t& size, std::uint16_t val)
{
    size += sizeof(val);
}

InByteStream& operator>>(InByteStream& strm, std::int16_t& val)
{
    std::uint8_t tmp;
    strm >> tmp;
    val = tmp;
    val <<= 8u;
    strm >> tmp;
    val |= tmp;
    return strm;
}

OutByteStream& operator<<(OutByteStream& strm, std::int16_t val)
{
    strm << static_cast<std::uint8_t>(val >> 8u);
    strm << static_cast<std::uint8_t>(val);
    return strm;
}

void serialized_size(std::size_t& size, std::int16_t val)
{
    size += sizeof(val);
}

InByteStream& operator>>(InByteStream& strm, std::uint32_t& val)
{
    std::uint16_t tmp;
    strm >> tmp;
    val = tmp;
    val <<= 16u;
    strm >> tmp;
    val |= tmp;
    return strm;
}

OutByteStream& operator<<(OutByteStream& strm, std::uint32_t val)
{
    strm << static_cast<std::uint16_t>(val >> 16u);
    strm << static_cast<std::uint16_t>(val);
    return strm;
}

void serialized_size(std::size_t& size, std::uint32_t val)
{
    size += sizeof(val);
}

InByteStream& operator>>(InByteStream& strm, std::int32_t& val)
{
    std::uint16_t tmp;
    strm >> tmp;
    val = tmp;
    val <<= 16u;
    strm >> tmp;
    val |= tmp;
    return strm;
}

OutByteStream& operator<<(OutByteStream& strm, std::int32_t val)
{
    strm << static_cast<std::uint16_t>(val >> 16u);
    strm << static_cast<std::uint16_t>(val);
    return strm;
}

void serialized_size(std::size_t& size, std::int32_t val)
{
    size += sizeof(val);
}

InByteStream& operator>>(InByteStream& strm, std::uint64_t& val)
{
    std::uint32_t tmp;
    strm >> tmp;
    val = tmp;
    val <<= 32u;
    strm >> tmp;
    val |= tmp;
    return strm;
}

OutByteStream& operator<<(OutByteStream& strm, std::uint64_t val)
{
    strm << static_cast<std::uint32_t>(val >> 32u);
    strm << static_cast<std::uint32_t>(val);
    return strm;
}

void serialized_size(std::size_t& size, std::uint64_t val)
{
    size += sizeof(val);
}

InByteStream& operator>>(InByteStream& strm, std::int64_t& val)
{
    std::uint32_t tmp;
    strm >> tmp;
    val = tmp;
    val <<= 32u;
    strm >> tmp;
    val |= tmp;
    return strm;
}

OutByteStream& operator<<(OutByteStream& strm, std::int64_t val)
{
    strm << static_cast<std::uint32_t>(val >> 32u);
    strm << static_cast<std::uint32_t>(val);
    return strm;
}

void serialized_size(std::size_t& size, std::int64_t val)
{
    size += sizeof(val);
}

InByteStream& operator>>(InByteStream& strm, float& val)
{
    auto n{strm.buffer().sgetn(reinterpret_cast<std::uint8_t*>(&val), sizeof(val))};
    if(n != sizeof(val))
        throw std::range_error("End of stream reached");
    return strm;
}

OutByteStream& operator<<(OutByteStream& strm, float val)
{
    auto n{strm.buffer().sputn(reinterpret_cast<const std::uint8_t*>(&val), sizeof(val))};
    if(n != sizeof(val))
        throw std::range_error("End of stream reached");
    return strm;
}

void serialized_size(std::size_t& size, float val)
{
    size += sizeof(val);
}

InByteStream& operator>>(InByteStream& strm, double& val)
{
    auto n{strm.buffer().sgetn(reinterpret_cast<std::uint8_t*>(&val), sizeof(val))};
    if(n != sizeof(val))
        throw std::range_error("End of stream reached");
    return strm;
}

OutByteStream& operator<<(OutByteStream& strm, double val)
{
    auto n{strm.buffer().sputn(reinterpret_cast<const std::uint8_t*>(&val), sizeof(val))};
    if(n != sizeof(val))
        throw std::range_error("End of stream reached");
    return strm;
}

void serialized_size(std::size_t& size, double val)
{
    size += sizeof(val);
}

InByteStream& operator>>(InByteStream& strm, bool& val)
{
    std::uint8_t tmp;
    strm >> tmp;
    val = (tmp == 1);
    return strm;
}

OutByteStream& operator<<(OutByteStream& strm, bool val)
{
    strm << static_cast<std::uint8_t>(val == 1);
    return strm;
}

void serialized_size(std::size_t& size, bool)
{
    ++size;
}
