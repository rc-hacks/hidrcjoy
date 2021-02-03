//
// ppm_receiver.h
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#pragma once
#include <stdint.h>
#include <avr/interrupt.h>

/////////////////////////////////////////////////////////////////////////////

template<typename T, typename timer>
class PpmReceiverT
{
    static const uint8_t minChannelCount = 4;
    static const uint8_t maxChannelCount = 9;
    static const uint16_t defaultSyncPulseWidthUs = 3500;
    static const uint8_t timeoutMs = 100;

public:
    void Initialize()
    {
        timer::Initialize();
        timer::OCR() = timer::TCNT() + m_minSyncPulseWidth;
        Reset();
    }

    void Terminate()
    {
        timer::Terminate();
    }

    void SetConfiguration(uint16_t minSyncPulseWidthUs, bool invertedSignal)
    {
        m_minSyncPulseWidth = timer::UsToTicks(minSyncPulseWidthUs);
        m_invertedSignal = invertedSignal;
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

    uint16_t GetChannelTicks(uint8_t channel) const
    {
        atl::AutoLock lock;
        return m_pulseWidth[m_currentBank ^ 1][channel];
    }

    uint16_t GetChannelPulseWidth(uint8_t channel) const
    {
        return channel < m_channelCount ? timer::TicksToUs(GetChannelTicks(channel)) : 0;
    }

    void OnInputCapture(uint16_t time, bool risingEdge)
    {
        if (risingEdge != m_invertedSignal)
            return;

        timer::OCR() = time + m_minSyncPulseWidth;
        ProcessEdge(time);
    }

    void OnOutputCompare()
    {
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
            ProcessFrame();
        }

        m_state = State::SyncDetected;
        static_cast<T*>(this)->OnSyncDetected();
    }

    void ProcessFrame()
    {
        uint8_t currentChannel = m_currentChannel;
        if (currentChannel >= minChannelCount)
        {
            m_timeoutCounter = 0;
            m_currentBank ^= 1;
            m_channelCount = currentChannel;
            m_isReceiving = true;
            m_hasNewData = true;
            static_cast<T*>(this)->OnFrameReceived();
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