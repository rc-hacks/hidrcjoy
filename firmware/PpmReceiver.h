//
// PpmReceiver.h
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#pragma once
#include <stdint.h>

/////////////////////////////////////////////////////////////////////////////

template<class timer>
class PpmReceiver : protected timer
{
    static const uint8_t minChannelCount = 4;
    static const uint8_t maxChannelCount = 9;
    static const uint16_t defaultSyncPulseWidthUs = 3500;
    static const uint8_t timeoutMs = 100;

public:
    void Initialize(void)
    {
        timer::Initialize(m_invertedSignal);
        timer::OCR() = timer::TCNT() + m_minSyncPulseWidth;
        Reset();
    }

    void Terminate(void)
    {
        timer::Terminate();
    }

    void SetConfiguration(uint16_t minSyncPulseWidthUs, bool invertedSignal)
    {
        m_minSyncPulseWidth = timer::UsToTicks(minSyncPulseWidthUs);
        m_invertedSignal = invertedSignal;
        Initialize();
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

    uint16_t GetChannelPulseWidth(uint8_t channel) const
    {
        return channel < m_channelCount ? timer::TicksToUs(m_pulseWidth[m_currentBank ^ 1][channel]) : 0;
    }

    void OnInputCapture()
    {
        uint16_t time = timer::ICR();
        timer::OCR() = time + m_minSyncPulseWidth;
        ProcessEdge(time);
    }

    void OnOutputCompare()
    {
        ProcessSyncPause();
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
                m_pulseWidth[m_currentBank][currentChannel] = diff;
                m_currentChannel = currentChannel + 1;
            }
        }
    }

    void ProcessSyncPause()
    {
        if (m_state == State::ReceivingData)
        {
            FinishFrame();
        }

        m_state = State::SyncDetected;
    }

    void FinishFrame()
    {
        uint8_t currentChannel = m_currentChannel;
        if (currentChannel >= minChannelCount)
        {
            m_timeoutCounter = 0;
            m_currentBank ^= 1;
            m_channelCount = currentChannel;
            m_isReceiving = true;
            m_hasNewData = true;
        }
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
    volatile uint8_t m_timeoutCounter = 0;
    volatile bool m_invertedSignal = false;
    volatile bool m_isReceiving = false;
    volatile bool m_hasNewData = false;
};
