//
// UsbReports.h
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#pragma once
#include <stdint.h>
#include "Configuration.h"

/////////////////////////////////////////////////////////////////////////////

enum ReportIds
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

enum Status
{
    NoSignal,
    PpmSignal,
    SrxlSignal,
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
    uint8_t m_status;
    uint16_t m_channelPulseWidth[Configuration::maxOutputChannels];
};
