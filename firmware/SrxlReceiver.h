//
// SrxlReceiver.h
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#pragma once
#include <stdint.h>
#include "Configuration.h"

/////////////////////////////////////////////////////////////////////////////

class SrxlReceiver
{
    static const uint8_t headerV1 = 0xA1;
    static const uint8_t headerV2 = 0xA2;
    static const uint32_t baudrate = 115200;
    static const uint32_t dataFrameTimeout = 4000;
    static const uint32_t signalTimeout = 100000;

    enum FrameStatus : uint8_t
    {
        Special = 0xF0,
        Empty,
        Ready,
        Ok,
        Error,
    };

    struct Frame
    {
        uint8_t m_data[1 + 16 * 2 + 2] = {};
        uint8_t m_status = 0;
        uint8_t m_position = 0;
    };

public:
    void Initialize(void)
    {
#if defined(UCSR0A)
        UBRR0 = ((F_CPU / 4 / baudrate) - 1) / 2;
        UCSR1A = _BV(U2X1);

        // Enable receiver and RX IRQ
        UCSR1B = _BV(RXEN1) | _BV(RXCIE1);
        UCSR1C = _BV(UCSZ11) | _BV(UCSZ10);
#elif defined(UCSR1A)
        UBRR1 = ((F_CPU / 4 / baudrate) - 1) / 2;
        UCSR1A = _BV(U2X1);

        // Enable receiver and RX IRQ
        UCSR1B = _BV(RXEN1) | _BV(RXCIE1);
        UCSR1C = _BV(UCSZ11) | _BV(UCSZ10);
#else
#error Unsupported configuration
#endif
    }

    void Update(uint32_t time)
    {
        DecodeDataFrame();

        uint8_t updateCounter = m_updateCounter;
        if (updateCounter == m_lastUpdateCount)
        {
            if (time - m_lastUpdateTime > signalTimeout)
            {
                m_isReceiving = false;
            }
        }
        else
        {
            m_lastUpdateCount = updateCounter;
            m_lastUpdateTime = time;
            m_isReceiving = true;
        }
    }

    bool IsReceiving() const
    {
        return m_isReceiving;
    }

    uint16_t GetChannelData(uint8_t channel) const
    {
        const Frame& frame = GetReceivedFrame();
        uint8_t index = 1 + channel * 2;
        cli();
        uint16_t value = GetUInt16(frame.m_data, index);
        sei();
        return TicksToUs(value);
    }

    void OnDataReceived(uint8_t ch, uint32_t time)
    {
        uint32_t diff = time - m_lastTime;
        m_lastTime = time;

        Frame& frame = GetCurrentFrame();

        if (diff > dataFrameTimeout)
        {
            frame.m_position = 0;
            frame.m_status = Empty;
        }

        if (frame.m_position < sizeof(frame.m_data))
        {
            frame.m_data[frame.m_position++] = ch;

            if (frame.m_position >= sizeof(frame.m_data))
            {
                frame.m_status = Ready;
                m_frameIndex = !m_frameIndex;
            }
        }
    }

private:
    static uint16_t TicksToUs(uint16_t value)
    {
        return 800 + static_cast<uint16_t>(static_cast<uint32_t>(value & 0xFFF) * (2200 - 800) / 0x1000);
    }

    void DecodeDataFrame()
    {
        Frame& frame = GetReceivedFrame();

        if (frame.m_status == Ready)
        {
            uint8_t payloadLength = 0;
            switch (frame.m_data[0])
            {
            case headerV1:
                payloadLength = 1 + 12 * 2;
                break;
            case headerV2:
                payloadLength = 1 + 16 * 2;
                break;
            }

            if (payloadLength > 0)
            {
                uint16_t expectedCrc = GetUInt16(frame.m_data, payloadLength);
                if (CalculateCrc16(frame.m_data, payloadLength) == expectedCrc)
                {
                    m_updateCounter++;
                    frame.m_status = Ok;
                }
                else
                {
                    frame.m_status = Error;
                }
            }
        }
    }

    Frame& GetCurrentFrame()
    {
        return m_frameIndex == 0 ? m_frame[0] : m_frame[1];
    }

    Frame& GetReceivedFrame()
    {
        return m_frameIndex == 1 ? m_frame[0] : m_frame[1];
    }

    const Frame& GetReceivedFrame() const
    {
        return m_frameIndex == 1 ? m_frame[0] : m_frame[1];
    }

    static uint16_t GetUInt16(const uint8_t* data, uint8_t index)
    {
        return (data[index] << 8) | data[index + 1];
    }

    static uint16_t CalculateCrc16(const uint8_t* data, uint8_t count)
    {
        uint16_t crc = 0;

        for (uint8_t i = 0; i < count; i++)
        {
            crc = CalculateCrc16(crc, data[i]);
        }

        return crc;
    }

    static uint16_t CalculateCrc16(uint16_t crc, uint8_t value)
    {
        crc = crc ^ (static_cast<uint16_t>(value) << 8);

        for (uint8_t i = 0; i < 8; i++)
        {
            if ((crc & 0x8000) != 0)
            {
                crc = (crc << 1) ^ 0x1021;
            }
            else
            {
                crc = crc << 1;
            }
        }

        return crc;
    }

private:
    uint32_t m_lastTime = 0;
    Frame m_frame[2];
    uint8_t m_frameIndex = 0;
    uint8_t m_updateCounter = 0;
    uint8_t m_lastUpdateCount = 0;
    uint32_t m_lastUpdateTime = 0;
    bool m_isReceiving = false;
} g_SrxlReceiver;

#if defined(UCSR0A)
ISR(USART1_RX_vect)
{
    g_SrxlReceiver.OnDataReceived(UDR0, g_Timer.GetTimeInUs());
}
#elif defined(UCSR1A)
ISR(USART1_RX_vect)
{
    g_SrxlReceiver.OnDataReceived(UDR1, g_Timer.GetTimeInUs());
}
#endif
