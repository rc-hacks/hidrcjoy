//
// pcm_receiver_timer1.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <avr/io.h>
#include <stdint.h>

class PcmReceiverTimer1
{
public:
    static void Initialize()
    {
        // Input Capture Noise Canceler
        TCCR1B |= _BV(ICNC1);

        // Clear pending IRQs
        TIFR1 |= _BV(ICF1);

        // Enable IRQs: Input Capture
        TIMSK1 |= _BV(ICIE1);
    }

    static volatile uint16_t& TCNT()
    {
        return TCNT1;
    }

    static volatile uint16_t& ICR()
    {
        return ICR1;
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
