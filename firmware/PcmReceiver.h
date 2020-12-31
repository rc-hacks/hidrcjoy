//
// PcmReceiver.h
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#pragma once
#include <atl/autolock.h>
#include <stdint.h>

/////////////////////////////////////////////////////////////////////////////

template<class timer>
class PcmReceiver : protected timer
{
    static const uint16_t minSyncPulseWidthUs = 750;
    static const uint8_t minChannelCount = 4;
    static const uint8_t maxChannelCount = 9;
    static const uint8_t timeoutMs = 100;

public:
    void SetConfiguration()
    {
        Initialize();
    }

    void Initialize(void)
    {
        timer::Initialize();
        Reset();
    }

    void Terminate(void)
    {
        timer::Terminate();
    }

    void Reset()
    {
        WaitForSync();

        m_currentBank = 0;
        m_channelCount = 0;
        m_isReceiving = false;
        m_hasNewData = false;
    }

    void RunTask()
    {
        if (m_timeoutCounter < timeoutMs)
        {
            m_timeoutCounter++;
        }
        else
        {
            m_timeoutCounter = 0;
            Reset();
        }
    }

    bool IsReceiving() const
    {
        return m_isReceiving;
    }

    bool HasNewData() const
    {
        return m_hasNewData;
    }

    void ClearNewData()
    {
        m_hasNewData = false;
    }

    uint8_t GetChannelCount() const
    {
        return m_channelCount;
    }

    uint8_t GetChannelData(uint8_t channel) const
    {
        return channel < m_channelCount ? m_channelData[m_currentBank ^ 1][channel] : 0x80;
    }

    void OnInputCapture()
    {
#if HIDRCJOY_DEBUG
        g_pinDebug10 = m_risingEdge;
        g_pinDebug11.Toggle();
#endif

        uint16_t time = timer::ICR();
        ProcessEdge(time);
    }

private:
    void ProcessEdge(uint16_t time)
    {
        uint16_t diff = time - m_timeOfLastEdge;
        m_timeOfLastEdge = time;

        if (m_state == State::WaitingForSync)
        {
            if (m_risingEdge)
            {
                m_risingEdge = false;
                timer::SetCaptureEdge(false);

                if (diff >= timer::UsToTicks(minSyncPulseWidthUs))
                {
                    m_state = State::SyncDetected;
                }
            }
            else
            {
                m_risingEdge = true;
                timer::SetCaptureEdge(true);
            }
        }
        else if (m_state == State::SyncDetected)
        {
#if HIDRCJOY_DEBUG
            g_pinDebug12 = true;
#endif

            m_state = State::ReceivingData;
            m_lastBits = 3;
            m_bitCount = 0;
            m_currentByte = 0;
            m_currentChannel = 0;
        }
        else if (m_state == State::ReceivingData)
        {
            bool abort = false;
            bool byteComplete = false;
            uint8_t offset = 3 - m_lastBits;
            uint8_t symbol = GetSymbol(diff);
            if (symbol >= offset)
            {
                uint8_t bits = symbol - offset;
                if (bits <= 3)
                {
                    if (m_bitCount >= 8)
                    {
                        if (CalculateChecksum(m_currentByte) == bits)
                        {
                            byteComplete = true;
                        }
                        else
                        {
                            abort = true;
                        }
                    }
                    else
                    {
                        m_bitCount += 2;
                        m_currentByte = (m_currentByte << 2) | bits;
                    }

                    m_lastBits = bits;
                }
                else
                {
                    byteComplete = true;
                }
            }
            else
            {
                byteComplete = true;
            }

            if (abort)
            {
                WaitForSync();
            }
            else if (byteComplete)
            {
                uint8_t currentChannel = m_currentChannel;
                if (currentChannel < maxChannelCount)
                {
                    m_channelData[m_currentBank][currentChannel] = m_currentByte;

                    currentChannel++;
                    if (currentChannel >= minChannelCount)
                    {
                        m_timeoutCounter = 0;
                        m_currentBank ^= 1;
                        m_channelCount = currentChannel;
                        m_isReceiving = true;
                        m_hasNewData = true;
                        WaitForSync();
                    }

                    m_currentChannel = currentChannel;
                }

                m_bitCount = 0;
                m_currentByte = 0;
            }
        }
    }

    void WaitForSync()
    {
#if HIDRCJOY_DEBUG
        g_pinDebug10 = false;
        g_pinDebug11 = false;
        g_pinDebug12 = false;
#endif

        timer::SetCaptureEdge(false);
        m_risingEdge = false;
        m_state = State::WaitingForSync;
    }

    static uint8_t GetSymbol(uint16_t width)
    {
        static const uint16_t S0 = 880;
        static const uint16_t S1 = 1020;
        static const uint16_t S2 = 1160;
        static const uint16_t S3 = 1300;
        static const uint16_t S4 = 1440;
        static const uint16_t S5 = 1580;
        static const uint16_t S6 = 1720;
        static const uint16_t W = 140 / 2;

        if (width < timer::UsToTicks(S3 - W))
        {
            if (width < timer::UsToTicks(S1 - W))
            {
                if (width < timer::UsToTicks(S0 - W))
                {
                    return 7;
                }
                else
                {
                    return 0;
                }

                return 0;
            }
            else
            {
                if (width < timer::UsToTicks(S2 - W))
                {
                    return 1;
                }
                else
                {
                    return 2;
                }
            }
        }
        else
        {
            if (width < timer::UsToTicks(S5 - W))
            {
                if (width < timer::UsToTicks(S4 - W))
                {
                    return 3;
                }
                else
                {
                    return 4;
                }
            }
            else
            {
                if (width < timer::UsToTicks(S6 - W))
                {
                    return 5;
                }
                else
                {
                    return 6;
                }
            }
        }
    }

    static uint8_t CalculateChecksum(uint8_t value)
    {
        return (3 ^ (value >> 6) ^ (value >> 4) ^ (value >> 2) ^ (value >> 0)) & 3;
    }

private:
    enum State : uint8_t
    {
        WaitingForSync,
        SyncDetected,
        ReceivingData,
    };

    volatile uint8_t m_channelData[2][maxChannelCount] = {};
    volatile uint16_t m_timeOfLastEdge = 0;
    volatile State m_state = State::WaitingForSync;
    volatile uint8_t m_lastBits = 3;
    volatile uint8_t m_bitCount = 0;
    volatile uint8_t m_currentByte = 0;
    volatile uint8_t m_currentBank = 0;
    volatile uint8_t m_currentChannel = 0;
    volatile uint8_t m_channelCount = 0;
    volatile uint8_t m_timeoutCounter = 0;
    volatile bool m_risingEdge = false;
    volatile bool m_isReceiving = false;
    volatile bool m_hasNewData = false;
};
