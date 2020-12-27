//
// PpmReceiver.h
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#pragma once
#include <atl/autolock.h>
#include <stdint.h>

/////////////////////////////////////////////////////////////////////////////

template<class timer>
class PpmReceiver : protected timer
{
    static const uint8_t minChannelCount = 4;
    static const uint8_t maxChannelCount = 9;
    static const uint16_t timeoutTickUs = 1000;
    static const uint8_t timeoutTicks = 100;

public:
    void SetConfiguration(uint16_t minSyncPulseWidthUs, bool invertedSignal)
    {
        m_minSyncPulseWidth = timer::UsToTicks(minSyncPulseWidthUs);
        m_invertedSignal = invertedSignal;
        Initialize();
    }

    void Initialize(void)
    {
        timer::Initialize(m_invertedSignal);

        auto tcnt = timer::TCNT();
        timer::OCRA() = tcnt + m_minSyncPulseWidth;
        timer::OCRB() = tcnt + timer::UsToTicks(timeoutTickUs);

        Reset();
    }

    void Terminate(void)
    {
        timer::Terminate();
    }

    void Reset()
    {
        m_state = State::WaitingForSync;
        m_isReceiving = false;
        m_hasNewData = false;
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

    uint16_t GetChannelData(uint8_t channel) const
    {
        uint16_t channelData;
        {
            atl::AutoLock lock;
            channelData = m_channelData[channel];
        }

        return timer::TicksToUs(channelData);
    }

    void OnInputCapture()
    {
        uint16_t time = timer::ICR();
        timer::OCRA() = time + m_minSyncPulseWidth;
        ProcessEdge(time);
    }

    void OnOutputCompareA()
    {
        ProcessSyncPause();
    }

    void OnOutputCompareB()
    {
        timer::OCRB() += timer::UsToTicks(timeoutTickUs);
        ProcessSignalTimeout();
    }

private:
    void ProcessEdge(uint16_t time)
    {
        uint16_t diff = time - m_timeOfLastEdge;
        m_timeOfLastEdge = time;

        if (m_state == State::SyncDetected)
        {
            m_state = State::ReceivingData;
            m_currentChannel = 0;
        }
        else if (m_state == State::ReceivingData)
        {
            uint8_t currentChannel = m_currentChannel;
            if (currentChannel < maxChannelCount)
            {
                m_pulseWidth[currentChannel] = diff;
                m_currentChannel = currentChannel + 1;;
            }
        }
    }

    void ProcessSyncPause()
    {
        if (m_state == State::WaitingForSync)
        {
            m_state = State::SyncDetected;
        }
        else if (m_state == State::ReceivingData)
        {
            uint8_t channelCount = m_currentChannel;
            if (channelCount >= minChannelCount)
            {
                for (uint8_t i = 0; i < maxChannelCount; i++)
                {
                    m_channelData[i] = i < channelCount ? m_pulseWidth[i] : 0;
                }

                m_channelCount = channelCount;
                m_isReceiving = true;
                m_hasNewData = true;
                m_timeoutCounter = 0;
                m_state = State::SyncDetected;
            }
        }
    }

    void ProcessSignalTimeout()
    {
        if (m_timeoutCounter < timeoutTicks)
        {
            m_timeoutCounter++;
        }
        else
        {
            m_timeoutCounter = 0;
            Reset();
        }
    }

private:
    enum State : uint8_t
    {
        WaitingForSync,
        SyncDetected,
        ReceivingData,
    };

    volatile uint16_t m_pulseWidth[maxChannelCount] = {};
    volatile uint16_t m_channelData[maxChannelCount] = {};
    volatile uint16_t m_minSyncPulseWidth = 0;
    volatile uint16_t m_timeOfLastEdge = 0;
    volatile uint8_t m_currentChannel = 0;
    volatile uint8_t m_channelCount = 0;
    volatile uint8_t m_timeoutCounter = 0;
    volatile State m_state = State::WaitingForSync;
    volatile bool m_invertedSignal = false;
    volatile bool m_isReceiving = false;
    volatile bool m_hasNewData = false;
};
