//
// Created by henrik on 09.11.19.
// Copyright 2020 Net64 Coop Project
// Licensed under GPLv3
// Refer to the LICENSE file included
//

#pragma once

#include <streambuf>


/// Fixed size byte buffer using externally allocated memory
struct ByteRefBuffer : std::basic_streambuf<std::uint8_t>
{
    using CharType = std::uint8_t;

    enum struct InitialState
    {
        FULL,
        EMPTY
    };

    ByteRefBuffer(void* data, std::streamsize size, InitialState state);

protected:
    std::streamsize showmanyc() override;
    std::streamsize xsgetn(CharType* s, std::streamsize n) override;

    std::streamsize xsputn(const CharType* s, std::streamsize n) override;
};
