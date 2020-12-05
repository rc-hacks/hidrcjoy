//
// PpmReceiver.h
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#pragma once
#include <atl/autolock.h>
#include <stdint.h>
#include "Configuration.h"

/////////////////////////////////////////////////////////////////////////////

class PpmReceiver
{
    static const uint8_t minChannels = 4;
    static const uint8_t maxTimeoutCounter = 25;

public:
    void Initialize(void)
    {
#if defined (__AVR_ATtiny85__)
        USISR = 0x0F;

        // Outputs disabled, enable counter overflow interrupt, external clock source
        USICR = _BV(USIOIE) | _BV(USIWM1) | _BV(USIWM0) | _BV(USICS1);
#elif defined (__AVR_ATtiny44__)
        // On the FabISP board, the ICP pin is tied up for USB, so use the ADC comparator with ADC6 instead

        // Analog comparator bandgap select, analog comparator input capture enable
        ACSR = _BV(ACBG) | _BV(ACIC);

        // Disable ADC
        ADCSRA = 0;

        // Analog comparator multiplexer enable
        ADCSRB = _BV(ACME);

        // ADC6
        ADMUX = _BV(MUX2) | _BV(MUX1);

        // Noise canceler, input capture rising edge, clk/8
        TCCR1B = _BV(ICNC1) | _BV(CS11);
#elif defined (__AVR_ATtiny167__) || defined (__AVR_ATmega32U4__)
        // Noise canceler, input capture rising/falling edge, clk/8
        TCCR1A = 0;
        TCCR1B = _BV(ICNC1) | (m_invertedSignal ? 0 : _BV(ICES1)) | _BV(CS11);

        // Set output compare to timeout
        OCR1A = TCNT1 + m_minSyncPulseWidth;

        //  Input Capture Interrupt Enable, Output Compare A Match Interrupt Enable
        TIMSK1 = _BV(ICIE1) | _BV(OCIE1A);
#else
#error Unsupported MCU
#endif

        Reset();
    }

    void SetConfiguration(uint16_t minSyncPulseWidth, bool invertedSignal)
    {
        m_minSyncPulseWidth = UsToTicks(minSyncPulseWidth);
        m_invertedSignal = invertedSignal;
        ATL_DEBUG_PRINT("PpmReceiver::SetConfiguration(%u, %d)\n", minSyncPulseWidth, invertedSignal);
        Initialize();
    }

    void Reset()
    {
        m_isReceiving = false;
        m_skipFirstPulse = true;
        m_hasSyncPulse = false;
        m_hasNewData = false;
    }

    void OnInputCapture()
    {
        uint16_t time = ICR1;
        OCR1A = time + m_minSyncPulseWidth;
        ProcessPulse(time);
    }

    void OnOutputCompare()
    {
        OCR1A += m_minSyncPulseWidth;
        ProcessTimeout();
    }

    bool IsReceiving() const
    {
        return m_isReceiving;
    }

    bool HasNewData() const
    {
        return m_hasNewData;
    }

    uint16_t GetChannelData(uint8_t channel) const
    {
        AutoLock lock;
        return m_pulseWidthUs[channel];
    }

    void AcknowledgeNewData()
    {
        m_hasNewData = false;
    }

private:
    void ProcessPulse(uint16_t time)
    {
        uint16_t diff = time - m_timeOfLastPulse;
        m_timeOfLastPulse = time;

        if (m_skipFirstPulse)
        {
            m_skipFirstPulse = false;
        }
        else if (diff >= m_minSyncPulseWidth)
        {
            m_currentChannel = 0;
            m_hasSyncPulse = true;
            g_pinDebug10 = true;
        }
        else if (m_hasSyncPulse)
        {
            if (m_currentChannel < Configuration::maxChannels)
            {
                m_pulseWidthTicks[m_currentChannel] = diff;
                m_currentChannel++;
            }
        }

        m_timeoutCounter = 0;
    }

    void ProcessTimeout()
    {
        g_pinDebug10 = false;

        if (m_hasSyncPulse && m_currentChannel >= Configuration::minChannels)
        {
            for (uint8_t i = 0; i < Configuration::maxChannels; i++)
            {
                m_pulseWidthUs[i] = i < m_currentChannel ? TicksToUs(m_pulseWidthTicks[i]) : 0;
            }

            m_isReceiving = true;
            m_hasSyncPulse = false;
            m_hasNewData = true;
        }
        else
        {
            if (m_timeoutCounter < maxTimeoutCounter)
            {
                m_timeoutCounter++;
            }
            else
            {
                Reset();
            }
        }
    }

    static uint16_t TicksToUs(uint16_t value)
    {
#if defined (__AVR_ATtiny85__)
        return value * (64 * 2) / (2 * F_CPU / 1000000);
#else
        return value * 8 / (F_CPU / 1000000);
#endif
    }

    static uint16_t UsToTicks(uint16_t value)
    {
#if defined (__AVR_ATtiny85__)
        return value * (2 * F_CPU / 1000000) / (64 * 2);
#else
        return value * (F_CPU / 1000000) / 8;
#endif
    }

private:
    volatile bool m_isReceiving = false;
    volatile bool m_skipFirstPulse = true;
    volatile bool m_hasSyncPulse = false;
    volatile bool m_hasNewData = false;
    volatile uint8_t m_timeoutCounter = 0;
    volatile uint8_t m_currentChannel = 0;
    volatile uint16_t m_pulseWidthTicks[Configuration::maxChannels] = {};
    volatile uint16_t m_pulseWidthUs[Configuration::maxChannels] = {};
    uint16_t m_timeOfLastPulse = 0;
    uint16_t m_minSyncPulseWidth = 0;
    bool m_invertedSignal = false;
} g_PpmReceiver;

#if defined (__AVR_ATtiny85__)
ISR(USI_OVF_vect)
{
    uint16_t ticks = g_Timer.GetTicksNoCli();
    bool level = (PPM_SIGNAL_PIN & _BV(PPM_SIGNAL)) != 0;

    // Clear counter overflow flag
    USISR = _BV(USIOIF) | 0x0F;

    sei();
    g_PpmReceiver.OnPinChanged(level, ticks);
}
#elif defined (__AVR_ATtiny44__) || defined (__AVR_ATtiny167__) || defined (__AVR_ATmega32U4__)
ISR(TIMER1_CAPT_vect)
{
    g_PpmReceiver.OnInputCapture();
}

ISR(TIMER1_COMPA_vect)
{
    g_PpmReceiver.OnOutputCompare();
}
#endif
