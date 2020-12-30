//
// TaskTimer1.h
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#pragma once
#include <avr/io.h>
#include <stdint.h>

/////////////////////////////////////////////////////////////////////////////

class TaskTimer1
{
    static const uint16_t TaskTickUs = 1000; // 1ms

public:
    static void Initialize()
    {

        // clk/8
        TCCR3A = 0;
        TCCR3B = _BV(CS31);

        // Set timeout
        OCR3A = TCNT3 + UsToTicks(TaskTickUs);

        // Clear pending IRQs
        TIFR3 |= _BV(OCF3A);

        // Enable IRQs: Output Compare A
        TIMSK3 = _BV(OCIE3A);
    }

    static void Terminate(void)
    {
        TIMSK3 = 0;
    }

    static void SetNextTick(void)
    {
        OCR3A += UsToTicks(TaskTickUs);
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
