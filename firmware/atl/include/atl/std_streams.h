//
// std_streams.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <stdio.h>

// In order to setup printf() support, you need to do:
// - Call StdStreams::SetupStdout() once at initialization.
//
// Note: As printf may be called from an IRQ context, putchar() must work without IRQs.
// For instance, use an unbuffered serial stream.
//
// Example:
// 
// static Serial serial;
// 
// int main(void)
// {
//     StdStreams::SetupStdout([](char ch) { serial.WriteChar(ch); });
//     serial.Open(115200);
//     ...
// }

// Workaroud for C++ incompatibility in avr-libc.
#undef FDEV_SETUP_STREAM
#define FDEV_SETUP_STREAM(p, g, f) { NULL, 0, f, 0, 0, p, g, 0 }

namespace atl
{
    class StdStreams
    {
    public:
        using getchar_t = char(*)(void);
        using putchar_t = void(*)(char ch);

        static void SetupStdin(getchar_t getchar)
        {
            static FILE file = FDEV_SETUP_STREAM(NULL, GetCharWrapper, _FDEV_SETUP_READ);
            GetCharRef() = getchar;
            stdin = &file;
        }

        static void SetupStdout(putchar_t putchar)
        {
            static FILE file = FDEV_SETUP_STREAM(PutCharWrapper, NULL, _FDEV_SETUP_WRITE);
            PutCharRef() = putchar;
            stdout = &file;
            stderr = &file;
        }

    private:
        static int GetCharWrapper(FILE*)
        {
            return GetCharRef()();
        }

        static int PutCharWrapper(char ch, FILE*)
        {
            PutCharRef()(ch);
            return 0;
        }

        static getchar_t& GetCharRef()
        {
            static getchar_t getchar;
            return getchar;
        }

        static putchar_t& PutCharRef()
        {
            static putchar_t putchar;
            return putchar;
        }
    };
}
