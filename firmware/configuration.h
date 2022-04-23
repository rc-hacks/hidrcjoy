//
// configuration.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <stdint.h>

#define MAX_CHANNELS 7

struct Configuration
{
#ifdef __cplusplus
    static const uint8_t version = 0x11;
    static const uint8_t maxInputChannels = 7;
    static const uint8_t maxOutputChannels = 7;
    static const uint16_t minSyncWidth = 2000;
    static const uint16_t maxSyncWidth = 10000;
    static const uint16_t minChannelPulseWidth = 500;
    static const uint16_t maxChannelPulseWidth = 3000;

    enum Flags
    {
        InvertedSignal = 1,
    };
#endif

    uint8_t m_reportId;
    uint8_t m_version;
    uint8_t m_flags;
    uint8_t m_dummy;
    uint16_t m_minSyncPulseWidth;
    uint16_t m_centerChannelPulseWidth;
    uint16_t m_channelPulseWidthRange;
    uint8_t m_polarity;
    uint8_t m_mapping[MAX_CHANNELS];
};
