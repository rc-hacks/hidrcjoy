//
// usb_spec.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <atl/compiler.h>
#include <stdint.h>

namespace atl
{
    // usb_20.pdf, Table 9-2, Format of Setup Data
    static const uint8_t UsbRequestTypeDirection = 0x80;
    static const uint8_t UsbRequestTypeHostToDevice = 0x00;
    static const uint8_t UsbRequestTypeDeviceToHost = 0x80;

    static const uint8_t UsbRequestTypeType = 0x60;
    static const uint8_t UsbRequestTypeStandard = 0x00;
    static const uint8_t UsbRequestTypeClass = 0x20;
    static const uint8_t UsbRequestTypeVendor = 0x40;

    static const uint8_t UsbRequestTypeRecipient = 0x1F;
    static const uint8_t UsbRequestTypeDevice = 0x00;
    static const uint8_t UsbRequestTypeInterface = 0x01;
    static const uint8_t UsbRequestTypeEndpoint = 0x02;
    static const uint8_t UsbRequestTypeOther = 0x03;

    // bmRequestType Combinations
    static const uint8_t RequestTypeStandardDeviceIn = UsbRequestTypeDeviceToHost | UsbRequestTypeStandard | UsbRequestTypeDevice;
    static const uint8_t RequestTypeStandardDeviceOut = UsbRequestTypeHostToDevice | UsbRequestTypeStandard | UsbRequestTypeDevice;
    static const uint8_t RequestTypeStandardInterfaceIn = UsbRequestTypeDeviceToHost | UsbRequestTypeStandard | UsbRequestTypeInterface;
    static const uint8_t RequestTypeStandardInterfaceOut = UsbRequestTypeHostToDevice | UsbRequestTypeStandard | UsbRequestTypeInterface;
    static const uint8_t RequestTypeStandardEndpointIn = UsbRequestTypeDeviceToHost | UsbRequestTypeStandard | UsbRequestTypeEndpoint;
    static const uint8_t RequestTypeStandardEndpointOut = UsbRequestTypeHostToDevice | UsbRequestTypeStandard | UsbRequestTypeEndpoint;

    static const uint8_t RequestTypeClassDeviceIn = UsbRequestTypeDeviceToHost | UsbRequestTypeClass | UsbRequestTypeDevice;
    static const uint8_t RequestTypeClassDeviceOut = UsbRequestTypeHostToDevice | UsbRequestTypeClass | UsbRequestTypeDevice;
    static const uint8_t RequestTypeClassInterfaceIn = UsbRequestTypeDeviceToHost | UsbRequestTypeClass | UsbRequestTypeInterface;
    static const uint8_t RequestTypeClassInterfaceOut = UsbRequestTypeHostToDevice | UsbRequestTypeClass | UsbRequestTypeInterface;
    static const uint8_t RequestTypeClassEndpointIn = UsbRequestTypeDeviceToHost | UsbRequestTypeClass | UsbRequestTypeEndpoint;
    static const uint8_t RequestTypeClassEndpointOut = UsbRequestTypeHostToDevice | UsbRequestTypeClass | UsbRequestTypeEndpoint;

    static const uint8_t RequestTypeVendorDeviceIn = UsbRequestTypeDeviceToHost | UsbRequestTypeVendor | UsbRequestTypeDevice;
    static const uint8_t RequestTypeVendorDeviceOut = UsbRequestTypeHostToDevice | UsbRequestTypeVendor | UsbRequestTypeDevice;
    static const uint8_t RequestTypeVendorInterfaceIn = UsbRequestTypeDeviceToHost | UsbRequestTypeVendor | UsbRequestTypeInterface;
    static const uint8_t RequestTypeVendorInterfaceOut = UsbRequestTypeHostToDevice | UsbRequestTypeVendor | UsbRequestTypeInterface;
    static const uint8_t RequestTypeVendorEndpointIn = UsbRequestTypeDeviceToHost | UsbRequestTypeVendor | UsbRequestTypeEndpoint;
    static const uint8_t RequestTypeVendorEndpointOut = UsbRequestTypeHostToDevice | UsbRequestTypeVendor | UsbRequestTypeEndpoint;

    // usb_20.pdf, Table 9-4, Standard Request Codes
    static const uint8_t UsbRequestGetStatus = 0;
    static const uint8_t UsbRequestClearFeature = 1;
    static const uint8_t UsbRequestSetFeature = 3;
    static const uint8_t UsbRequestSetAddress = 5;
    static const uint8_t UsbRequestGetDescriptor = 6;
    static const uint8_t UsbRequestSetDescriptor = 7;
    static const uint8_t UsbRequestGetConfiguration = 8;
    static const uint8_t UsbRequestSetConfiguration = 9;
    static const uint8_t UsbRequestGetInterface = 10;
    static const uint8_t UsbRequestSetInterface = 11;
    static const uint8_t UsbRequestSyncFrame = 12;

