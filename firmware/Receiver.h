//
// Receiver.h
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#pragma once
#include <stdint.h>
#include "Configuration.h"
#include "UsbReports.h"
#include "PpmReceiver.h"
#if HIDRCJOY_SRXL
#include "SrxlReceiver.h"
#endif
/////////////////////////////////////////////////////////////////////////////

class Receiver
{
public:
    void Initialize()
    {
        g_PpmReceiver.Initialize();
#if HIDRCJOY_SRXL
        g_SrxlReceiver.Initialize();
#endif
    }

    void LoadDefaultConfiguration()
    {
        m_Configuration.m_version = Configuration::version;
        m_Configuration.m_flags = 0;
        m_Configuration.m_minSyncPulseWidth = 3500;
        m_Configuration.m_centerChannelPulseWidth = 1500;
        m_Configuration.m_channelPulseWidthRange = 550;
        m_Configuration.m_polarity = 0;

        for (uint8_t i = 0; i < sizeof(m_Configuration.m_mapping); i++)
        {
            m_Configuration.m_mapping[i] = i;
        }
    }

    void UpdateConfiguration()
    {
        g_PpmReceiver.SetConfiguration(m_Configuration.m_minSyncPulseWidth, (m_Configuration.m_flags & Configuration::Flags::InvertedSignal) != 0);
    }

    bool IsValidConfiguration() const
    {
        if (m_Configuration.m_version != Configuration::version)
            return false;

        if (m_Configuration.m_minSyncPulseWidth < Configuration::minSyncWidth ||
            m_Configuration.m_minSyncPulseWidth > Configuration::maxSyncWidth)
            return false;

        if (m_Configuration.m_centerChannelPulseWidth < Configuration::minChannelPulseWidth ||
            m_Configuration.m_centerChannelPulseWidth > Configuration::maxChannelPulseWidth)
            return false;

        if (m_Configuration.m_channelPulseWidthRange < 10 ||
            m_Configuration.m_channelPulseWidthRange > Configuration::maxChannelPulseWidth)
            return false;

        for (uint8_t i = 0; i < sizeof(m_Configuration.m_mapping); i++)
        {
            if (m_Configuration.m_mapping[i] >= Configuration::maxChannels)
            {
                return false;
            }
        }

        return true;
    }

    void Update()
    {
        //g_PpmReceiver.Update();
#if HIDRCJOY_SRXL
        g_SrxlReceiver.Update(TCNT1);
#endif
    }

    uint16_t GetChannelData(uint8_t channel) const
    {
        uint8_t index = m_Configuration.m_mapping[channel];
        
        if (g_PpmReceiver.IsReceiving())
        {
            return g_PpmReceiver.GetChannelData(index);
        }
#if HIDRCJOY_SRXL
        else if (g_SrxlReceiver.IsReceiving())
        {
            return g_SrxlReceiver.GetChannelData(index);
        }
#endif
        else
        {
            return 0;
        }
    }

    uint8_t GetStatus() const
    {
        if (g_PpmReceiver.IsReceiving())
        {
            return Status::PpmSignal;
        }
#if HIDRCJOY_SRXL
        else if (g_SrxlReceiver.IsReceiving())
        {
            return Status::SrxlSignal;
        }
#endif
        else
        {
            return Status::NoSignal;
        }
    }

    bool IsReceiving() const
    {
        return g_PpmReceiver.IsReceiving();
    }

    bool HasNewData() const
    {
        return g_PpmReceiver.HasNewData();
    }

    void AcknowledgeNewData()
    {
        g_PpmReceiver.AcknowledgeNewData();
    }

    uint8_t GetValue(uint8_t channel) const
    {
        return ScaleValue(channel, GetChannelData(channel));
    }

private:
    int16_t ScaleValue(uint8_t channel, int16_t data) const
    {
        int16_t center = m_Configuration.m_centerChannelPulseWidth;
        int16_t range = m_Configuration.m_channelPulseWidthRange;
        int16_t value = InvertValue(channel, data - center);
        return SaturateValue(128 + (128 * (int32_t)value / range));
    }

    int16_t InvertValue(uint8_t channel, int16_t value) const
    {
        return (m_Configuration.m_polarity & (1 << channel)) == 0 ? value : -value;
    }

    uint8_t SaturateValue(int32_t value) const
    {
        if (value < 0)
        {
            return 0;
        }
        else if (value > 255)
        {
            return 255;
        }
        else
        {
            return value;
        }
    }

public:
    Configuration m_Configuration;
};
