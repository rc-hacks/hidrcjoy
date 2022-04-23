//
// ppm_receiver.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <atl/autolock.h>
#include <stdint.h>

template<typename T, typename timer>
class PpmReceiverT
{
    static const uint8_t minChannelCount = 4;
    static const uint8_t maxChannelCount = 9;
    static const uint8_t maxTimeoutCount = 50;
    static const uint16_t defaultSyncPulseWidthUs = 3500;

public:
    using Timer = timer;

    void Initialize()
    {
        timer::Initialize();
        timer::OCR() = timer::TCNT() + m_minSyncPulseWidth;
        Reset();
    }

    void Reset()
    {
        m_state = State::WaitingForSync;
        m_currentBank = 0;
        m_currentChannel = 0;
        m_channelCount = 0;
        m_timeoutCount = maxTimeoutCount;
        m_hasNewData = false;
    }

    void SetMinSyncPulseWidth(uint16_t minSyncPulseWidthUs)
    {
        m_minSyncPulseWidth = timer::UsToTicks(minSyncPulseWidthUs);
    }

    bool IsReceiving() const
    {
        return m_timeoutCount < maxTimeoutCount;
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

    uint16_t GetChannelTicks(uint8_t channel) const
    {
        if (channel >= m_channelCount)
            return 0;

        atl::AutoLock lock;
        return m_pulseWidth[m_currentBank ^ 1][channel];
    }

    uint16_t GetChannelPulseWidth(uint8_t channel) const
    {
        uint16_t ticks = GetChannelTicks(channel);
        if (ticks == 0)
            return 0;

        return timer::TicksToUs(ticks);
    }

    void OnInputEdge(uint16_t time)
    {
        timer::OCR() = time + m_minSyncPulseWidth;
        ProcessEdge(time);
    }

    void OnOutputCompare()
    {
        timer::OCR() += m_minSyncPulseWidth;
        ProcessSyncPause();
    }

protected:
    void OnSyncDetected()
    {
    }

    void OnFrameReceived()
    {
    }

private:
    void ProcessEdge(uint16_t time)
    {
        uint16_t diff = time - m_timeOfLastEdge;
        m_timeOfLastEdge = time;

        State state = m_state;
        if (state == State::SyncDetected)
        {
            m_state = State::ReceivingData;
        }
        else if (state == State::ReceivingData)
        {
            uint8_t currentChannel = m_currentChannel;
            if (currentChannel < maxChannelCount)
            {
                m_pulseWidth[m_currentBank][currentChannel] = diff;
                m_currentChannel = currentChannel + 1;
            }
        }
    }

    void ProcessSyncPause()
    {
        uint8_t currentChannel = m_currentChannel;
        if (currentChannel >= minChannelCount)
        {
            m_currentBank ^= 1;
            m_channelCount = currentChannel;
            m_timeoutCount = 0;
            m_hasNewData = true;
            static_cast<T*>(this)->OnFrameReceived();
        }
        else
        {
            if (m_timeoutCount < maxTimeoutCount)
            {
                m_timeoutCount++;
            }
            else
            {
                m_channelCount = 0;
            }
        }

        m_state = State::SyncDetected;
        m_currentChannel = 0;
        static_cast<T*>(this)->OnSyncDetected();
    }

private:
    enum State : uint8_t
    {
        WaitingForSync,
        SyncDetected,
        ReceivingData,
    };

    volatile uint16_t m_pulseWidth[2][maxChannelCount] = {};
    volatile uint16_t m_minSyncPulseWidth = timer::UsToTicks(defaultSyncPulseWidthUs);
    volatile uint16_t m_timeOfLastEdge = 0;
    volatile State m_state = State::WaitingForSync;
    volatile uint8_t m_currentBank = 0;
    volatile uint8_t m_currentChannel = 0;
    volatile uint8_t m_channelCount = 0;
    volatile uint8_t m_timeoutCount = 0;
    volatile bool m_hasNewData = false;
};
