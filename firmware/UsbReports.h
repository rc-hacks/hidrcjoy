//
// UsbReports.h
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#pragma once
#include <stdint.h>
#include "Configuration.h"

/////////////////////////////////////////////////////////////////////////////

enum ReportId
{
    UnusedId,
    UsbReportId,
    UsbEnhancedReportId,
    ConfigurationReportId,
    LoadConfigurationDefaultsId,
    ReadConfigurationFromEepromId,
    WriteConfigurationToEepromId,
    JumpToBootloaderId,
};

enum class SignalSource : uint8_t
{
    None,
    PPM,
    PCM,
    SRXL,
};

struct UsbReport
{
    uint8_t m_reportId;
    uint8_t m_value[Configuration::maxOutputChannels];
};

static_assert(sizeof(UsbReport) <= 8, "Report size for low-speed devices may not exceed 8 bytes");

struct UsbEnhancedReport
{
    uint8_t m_reportId;
    SignalSource m_signalSource;
    uint8_t m_channelCount;
    uint8_t m_dummy;
    uint16_t m_channelPulseWidth[Configuration::maxOutputChannels];
};
