//
// watchdog.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <avr/wdt.h>

namespace atl
{
    class Watchdog
    {
    public:
        enum class Timeout : uint8_t
        {
            Time15ms = WDTO_15MS,
            Time30ms = WDTO_30MS,
            Time60ms = WDTO_60MS,
            Time120ms = WDTO_120MS,
            Time250ms = WDTO_250MS,
            Time500ms = WDTO_500MS,
            Time1s = WDTO_1S,
            Time2s = WDTO_2S,
        };

    public:
        static void Enable(Timeout timeout)
        {
            wdt_enable(static_cast<uint8_t>(timeout));
        }

        static void Disable()
        {
            MCUSR = 0;
            wdt_disable();
        }

        static void Reset()
        {
            wdt_reset();
        }
    };
}
