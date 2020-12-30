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
    static void Initialize(bool invertedSignal)
    {
        // Noise canceler, input capture edge, clk/8
        TCCR1A = 0;
        TCCR1B = (TCCR1B & ~(_BV(CS32) | _BV(CS31) | _BV(CS30))) | _BV(ICNC1) | (invertedSignal ? 0 : _BV(ICES1)) | _BV(CS11);

        // Clear pending IRQs
        TIFR1 |= _BV(ICF1);

        // Enable IRQs: Input Capture
        TIMSK1 |= _BV(ICIE1);
    }

    static void Terminate(void)
    {
        TIMSK1 &= ~(_BV(ICIE1) | _BV(OCIE1B));
    }

    static void SetCaptureEdge(bool rising)
    {
        if (rising)
        {
            TCCR1B |= _BV(ICES1);
        }
        else
        {
            TCCR1B &= ~_BV(ICES1);
        }
    }

    static volatile uint16_t& TCNT()
    {
        return TCNT1;
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
