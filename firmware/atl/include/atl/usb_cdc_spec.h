//
// usb_cdc_spec.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <atl/compiler.h>
#include <stdint.h>

namespace atl
{
    static const uint16_t CdcVersion = 0x0110;

    static const uint8_t CdcDeviceClass = 0x02;

    // CDC120.pdf, 4 Class-Specific Codes
    static const uint8_t CdcInterfaceClassCommunications = 0x02;
    static const uint8_t CdcInterfaceSubclassCommunicationsAcm = 0x02;
    static const uint8_t CdcInterfaceProtocolCommunicationsNone = 0x00;

    static const uint8_t CdcInterfaceClassData = 0x0A;
    static const uint8_t CdcInterfaceSubclassDataNone = 0x00;
    static const uint8_t CdcInterfaceProtocolDataNone = 0x00;

    // CDC120.pdf, Table 12, Type Values for the bDescriptorType Field
    static const uint8_t CdcDescriptorTypeInterface = 0x24;
    static const uint8_t CdcDescriptorTypeEndpoint = 0x25;

    // CDC120.pdf, Table 13, bDescriptor SubType in Communications Class Functional Descriptors
    static const uint8_t CdcDescriptorSubtypeHeader = 0x00;
    static const uint8_t CdcDescriptorSubtypeCallManagement = 0x01;
    static const uint8_t CdcDescriptorSubtypeAbstractControlManagement = 0x02;
    static const uint8_t CdcDescriptorSubtypeUnion = 0x06;

    // PSTN120.pdf, Table 13, Class-Specific Request Codes for PSTN subclasses
    static const uint8_t CdcRequestSendEncapsulatedCommand = 0x00;
    static const uint8_t CdcRequestGetEncapsulatedResponse = 0x01;
    static const uint8_t CdcRequestSetCommFeature = 0x02;
    static const uint8_t CdcRequestGetCommFeature = 0x03;
    static const uint8_t CdcRequestClearCommFeature = 0x04;
    static const uint8_t CdcRequestSetAuxLineState = 0x10;
    static const uint8_t CdcRequestSetHookState = 0x11;
    static const uint8_t CdcRequestPulseSetup = 0x12;
    static const uint8_t CdcRequestSendPulse = 0x13;
    static const uint8_t CdcRequestSetPulseTime = 0x14;
    static const uint8_t CdcRequestRingAuxJack = 0x15;
    static const uint8_t CdcRequestSetLineCoding = 0x20;
    static const uint8_t CdcRequestGetLineCoding = 0x21;
    static const uint8_t CdcRequestSetControlLineState = 0x22;
    static const uint8_t CdcRequestSendBreak = 0x23;
    static const uint8_t CdcRequestSetRingerParms = 0x30;
    static const uint8_t CdcRequestGetRingerParms = 0x31;
    static const uint8_t CdcRequestSetOperationParms = 0x32;
    static const uint8_t CdcRequestGetOperationParms = 0x33;
    static const uint8_t CdcRequestSetLineParms = 0x34;
    static const uint8_t CdcRequestGetLineParms = 0x35;
    static const uint8_t CdcRequestDialDigits = 0x36;

    // CDC120.pdf, Table 15, Class-Specific Descriptor Header Format  
    struct ATL_ATTRIBUTE_PACKED CdcHeaderFunctionalDescriptor
    {
        uint8_t bFunctionLength;
        uint8_t bDescriptorType;
        uint8_t bDescriptorSubtype;
        uint16_t bcdCDC;
    };

    // CDC120.pdf, Table 15, Class-Specific Descriptor Header Format  
    struct ATL_ATTRIBUTE_PACKED CdcUnionFunctionalDescriptor
    {
        uint8_t bFunctionLength;
        uint8_t bDescriptorType;
        uint8_t bDescriptorSubtype;
        uint8_t bControlInterface;
        uint8_t bSubordinateInterface0;
    };

    // PSTN120.pdf, Table 3, Call Management Functional Descriptor  
    struct ATL_ATTRIBUTE_PACKED CdcCallManagementFunctionalDescriptor
    {
        uint8_t bFunctionLength;
        uint8_t bDescriptorType;
        uint8_t bDescriptorSubtype;
        uint8_t bmCapabilities;
        uint8_t bDataInterface;
    };

    // PSTN120.pdf, Table 4, Abstract Control Management Functional Descriptor 
    struct ATL_ATTRIBUTE_PACKED CdcAbstractControlManagementFunctionalDescriptor
    {
        uint8_t bFunctionLength;
        uint8_t bDescriptorType;
        uint8_t bDescriptorSubtype;
        uint8_t bmCapabilities;
    };

    // PSTN120.pdf, Table 17, Line Coding Structure
    struct ATL_ATTRIBUTE_PACKED CdcLineCoding
    {
        uint32_t dwDTERate;
        uint8_t bCharFormat;
        uint8_t bParityType;
        uint8_t bDataBits;
    };

    enum class CdcStopBits : uint8_t
    {
        One = 0,
        OnePointFive = 1,
        Two = 2,
    };

    enum class CdcParity : uint8_t
    {
        None = 0,
        Odd = 1,
        Even = 2,
        Mark = 3,
        Space = 4,
    };

    // PSTN120.pdf, Table 18, Control Signal Bitmap Values for SetControlLineState
    static const uint8_t CdcControlLineStateDtr = 0x01;
    static const uint8_t CdcControlLineStateRts = 0x02;
};
