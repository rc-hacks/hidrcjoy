//
// SrxlReceiver.h
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#pragma once
#include <stdint.h>

/////////////////////////////////////////////////////////////////////////////

template<class timer, class usart>
class SrxlReceiver : protected timer, protected usart
{
    static const uint8_t maxChannelCount = 16;
    static const uint32_t baudrate = 115200;
    static const uint8_t headerV1 = 0xA1;
    static const uint8_t headerV2 = 0xA2;
    static const uint16_t syncPauseUs = 5000;
    static const uint16_t timeoutTickUs = 1000;
    static const uint8_t timeoutTicks = 25;

    enum FrameStatus : uint8_t
    {
        Special = 0xF0,
        Empty,
        Ready,
        Ok,
        Error,
    };

public:
    void Initialize(void)
    {
        timer::Initialize();
        usart::Initialize(baudrate);

        auto tcnt = timer::TCNT();
        timer::OCRA() = tcnt + timer::UsToTicks(syncPauseUs);
        timer::OCRB() = tcnt + timer::UsToTicks(timeoutTickUs);

        Reset();
    }

    void Terminate(void)
    {
        timer::Terminate();
        usart::Terminate();
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

        return DataToUs(channelData);
    }

    void OnDataReceived(uint8_t ch)
    {
        timer::OCRA() = timer::TCNT() + timer::UsToTicks(syncPauseUs);
        AddByteToFrame(ch);
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
    void AddByteToFrame(uint8_t ch)
    {
        if (m_state == State::SyncDetected)
        {
            m_state = State::ReceivingData;
            m_bytesReceived = 0;
        }

        if (m_state == State::ReceivingData)
        {
            if (m_bytesReceived < sizeof(m_frame))
            {
                m_frame[m_bytesReceived++] = ch;

                if (m_frame[0] == headerV1 && m_bytesReceived == 1 + 12 * 2 + 2)
                {
                    DecodeDataFrame(12);
                }
                else if (m_frame[0] == headerV2 && m_bytesReceived == 1 + 16 * 2 + 2)
                {
                    DecodeDataFrame(16);
                }

                m_timeoutCounter = 0;
            }
        }
    }

    void DecodeDataFrame(uint8_t channelCount)
    {
        uint16_t crc = GetUInt16(m_frame, m_bytesReceived - 2);
        if (CalculateCrc16(m_frame, m_bytesReceived - 2) == crc)
        {
            for (uint8_t i = 0; i < channelCount; i++)
            {
                m_channelData[i] = GetUInt16(m_frame, 1 + i * 2);
            }

            m_channelCount = channelCount;
            m_isReceiving = true;;
            m_hasNewData = true;;
            m_timeoutCounter = 0;
            m_state = State::SyncDetected;
        }
        else
        {
            m_status = Error;
        }
    }

    void ProcessSyncPause()
    {
        m_state = State::SyncDetected;
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

    static uint16_t GetUInt16(const uint8_t* data, uint8_t index)
    {
        return (data[index] << 8) | data[index + 1];
    }

    static uint16_t DataToUs(uint16_t value)
    {
        return 800 + static_cast<uint16_t>(static_cast<uint32_t>(value & 0xFFF) * (2200 - 800) / 0x1000);
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
    enum State : uint8_t
    {
        WaitingForSync,
        SyncDetected,
        ReceivingData,
    };

    uint8_t m_frame[1 + 16 * 2 + 2] = {};
    volatile uint16_t m_channelData[maxChannelCount] = {};
    volatile uint8_t m_status = 0;
    volatile uint8_t m_bytesReceived = 0;
    volatile uint8_t m_channelCount = 0;
    volatile uint8_t m_timeoutCounter = 0;
    volatile State m_state = State::WaitingForSync;
    volatile bool m_isReceiving = false;
    volatile bool m_hasNewData = false;
};
