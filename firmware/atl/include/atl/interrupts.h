//
// interrupts.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <avr/interrupt.h>

namespace atl
{
    class Interrupts
    {
    public:
        static void Enable()
        {
            sei();
        }

        static void Disable()
        {
            cli();
        }
    };
}
