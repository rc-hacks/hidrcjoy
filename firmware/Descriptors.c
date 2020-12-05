//
// Descriptors.c
// Copyright(C) 2018 Marius Greuel.All rights reserved.
//

#include <avr/pgmspace.h>
#include "UsbReports.h"
#include "Descriptors.h"

/////////////////////////////////////////////////////////////////////////////

#if defined(USB_V_USB)

#include "usbdrv/usbdrv.h"
#include "usbconfig.h"

const uint8_t usbHidReportDescriptor[] PROGMEM =
{
    0x05, 0x01,         // USAGE_PAGE (Generic Desktop)
    0x09, 0x04,         // USAGE (Joystick)
    0xA1, 0x01,         // COLLECTION (Application)
    0x09, 0x01,         //   USAGE (Pointer)
    0x85, UsbReportId,  //   REPORT_ID (UsbReportId)
    0x75, 0x08,         //   REPORT_SIZE (8)
    0x15, 0x00,         //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00,   //   LOGICAL_MAXIMUM (255)
    0x35, 0x00,         //   PHYSICAL_MINIMUM (0)
    0x46, 0xFF, 0x00,   //   PHYSICAL_MAXIMUM (255)
    0xA1, 0x00,         //   COLLECTION (Physical)
    0x09, 0x30,         //     USAGE (X)
    0x09, 0x31,         //     USAGE (Y)
    0x95, 0x02,         //     REPORT_COUNT (2)
    0x81, 0x02,         //     INPUT (Data,Var,Abs)
    0xC0,               //   END_COLLECTION
    0xA1, 0x00,         //   COLLECTION (Physical)
    0x09, 0x32,         //     USAGE (Z)
    0x09, 0x33,         //     USAGE (Rx)
    0x95, 0x02,         //     REPORT_COUNT (2)
    0x81, 0x02,         //     INPUT (Data,Var,Abs)
    0xC0,               //   END_COLLECTION
    0xA1, 0x00,         //   COLLECTION (Physical)
    0x09, 0x34,         //     USAGE (Ry)
    0x09, 0x35,         //     USAGE (Rz)
    0x09, 0x36,         //     USAGE (Slider)
    0x95, 0x03,         //     REPORT_COUNT (3)
    0x81, 0x02,         //     INPUT (Data,Var,Abs)
    0xC0,               //   END_COLLECTION
    0xA1, 0x02,         //   COLLECTION (Logical)
    0x06, 0x00, 0xFF,   //     USAGE_PAGE (Vendor Defined Page 1)
    0x85, UsbEnhancedReportId, // REPORT_ID (UsbEnhancedReportId)
    0x95, sizeof(struct UsbEnhancedReport), // REPORT_COUNT (...)
    0x09, 0x00,         //     USAGE (...)
    0xB1, 0x02,         //     FEATURE (Data,Var,Abs)
    0x85, ConfigurationReportId, // REPORT_ID (...)
    0x95, sizeof(struct Configuration), // REPORT_COUNT (...)
    0x09, ConfigurationReportId, // USAGE (...)
    0xB1, 0x02,         //     FEATURE (Data,Var,Abs)
    0x85, LoadConfigurationDefaultsId, // REPORT_ID (...)
    0x95, 0x01,         //     REPORT_COUNT (1)
    0x09, LoadConfigurationDefaultsId, // USAGE (...)
    0xB1, 0x02,         //     FEATURE (Data,Var,Abs)
    0x85, ReadConfigurationFromEepromId, //REPORT_ID (...)
    0x95, 0x01,         //     REPORT_COUNT (1)
    0x09, ReadConfigurationFromEepromId, // USAGE (...)
    0xB1, 0x02,         //     FEATURE (Data,Var,Abs)
    0x85, WriteConfigurationToEepromId, // REPORT_ID (...)
    0x95, 0x01,         //     REPORT_COUNT (1)
    0x09, WriteConfigurationToEepromId, // USAGE (...)
    0xB1, 0x02,         //     FEATURE (Data,Var,Abs)
    0x85, JumpToBootloaderId, // REPORT_ID (...)
    0x95, 0x01,         //     REPORT_COUNT (1)
    0x09, JumpToBootloaderId, // USAGE (...)
    0xB1, 0x02,         //     FEATURE (Data,Var,Abs)
    0xC0,               //   END_COLLECTION
    0xC0,               // END COLLECTION
};

