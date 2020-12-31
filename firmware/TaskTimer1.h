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
    void Initialize()
    {
        uint8_t tccr3a = 0;
        uint8_t tccr3b = 0;

        // clk/8
        tccr3b |= _BV(CS31);

        TCCR3A = tccr3a;
        TCCR3B = tccr3b;

        // Set timeout
        OCR3A = TCNT3 + UsToTicks(TaskTickUs);

        // Clear pending IRQs
        TIFR3 |= _BV(OCF3A);

        // Enable IRQs: Output Compare A
        TIMSK3 |= _BV(OCIE3A);
    }

    void Terminate(void)
    {
        TIMSK3 &= ~_BV(OCIE3A);
    }

    uint32_t GetMilliseconds() const
    {
        return m_milliseconds;
    }

    void OnOutputCompare(void)
    {
        OCR3A += UsToTicks(TaskTickUs);
        m_milliseconds++;
    }

    static constexpr uint16_t TicksToUs(uint16_t value)
    {
        return value * 8 / (F_CPU / 1000000);
    }

    static constexpr uint16_t UsToTicks(uint16_t value)
    {
        return value * (F_CPU / 1000000) / 8;
    }

private:
    uint32_t m_milliseconds = 0;
};
