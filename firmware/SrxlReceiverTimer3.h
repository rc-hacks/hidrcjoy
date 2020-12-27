//
// SrxlReceiverTimer3.h
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#pragma once
#include <avr/io.h>
#include <stdint.h>

/////////////////////////////////////////////////////////////////////////////

class SrxlReceiverTimer3
{
public:
    static void Initialize()
    {
        // clk/8
        TCCR3A = 0;
        TCCR3B = _BV(CS31);

        // Set long timeout
        OCR3B = OCR3A = TCNT3 - 1;

        // Clear pending IRQs
        TIFR3 |= _BV(OCF3B) | _BV(OCF3A);

        // Enable IRQs: Output Compare A/B
        TIMSK3 = _BV(OCIE3B) | _BV(OCIE3A);
    }

    static void Terminate(void)
    {
        TIMSK3 = 0;
    }

    static volatile uint16_t& TCNT()
    {
        return TCNT3;
    }

    static volatile uint16_t& OCRA()
    {
        return OCR3A;
    }

    static volatile uint16_t& OCRB()
    {
        return OCR3B;
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