_Static_assert(sizeof(usbHidReportDescriptor) == USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH, "USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH size mismatch");

#elif defined(USB_LUFA)

#include <LUFA/Drivers/USB/USB.h>

typedef struct
{
    USB_Descriptor_Configuration_Header_t Config;
    USB_Descriptor_Interface_t HID_Interface;
    USB_HID_Descriptor_HID_t HID_JoystickHID;
    USB_Descriptor_Endpoint_t HID_ReportINEndpoint;
} USB_Descriptor_Configuration_t;

enum InterfaceDescriptors_t
{
    INTERFACE_ID_Joystick = 0,
};

enum StringDescriptors_t
{
    STRING_ID_Language,
    STRING_ID_Manufacturer,
    STRING_ID_Product,
    STRING_ID_Serial,
};

static const USB_Descriptor_HIDReport_Datatype_t JoystickReport[] PROGMEM =
{
    HID_RI_USAGE_PAGE(8, 0x01), // USAGE_PAGE (Generic Desktop)
    HID_RI_USAGE(8, 0x04),      // USAGE (Joystick)
    HID_RI_COLLECTION(8, 0x01), // COLLECTION (Application)
        HID_RI_USAGE(8, 0x01),      //   USAGE (Pointer)
        HID_RI_REPORT_ID(8, 1),
        HID_RI_REPORT_SIZE(8, 0x08),
        HID_RI_LOGICAL_MINIMUM(8, 0),
        HID_RI_LOGICAL_MAXIMUM(16, 255),
        HID_RI_PHYSICAL_MINIMUM(8, 0),
        HID_RI_PHYSICAL_MAXIMUM(16, 255),
        HID_RI_COLLECTION(8, 0x00), // COLLECTION (Physical)
            HID_RI_USAGE(8, 0x30),  // USAGE (X)
            HID_RI_USAGE(8, 0x31),  // USAGE (Y)
            HID_RI_REPORT_COUNT(8, 0x02),
            HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
        HID_RI_END_COLLECTION(0),
        HID_RI_COLLECTION(8, 0x00), // COLLECTION (Physical)
            HID_RI_USAGE(8, 0x32),  // USAGE (Z)
            HID_RI_USAGE(8, 0x33),  // USAGE (Rx)
            HID_RI_REPORT_COUNT(8, 0x02),
            HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
        HID_RI_END_COLLECTION(0),
        HID_RI_COLLECTION(8, 0x00), // COLLECTION (Physical)
            HID_RI_USAGE(8, 0x34),  // USAGE (Ry)
            HID_RI_USAGE(8, 0x35),  // USAGE (Rz)
            HID_RI_USAGE(8, 0x36),  // USAGE (Slider)
            HID_RI_REPORT_COUNT(8, 0x03),
            HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
        HID_RI_END_COLLECTION(0),
        HID_RI_COLLECTION(8, 0x02), // COLLECTION (Logical)
            HID_RI_USAGE_PAGE(16, 0x00FF), // USAGE_PAGE (Vendor Defined Page 1)
            HID_RI_REPORT_ID(8, UsbEnhancedReportId),
            HID_RI_REPORT_COUNT(8, sizeof(struct UsbEnhancedReport)),
            HID_RI_USAGE(8, UsbEnhancedReportId),
            HID_RI_FEATURE(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
            HID_RI_REPORT_ID(8, ConfigurationReportId),
            HID_RI_REPORT_COUNT(8, sizeof(struct Configuration)),
            HID_RI_USAGE(8, ConfigurationReportId),
            HID_RI_FEATURE(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
            HID_RI_REPORT_ID(8, LoadConfigurationDefaultsId),
            HID_RI_REPORT_COUNT(8, 1),
            HID_RI_USAGE(8, LoadConfigurationDefaultsId),
            HID_RI_FEATURE(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
            HID_RI_REPORT_ID(8, ReadConfigurationFromEepromId),
            HID_RI_REPORT_COUNT(8, 1),
            HID_RI_USAGE(8, ReadConfigurationFromEepromId),
            HID_RI_FEATURE(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
            HID_RI_REPORT_ID(8, WriteConfigurationToEepromId),
            HID_RI_REPORT_COUNT(8, 1),
            HID_RI_USAGE(8, WriteConfigurationToEepromId),
            HID_RI_FEATURE(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
            HID_RI_REPORT_ID(8, JumpToBootloaderId),
            HID_RI_REPORT_COUNT(8, 1),
            HID_RI_USAGE(8, JumpToBootloaderId),
            HID_RI_FEATURE(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
        HID_RI_END_COLLECTION(0),
    HID_RI_END_COLLECTION(0),
};

static const USB_Descriptor_Device_t DeviceDescriptor PROGMEM =
{
    { sizeof(USB_Descriptor_Device_t), DTYPE_Device },
    VERSION_BCD(2,0,0),
    USB_CSCP_NoDeviceClass,
    USB_CSCP_NoDeviceSubclass,
    USB_CSCP_NoDeviceProtocol,
    FIXED_CONTROL_ENDPOINT_SIZE,
    0x16C0,
    0x03E8,
    VERSION_BCD(0, 0, 1),
    STRING_ID_Manufacturer,
    STRING_ID_Product,
    STRING_ID_Serial,
    FIXED_NUM_CONFIGURATIONS,
};

static const USB_Descriptor_Configuration_t ConfigurationDescriptor PROGMEM =
{
    {
        { sizeof(USB_Descriptor_Configuration_Header_t), DTYPE_Configuration },
        sizeof(USB_Descriptor_Configuration_t),
        1,
        1,
        NO_DESCRIPTOR,
        (USB_CONFIG_ATTR_RESERVED),
        USB_CONFIG_POWER_MA(100),
    },
    {
        { sizeof(USB_Descriptor_Interface_t), DTYPE_Interface },
        INTERFACE_ID_Joystick,
        0x00,
        1,
        HID_CSCP_HIDClass,
        HID_CSCP_NonBootSubclass,
        HID_CSCP_NonBootProtocol,
        NO_DESCRIPTOR,
    },
    {
        { sizeof(USB_HID_Descriptor_HID_t), HID_DTYPE_HID },
        VERSION_BCD(1, 1, 1),
        0x00,
        1,
        HID_DTYPE_Report,
        sizeof(JoystickReport),
    },
    {
        { sizeof(USB_Descriptor_Endpoint_t), DTYPE_Endpoint },
        JOYSTICK_EPADDR,
        (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        JOYSTICK_EPSIZE,
        10,
    },
};

static const USB_Descriptor_String_t LanguageString PROGMEM = USB_STRING_DESCRIPTOR_ARRAY(LANGUAGE_ID_ENG);
static const USB_Descriptor_String_t ManufacturerString PROGMEM = USB_STRING_DESCRIPTOR(L"Marius Greuel");
static const USB_Descriptor_String_t ProductString PROGMEM = USB_STRING_DESCRIPTOR(L"R/C to PC Joystick");
static const USB_Descriptor_String_t SerialString PROGMEM = USB_STRING_DESCRIPTOR(L"greuel.org:hidrcjoy");

uint16_t CALLBACK_USB_GetDescriptor(uint16_t wValue, uint16_t wIndex, const void** descriptor)
{
    uint8_t descriptorType = (wValue >> 8);
    uint8_t descriptorNumber = (wValue & 0xFF);

    switch (descriptorType)
    {
    case DTYPE_Device:
        *descriptor = &DeviceDescriptor;
        return sizeof(USB_Descriptor_Device_t);
    case DTYPE_Configuration:
        *descriptor = &ConfigurationDescriptor;
        return sizeof(USB_Descriptor_Configuration_t);
    case DTYPE_String:
        switch (descriptorNumber)
        {
        case STRING_ID_Language:
            *descriptor = &LanguageString;
            return pgm_read_byte(&LanguageString.Header.Size);
        case STRING_ID_Manufacturer:
            *descriptor = &ManufacturerString;
            return pgm_read_byte(&ManufacturerString.Header.Size);
        case STRING_ID_Product:
            *descriptor = &ProductString;
            return pgm_read_byte(&ProductString.Header.Size);
        case STRING_ID_Serial:
            *descriptor = &SerialString;
            return pgm_read_byte(&SerialString.Header.Size);
        }
        break;
    case DTYPE_HID:
        *descriptor = &ConfigurationDescriptor.HID_JoystickHID;
        return sizeof(USB_HID_Descriptor_HID_t);
    case DTYPE_Report:
        *descriptor = &JoystickReport;
        return sizeof(JoystickReport);
    }

    return NO_DESCRIPTOR;
}

#endif
