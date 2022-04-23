//
// srxl_receiver_timer1c.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <avr/io.h>
#include <stdint.h>

class SrxlReceiverTimer1C
{
public:
    static void Initialize()
    {
        // Set long timeout
        OCR1C = TCNT1 - 1;

        // Clear pending IRQs
        TIFR1 |= _BV(OCF1C);

        // Enable IRQs: Output Compare C
        TIMSK1 |= _BV(OCIE1C);
    }

    static volatile uint16_t& TCNT()
    {
        return TCNT1;
    }

    static volatile uint16_t& OCR()
    {
        return OCR1C;
    }

    // clk/8 => 1.3824 ticks/us

    static constexpr uint16_t TicksToUs(uint16_t value)
    {
        return static_cast<uint16_t>(static_cast<uint32_t>(value) * 80000 / (F_CPU / 100));
    }

    static constexpr uint16_t UsToTicks(uint16_t value)
    {
        return static_cast<uint16_t>(static_cast<uint32_t>(value) * (F_CPU / 100) / 80000);
    }
};
