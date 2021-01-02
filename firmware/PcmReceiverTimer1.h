//
// PcmReceiverTimer1.h
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#pragma once
#include <avr/io.h>
#include <stdint.h>

/////////////////////////////////////////////////////////////////////////////

class PcmReceiverTimer1
{
public:
    static void Initialize()
    {
        // clk/8, Input Capture Noise Canceler
        TCCR1A = 0;
        TCCR1B = _BV(CS11) | _BV(ICNC1);

        // Clear pending IRQs
        TIFR1 |= _BV(ICF1);

        // Enable IRQs: Input Capture
        TIMSK1 |= _BV(ICIE1);
    }

    static void Terminate(void)
    {
        TIMSK1 &= ~_BV(ICIE1);
    }

    static constexpr uint16_t TicksToUs(uint16_t value)
    {
        return value * 8 / (F_CPU / 1000000);
    }

    static constexpr uint16_t UsToTicks(uint16_t value)
    {
        return value * (F_CPU / 1000000) / 8;
    }
};
