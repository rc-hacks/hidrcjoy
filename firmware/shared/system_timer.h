//
// system_timer.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <atl/autolock.h>
#include <avr/io.h>
#include <stdint.h>

class SystemTimer
{
    static const uint16_t TaskTickUs = 1000; // 1ms

public:
    void Initialize()
    {
        // clk/8
        TCCR1A = 0;
        TCCR1B = _BV(CS11);

        // Set timeout
        OCR1A = TCNT1 + UsToTicks(TaskTickUs);

        // Clear pending IRQs
        TIFR1 |= _BV(OCF1A);

        // Enable IRQs: Output Compare A
        TIMSK1 |= _BV(OCIE1A);
    }

    uint16_t GetMilliseconds() const
    {
        atl::AutoLock lock;
        return m_milliseconds;
    }

    void OnOutputCompare()
    {
        OCR1A += UsToTicks(TaskTickUs);
        m_milliseconds++;
    }

    // clk/8 => 1.3824 ticks/us

    static constexpr uint16_t TicksToUs(uint16_t value)
    {
        return static_cast<uint16_t>(static_cast<uint32_t>(value) * 80000 / (F_CPU / 100));
    }

    static constexpr uint16_t UsToTicks(uint16_t value)
    {
        return static_cast<uint16_t>(static_cast<uint32_t>(value) * (F_CPU / 100) / 80000);
    }

private:
    uint16_t m_milliseconds = 0;
};