    // usb_20.pdf, Table 9-5, Descriptor Types
    static const uint8_t UsbDescriptorTypeDevice = 1;
    static const uint8_t UsbDescriptorTypeConfiguration = 2;
    static const uint8_t UsbDescriptorTypeString = 3;
    static const uint8_t UsbDescriptorTypeInterface = 4;
    static const uint8_t UsbDescriptorTypeEndpoint = 5;
    static const uint8_t UsbDescriptorTypeDeviceQualifier = 6;
    static const uint8_t UsbDescriptorTypeOtherSpeedConfiguration = 7;
    static const uint8_t UsbDescriptorTypeInterfacePower = 8;
    static const uint8_t UsbDescriptorTypeOtg = 9;
    static const uint8_t UsbDescriptorTypeDebug = 10;
    static const uint8_t UsbDescriptorTypeInterfaceAssociation = 11;

    // usb_20.pdf, Figure 9-4, Information returned by a GetStatus() request to a device
    static const uint8_t UsbDeviceStatusSelfPowered = 0x01;
    static const uint8_t UsbDeviceStatusRemoteWakeup = 0x02;

    // usb_20.pdf, Figure 9-6, Information returned by a GetStatus() request to an endpoint
    static const uint8_t UsbEndpointStatusHalt = 0x01;

    // usb_20.pdf, Table 9-6, Standard Feature Selectors
    static const uint8_t UsbFeatureEndpointHalt = 0;
    static const uint8_t UsbFeatureDeviceRemoteWakeup = 1;
    static const uint8_t UsbFeatureTestMode = 2;

    // usb_20.pdf, Table 9-8, Standard Device Descriptor
    static const uint16_t UsbSpecificationVersion110 = 0x110;
    static const uint16_t UsbSpecificationVersion200 = 0x200;

    // usb_20.pdf, Table 9-10, Standard Configuration Descriptor
    static const uint8_t UsbConfigurationAttributeRemoteWakeup = 0x20;
    static const uint8_t UsbConfigurationAttributeBusPowered = 0x80;
    static const uint8_t UsbConfigurationAttributeSelfPowered = 0xC0;

    // usb_20.pdf, Table 9-13, Standard Endpoint Descriptor
    static const uint8_t UsbEndpointAddressOut = 0x00;
    static const uint8_t UsbEndpointAddressIn = 0x80;

    static const uint8_t UsbEndpointTypeControl = 0x00;
    static const uint8_t UsbEndpointTypeIsochronous = 0x01;
    static const uint8_t UsbEndpointTypeBulk = 0x02;
    static const uint8_t UsbEndpointTypeInterrupt = 0x03;

    // usb_20.pdf, Table 9-2, Format of Setup Data
    struct ATL_ATTRIBUTE_PACKED UsbRequest
    {
        uint8_t bmRequestType;
        uint8_t bRequest;
        uint16_t wValue;
        uint16_t wIndex;
        uint16_t wLength;

        uint8_t GetType() const { return bmRequestType & UsbRequestTypeType; }
        uint8_t GetDirection() const { return bmRequestType & UsbRequestTypeDirection; }
        uint8_t GetRecipient() const { return bmRequestType & UsbRequestTypeRecipient; }
    };

    // usb_20.pdf, Table 9-8, Standard Device Descriptor
    struct ATL_ATTRIBUTE_PACKED UsbDeviceDescriptor
    {
        uint8_t bLength;
        uint8_t bDescriptorType;
        uint16_t bcdUSB;
        uint8_t bDeviceClass;
        uint8_t bDeviceSubClass;
        uint8_t bDeviceProtocol;
        uint8_t bMaxPacketSize0;
        uint16_t idVendor;
        uint16_t idProduct;
        uint16_t bcdDevice;
        uint8_t iManufacturer;
        uint8_t iProduct;
        uint8_t iSerialNumber;
        uint8_t bNumConfigurations;
    };

    // usb_20.pdf, Table 9-10, Standard Configuration Descriptor
    struct ATL_ATTRIBUTE_PACKED UsbConfigurationDescriptor
    {
        uint8_t bLength;
        uint8_t bDescriptorType;
        uint16_t wTotalLength;
        uint8_t bNumInterfaces;
        uint8_t bConfigurationValue;
        uint8_t iConfiguration;
        uint8_t bmAttributes;
        uint8_t MaxPower;
    };

