//
// usb_reports.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include "configuration.h"
#include <stdint.h>

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
    uint32_t m_updateRate;
    uint16_t m_channelPulseWidth[Configuration::maxOutputChannels];
};
