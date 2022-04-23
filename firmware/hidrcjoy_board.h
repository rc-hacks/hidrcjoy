//
// hidrcjoy_board.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "hidrcjoy_pinout.h"

class Board
{
public:
    void Initialize()
    {
        DDRB = 0;
        DDRC = 0;
        DDRD = 0;

        PORTB = 0;
        PORTC = 0;
        PORTD = 0;

        TCCR0A = 0;
        TCCR0B = 0;
        TCCR1A = 0;
        TCCR1B = 0;

        m_led.Initialize();
    }

    void RunTask(uint16_t time)
    {
        m_led.RunTask(time);
    }

private:
    class Led
    {
    public:
        void Initialize()
        {
            LED_DDR |= _BV(LED_BIT);
            LED_PORT &= ~_BV(LED_BIT);
        }

        void Set(bool value)
        {
            if (value)
            {
                LED_PORT |= _BV(LED_BIT);
            }
            else
            {
                LED_PORT &= ~_BV(LED_BIT);
            }
        }

        void Blink(uint16_t period)
        {
            m_period = period - 100;
            m_delay = 0;
            m_on = false;
        }

        void RunTask(uint16_t time)
        {
            if (m_period == 0)
                return;

            if (m_delay > 0)
            {
                uint16_t diff = time - m_startTime;
                if (diff < m_delay)
                {
                    return;
                }
            }

            m_on = !m_on;
            Set(m_on);

            m_startTime = time;
            m_delay = m_on ? 100 : m_period;
        }

    private:
        uint16_t m_period = 0;
        uint16_t m_startTime = 0;
        uint16_t m_delay = 0;
        bool m_on = false;
    };

public:
    Led m_led;
};