    // usb_20.pdf, Table 9-12, Standard Interface Descriptor
    struct ATL_ATTRIBUTE_PACKED UsbInterfaceDescriptor
    {
        uint8_t bLength;
        uint8_t bDescriptorType;
        uint8_t bInterfaceNumber;
        uint8_t bAlternateSetting;
        uint8_t bNumEndpoints;
        uint8_t bInterfaceClass;
        uint8_t bInterfaceSubClass;
        uint8_t bInterfaceProtocol;
        uint8_t iInterface;
    };

    // usb_20.pdf, Table 9-13, Standard Endpoint Descriptor
    struct ATL_ATTRIBUTE_PACKED UsbEndpointDescriptor
    {
        uint8_t bLength;
        uint8_t bDescriptorType;
        uint8_t bEndpointAddress;
        uint8_t bmAttributes;
        uint16_t wMaxPacketSize;
        uint8_t bInterval;
    };

    // InterfaceAssociationDescriptor_ecn.pdf, Table 9–Z, Standard Interface Association Descriptor
    struct ATL_ATTRIBUTE_PACKED UsbInterfaceAssociationDescriptor
    {
        uint8_t bLength;
        uint8_t bDescriptorType;
        uint8_t bFirstInterface;
        uint8_t bInterfaceCount;
        uint8_t bFunctionClass;
        uint8_t bFunctionSubClass;
        uint8_t bFunctionProtocol;
        uint8_t iFunction;
    };

    // Defined Class Codes
    // https://www.usb.org/defined-class-codes
    static const uint8_t UsbDeviceClassNone = 0x00;
    static const uint8_t UsbDeviceSubClassNone = 0x00;
    static const uint8_t UsbDeviceProtocolNone = 0x00;

    static const uint8_t UsbDeviceClassCdc = 0x02;

    static const uint8_t UsbDeviceClassMiscellaneous = 0xEF;
    static const uint8_t UsbDeviceSubClassMiscellaneousIad = 0x02;
    static const uint8_t UsbDeviceProtocolMiscellaneousIad = 0x01;

    static const uint8_t UsbDeviceClassVendor = 0xFF;
    static const uint8_t UsbDeviceSubClassVendorNone = 0x00;
    static const uint8_t UsbDeviceProtocolVendorNone = 0x00;

    static const uint8_t UsbInterfaceClassCdc = 0x02;
    static const uint8_t UsbInterfaceClassHid = 0x03;
    static const uint8_t UsbInterfaceClassMiscellaneous = 0xEF;
    static const uint8_t UsbInterfaceClassApplicationSpecific = 0xFE;

    static const uint8_t UsbInterfaceClassVendor = 0xFF;
    static const uint8_t UsbInterfaceSubClassVendorNone = 0x00;
    static const uint8_t UsbInterfaceProtocolVendorNone = 0x00;

    // OS_Desc_Intro.doc, Microsoft OS Descriptors
    static const uint16_t UsbMsOsDescriptorVersion = 0x0100;
    static const uint8_t UsbMsOsStringDescriptorId = 0xEE;
    static const uint8_t UsbMsOsDescriptorGenre = 0x01;
    static const uint8_t UsbMsOsDescriptorCompatId = 0x04;
    static const uint8_t UsbMsOsDescriptorProperties = 0x05;

    // OS_Desc_Intro.doc, Microsoft OS Descriptors
    struct ATL_ATTRIBUTE_PACKED UsbMsOsStringDescriptor
    {
        uint8_t bLength;
        uint8_t bDescriptorType;
        wchar_t qwSignature[7];
        uint8_t bMS_VendorCode;
        uint8_t bPad;
    };

    // OS_Desc_CompatID.doc, Extended Compat ID Descriptor Format
    struct ATL_ATTRIBUTE_PACKED UsbMsExtendedCompatHeader
    {
        uint32_t dwLength;
        uint16_t bcdVersion;
        uint16_t wIndex;
        uint8_t bCount;
        uint8_t reserved0[7];
    };

    // OS_Desc_CompatID.doc, Extended Compat ID Descriptor Format
    struct ATL_ATTRIBUTE_PACKED UsbMsExtendedCompatFunction
    {
        uint8_t bFirstInterfaceNumber;
        uint8_t reserved1;
        char compatibleID[8];
        char subCompatibleID[8];
        uint8_t reserved2[6];
    };

    // Convert a current in mA to USB current (2mA/bit)
    static constexpr uint8_t UsbMaxPower(uint16_t mA)
    {
        // usb_20.pdf, Table 9-10, Standard Configuration Descriptor
        return mA / 2;
    }
};
