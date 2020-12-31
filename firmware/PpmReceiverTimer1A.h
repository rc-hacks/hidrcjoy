//
// PpmReceiverTimer1A.h
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#pragma once
#include <avr/io.h>
#include <stdint.h>

/////////////////////////////////////////////////////////////////////////////

class PpmReceiverTimer1A
{
public:
    static void Initialize(bool fallingEdge)
    {
        uint8_t tccr1a = 0;
        uint8_t tccr1b = 0;

        // clk/8
        tccr1b |= _BV(CS11);

        //  Input Capture Noise Canceler
        tccr1b |= _BV(ICNC1);

        //  Input Capture Edge Select
        tccr1b |= fallingEdge ? 0 : _BV(ICES1);

        TCCR1A = tccr1a;
        TCCR1B = tccr1b;

        // Set long timeout
        OCR1A = TCNT1 - 1;

        // Clear pending IRQs
        TIFR1 |= _BV(ICF1) | _BV(OCF1A);

        // Enable IRQs: Input Capture, Output Compare A
        TIMSK1 |= _BV(ICIE1) | _BV(OCIE1A);
    }

    static void Terminate(void)
    {
        TIMSK1 &= ~(_BV(ICIE1) | _BV(OCIE1A));
    }

    static volatile uint16_t& TCNT()
    {
        return TCNT1;
    }

    static volatile uint16_t& OCR()
    {
        return OCR1A;
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
