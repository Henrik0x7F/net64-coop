//
// Created by henrik on 09.11.19.
// Copyright 2020 Net64 Coop Project
// Licensed under GPLv3
// Refer to the LICENSE file included
//

#include "byte_ref_buffer.hpp"
#include <algorithm>


ByteRefBuffer::ByteRefBuffer(void* data, std::streamsize size, InitialState state)
{
    auto begin{reinterpret_cast<CharType*>(data)};
    auto end{begin + size};

    if(state == InitialState::EMPTY)
    {
        setg(begin, begin, begin);
        setp(eback(), eback() + size);
    }
    else
    {
        setp(end, end);
        setg(begin, begin, end);
    }
}

std::streamsize ByteRefBuffer::showmanyc()
{
    return epptr() - gptr();
}

std::streamsize ByteRefBuffer::xsgetn(CharType* s, std::streamsize n)
{
    n = std::min(n, static_cast<std::streamsize>(epptr() - gptr()));
    std::copy_n(gptr(), n, s);
    gbump(n);
    setg(gptr(), gptr(), egptr());
    return n;
}

std::streamsize ByteRefBuffer::xsputn(const CharType* s, std::streamsize n)
{
    n = std::min(static_cast<std::streamsize>(epptr() - pptr()), n);
    std::copy_n(s, n, pptr());
    pbump(n);
    setp(pptr(), epptr());

    return n;
}
