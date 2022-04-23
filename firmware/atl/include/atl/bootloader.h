//
// bootloader.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <avr/boot.h>
#include <atl/autolock.h>
#include <atl/watchdog.h>
#include <stdint.h>

namespace atl
{
    class Bootloader
    {
    public:
        static void ResetToBootloader()
        {
            static const uint16_t bootKey = 0x7777;
            static const uint16_t bootKeyAddress = 0x0800;

            *reinterpret_cast<volatile uint16_t*>(bootKeyAddress) = bootKey;
            Watchdog::Enable(Watchdog::Timeout::Time120ms);
            while (true);
        }

        static void GetSerialNumber(uint8_t* serial, uint8_t count)
        {
            for (uint8_t i = 0; i < count; i++)
            {
                serial[i] = GetSignatureByte(0x000E + i);
            }
        }

        static uint8_t GetSignatureByte(uint16_t address)
        {
            AutoLock lock;
            return boot_signature_byte_get(address);
        }
    };
}
