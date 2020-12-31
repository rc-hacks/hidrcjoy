//
// SrxlReceiverTimer1C.h
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#pragma once
#include <avr/io.h>
#include <stdint.h>

/////////////////////////////////////////////////////////////////////////////

class SrxlReceiverTimer1C
{
public:
    static void Initialize()
    {
        uint8_t tccr1a = 0;
        uint8_t tccr1b = 0;

        // clk/8
        tccr1b |= _BV(CS11);

        TCCR1A = tccr1a;
        TCCR1B = tccr1b;

        // Set long timeout
        OCR1C = TCNT1 - 1;

        // Clear pending IRQs
        TIFR1 |= _BV(OCF1C);

        // Enable IRQs: Output Compare C
        TIMSK1 = _BV(OCIE1C);
    }

    static void Terminate(void)
    {
        TIMSK1 &= ~_BV(OCIE1C);
    }

    static volatile uint16_t& TCNT()
    {
        return TCNT1;
    }

    static volatile uint16_t& OCR()
    {
        return OCR1C;
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
