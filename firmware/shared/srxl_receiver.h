//
// srxl_receiver.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <stdint.h>
#include <atl/autolock.h>

template<typename T, typename timer, typename usart>
class SrxlReceiverT
{
    static const uint8_t maxChannelCount = 16;
    static const uint32_t baudrate = 115200;
    static const uint8_t headerV1 = 0xA1;
    static const uint8_t headerV2 = 0xA2;
    static const uint16_t syncPauseUs = 5000;
    static const uint8_t timeoutMs = 100;

    enum FrameStatus : uint8_t
    {
        Special = 0xF0,
        Empty,
        Ready,
        Ok,
        Error,
    };

public:
    void Initialize()
    {
        timer::Initialize();
        timer::OCR() = timer::TCNT() + timer::UsToTicks(syncPauseUs);
        usart::Initialize(baudrate);
        Reset();
    }

    void Reset()
    {
        m_state = State::WaitingForSync;
        m_currentBank = 0;
        m_bytesReceived = 0;
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
        if (channel >= m_channelCount)
            return 0;

        auto frame = m_frame[m_currentBank ^ 1];
        auto index = 1 + channel * 2;

        uint16_t value;
        {
            atl::AutoLock lock;
            value = GetUInt16(frame, index);
        }

        return DataToUs(value);
    }

    void OnDataReceived(uint8_t ch)
    {
        timer::OCR() = timer::TCNT() + timer::UsToTicks(syncPauseUs);
        AddByteToFrame(ch);
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

    void OnError()
    {
    }

private:
    void AddByteToFrame(uint8_t ch)
    {
        if (m_state == State::SyncDetected)
        {
            m_state = State::ReceivingData;
            m_bytesReceived = 0;
        }

        if (m_state == State::ReceivingData)
        {
            if (m_bytesReceived < sizeof(m_frame[0]))
            {
                auto frame = m_frame[m_currentBank];
                frame[m_bytesReceived++] = ch;

                if (frame[0] == headerV1 && m_bytesReceived == 1 + 12 * 2 + 2)
                {
                    ProcessFrame(frame, 12);
                }
                else if (frame[0] == headerV2 && m_bytesReceived == 1 + 16 * 2 + 2)
                {
                    ProcessFrame(frame, 16);
                }
            }
        }
    }

    void ProcessSyncPause()
    {
        m_state = State::SyncDetected;
        static_cast<T*>(this)->OnSyncDetected();
    }

    void ProcessFrame(const volatile uint8_t* frame, uint8_t channelCount)
    {
        uint16_t crc = GetUInt16(frame, m_bytesReceived - 2);
        if (CalculateCrc16(frame, m_bytesReceived - 2) == crc)
        {
            m_timeoutCounter = 0;
            m_currentBank ^= 1;
            m_channelCount = channelCount;
            m_isReceiving = true;
            m_hasNewData = true;
            static_cast<T*>(this)->OnFrameReceived();
            m_state = State::SyncDetected;
        }
        else
        {
            m_state = State::WaitingForSync;
            static_cast<T*>(this)->OnError();
        }
    }

    static uint16_t GetUInt16(const volatile uint8_t* data, uint8_t index)
    {
        return (data[index] << 8) | data[index + 1];
    }

    static uint16_t DataToUs(uint16_t value)
    {
        return 800 + static_cast<uint16_t>((static_cast<uint32_t>(value & 0xFFF) * 1400 + 0x800) / 0x1000);
    }

    static uint16_t CalculateCrc16(const volatile uint8_t* data, uint8_t count)
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
    enum State : uint8_t
    {
        WaitingForSync,
        SyncDetected,
        ReceivingData,
    };

    volatile uint8_t m_frame[2][1 + 16 * 2 + 2] = {};
    volatile State m_state = State::WaitingForSync;
    volatile uint8_t m_bytesReceived = 0;
    volatile uint8_t m_currentBank = 0;
    volatile uint8_t m_channelCount = 0;
    volatile uint8_t m_timeoutCounter = 0;
    volatile bool m_isReceiving = false;
    volatile bool m_hasNewData = false;
};
