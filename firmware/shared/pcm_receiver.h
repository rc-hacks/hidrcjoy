//
// pcm_receiver.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <stdint.h>

template<typename T, typename timer>
class PcmReceiverT
{
    static const uint16_t minSyncPulseWidthUs = 750;
    static const uint8_t minChannelCount = 4;
    static const uint8_t maxChannelCount = 10;
    static const uint8_t timeoutMs = 100;

public:
    void Initialize()
    {
        timer::Initialize();
        Reset();
    }

    void Reset()
    {
        m_state = State::WaitingForSync;
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
        if (channel >= m_channelCount)
            return 0x80;

        return ~m_channelData[m_currentBank ^ 1][channel];
    }

    uint16_t GetChannelPulseWidth(uint8_t channel) const
    {
        uint16_t value = GetChannelData(channel);
        return 1050 + 138 * value / 0x20;
    }

    void OnInputEdge(uint16_t time, bool risingEdge)
    {
        ProcessEdge(time, risingEdge);
    }

protected:
    void OnSyncDetected()
    {
    }

    void OnFrameReceived()
    {
    }

    void OnError()
    {
    }

private:
    void ProcessEdge(uint16_t time, bool risingEdge)
    {
        uint16_t diff = time - m_timeOfLastFallingEdge;

        if (risingEdge)
        {
            if (diff >= timer::UsToTicks(minSyncPulseWidthUs))
            {
                m_state = State::SyncDetected;
                static_cast<T*>(this)->OnSyncDetected();
            }
        }
        else
        {
            m_timeOfLastFallingEdge = time;

            if (m_state == State::SyncDetected)
            {
                m_state = State::ReceivingData;
                m_lastBits = 3;
                m_bitCount = 0;
                m_currentData = 0;
                m_currentChannel = 0;
            }
            else if (m_state == State::ReceivingData)
            {
                uint8_t symbol = GetSymbol(diff);
                uint8_t bits = symbol - (3 - m_lastBits);
                if (bits <= 3)
                {
                    if (m_bitCount >= 8)
                    {
                        if (CalculateChecksum(m_currentData) == bits)
                        {
                            uint8_t currentChannel = m_currentChannel;
                            if (currentChannel < maxChannelCount)
                            {
                                m_channelData[m_currentBank][currentChannel] = m_currentData;
                                m_currentChannel = currentChannel + 1;
                            }

                            m_bitCount = 0;
                            m_currentData = 0;
                        }
                        else
                        {
                            m_state = State::WaitingForSync;
                            static_cast<T*>(this)->OnError();
                        }
                    }
                    else
                    {
                        m_bitCount += 2;
                        m_currentData = (m_currentData << 2) | bits;

                        if (m_currentChannel == 8 && m_bitCount == 4)
                        {
                            ProcessFrame();
                            m_state = State::WaitingForSync;
                        }
                    }

                    m_lastBits = bits;
                }
            }
        }
    }

    void ProcessFrame()
    {
        if (m_currentData == 0xC)
        {
            // Value 6 and 7 contains channel 6 and 7.
        }
        else if (m_currentData == 0x9)
        {
            // Value 6 and 7 contains channel 8 and 9.
            m_channelData[m_currentBank][8] = m_channelData[m_currentBank][6];
            m_channelData[m_currentBank][9] = m_channelData[m_currentBank][7];
            m_channelData[m_currentBank][6] = m_channelData[m_currentBank ^ 1][6];
            m_channelData[m_currentBank][7] = m_channelData[m_currentBank ^ 1][7];
        }

        m_timeoutCounter = 0;
        m_currentBank ^= 1;
        m_channelCount = m_currentChannel + 2;
        m_isReceiving = true;
        m_hasNewData = true;
        static_cast<T*>(this)->OnFrameReceived();
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
    volatile uint16_t m_timeOfLastFallingEdge = 0;
    volatile State m_state = State::WaitingForSync;
    volatile uint8_t m_lastBits = 3;
    volatile uint8_t m_bitCount = 0;
    volatile uint8_t m_currentData = 0;
    volatile uint8_t m_currentBank = 0;
    volatile uint8_t m_currentChannel = 0;
    volatile uint8_t m_channelCount = 0;
    volatile uint8_t m_timeoutCounter = 0;
    volatile bool m_risingEdge = false;
    volatile bool m_isReceiving = false;
    volatile bool m_hasNewData = false;
};
