//
// ppm_receiver_timer1b.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <avr/io.h>
#include <stdint.h>

class PpmReceiverTimer1B
{
public:
    static void Initialize()
    {
        // Input Capture Noise Canceler
        TCCR1B |= _BV(ICNC1);

        // Set long timeout
        OCR1B = TCNT1 - 1;

        // Clear pending IRQs
        TIFR1 |= _BV(ICF1) | _BV(OCF1B);

        // Enable IRQs: Input Capture, Output Compare B
        TIMSK1 |= _BV(ICIE1) | _BV(OCIE1B);
    }

    static volatile uint16_t& TCNT()
    {
        return TCNT1;
    }

    static volatile uint16_t& ICR()
    {
        return ICR1;
    }

    static volatile uint16_t& OCR()
    {
        return OCR1B;
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
