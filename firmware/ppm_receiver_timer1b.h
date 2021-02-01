//
// ppm_receiver_timer_1b.h
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#pragma once
#include <avr/io.h>
#include <stdint.h>

/////////////////////////////////////////////////////////////////////////////

class PpmReceiverTimer1B
{
public:
    static void Initialize()
    {
        // clk/8, Input Capture Noise Canceler
        TCCR1A = 0;
        TCCR1B = _BV(CS11) | _BV(ICNC1);

        // Set long timeout
        OCR1B = TCNT1 - 1;

        // Clear pending IRQs
        TIFR1 |= _BV(ICF1) | _BV(OCF1B);

        // Enable IRQs: Input Capture, Output Compare B
        TIMSK1 |= _BV(ICIE1) | _BV(OCIE1B);
    }

    static void Terminate()
    {
        TIMSK1 &= ~(_BV(ICIE1) | _BV(OCIE1B));
    }

    static volatile uint16_t& TCNT()
    {
        return TCNT1;
    }

    static volatile uint16_t& OCR()
    {
        return OCR1B;
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
