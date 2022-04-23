//
// usb_hid_spec.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <atl/compiler.h>
#include <stdint.h>

namespace atl
{
    static const uint16_t HidVersion = 0x0111;

    static const uint8_t HidInterfaceClass = 0x03;

    // HID1_11.pdf, 4.2 Subclass 
    static const uint8_t HidInterfaceSubclassNone = 0;
    static const uint8_t HidInterfaceSubclassBootInterface = 1;

    // HID1_11.pdf, 4.3 Protocols 
    static const uint8_t HidInterfaceProtocolNone = 0;
    static const uint8_t HidInterfaceProtocolKeyboard = 1;
    static const uint8_t HidInterfaceProtocolMouse = 2;

    // HID1_11.pdf, 7.1 Standard Requests
    static const uint8_t HidDescriptorTypeHid = 0x21;
    static const uint8_t HidDescriptorTypeReport = 0x22;
    static const uint8_t HidDescriptorTypePhysical = 0x23;

    // HID1_11.pdf, 7.2 Class-Specific Requests
    static const uint8_t HidRequestGetReport = 0x01;
    static const uint8_t HidRequestGetIdle = 0x02;
    static const uint8_t HidRequestGetProtocol = 0x03;
    static const uint8_t HidRequestSetReport = 0x09;
    static const uint8_t HidRequestSetIdle = 0x0A;
    static const uint8_t HidRequestSetProtocol = 0x0B;

    // HID1_11.pdf, 7.2 Class-Specific Requests
    static const uint8_t HidProtocolBoot = 0;
    static const uint8_t HidProtocolReport = 1;

    // HID1_11.pdf, 6.2.1 HID Descriptor
    struct ATL_ATTRIBUTE_PACKED HidDescriptor
    {
        uint8_t bLength;
        uint8_t bDescriptorType;
        uint16_t bcdHID;
        uint8_t bCountryCode;
        uint8_t bNumDescriptors;
        uint8_t bClassDescriptorType;
        uint16_t wDescriptorLength;
    };
};
