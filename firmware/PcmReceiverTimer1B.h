//
// PcmReceiverTimer1B.h
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#pragma once
#include <avr/io.h>
#include <stdint.h>

/////////////////////////////////////////////////////////////////////////////

class PcmReceiverTimer1B
{
public:
    static void Initialize()
    {
        uint8_t tccr1a = 0;
        uint8_t tccr1b = 0;

        // clk/8
        tccr1b |= _BV(CS11);

        //  Input Capture Noise Canceler
        tccr1b |= _BV(ICNC1);

        TCCR1A = tccr1a;
        TCCR1B = tccr1b;

        // Clear pending IRQs
        TIFR1 |= _BV(ICF1);

        // Enable IRQs: Input Capture
        TIMSK1 |= _BV(ICIE1);
    }

    static void Terminate(void)
    {
        TIMSK1 &= ~_BV(ICIE1);
    }

    static void SetCaptureEdge(bool risingEdge)
    {
        if (risingEdge)
        {
            TCCR1B |= _BV(ICES1);
        }
        else
        {
            TCCR1B &= ~_BV(ICES1);
        }
    }

    static volatile uint16_t& ICR()
    {
        return ICR1;
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
