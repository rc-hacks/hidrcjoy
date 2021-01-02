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
        // clk/8, Input Capture Noise Canceler
        TCCR1A = 0;
        TCCR1B = _BV(CS11) | _BV(ICNC1);

        // Set timeout
        OCR1A = TCNT1 + UsToTicks(TaskTickUs);

        // Clear pending IRQs
        TIFR1 |= _BV(OCF1A);

        // Enable IRQs: Output Compare A
        TIMSK1 |= _BV(OCIE1A);
    }

    void Terminate(void)
    {
        TIMSK1 &= ~_BV(OCIE1A);
    }

    uint32_t GetMilliseconds() const
    {
        return m_milliseconds;
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

    static volatile uint16_t& TCNT()
    {
        return TCNT1;
    }

    static volatile uint16_t& ICR()
    {
        return ICR1;
    }

    void OnOutputCompare(void)
    {
        OCR1A += UsToTicks(TaskTickUs);
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
