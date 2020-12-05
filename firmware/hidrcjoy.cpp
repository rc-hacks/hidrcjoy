//
// hidrcjoy.cpp
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#include <stdint.h>
#include <string.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>

#define ATL_DEBUG 1
#define ATL_USB_DEBUG 1
#define ATL_USB_DEBUG_EXT 0
#include <boards/arduino_micro.h>
#include <atl/debug.h>
#include <atl/interrupts.h>
#include <atl/watchdog.h>
#include <atl/std_streams.h>
using namespace atl;

#if defined(USB_ATL)
#include <atl/usb_device.h>
#include <atl/usb_hid_spec.h>
#elif defined(USB_LUFA)
#include <LUFA/Drivers/USB/USB.h>
#include "Descriptors.h"
#elif defined(USB_V_USB)
extern "C" {
#include "usbdrv/usbdrv.h"
}
#endif

#if ATL_DEBUG
BuiltinLed g_BuiltinLed;
DigitalOutputPin<10> g_pinDebug10;
DigitalOutputPin<11> g_pinDebug11;
SerialT<SerialUnbuffered<SerialDriverUsart1>> g_serial;
#endif

/////////////////////////////////////////////////////////////////////////////

#define COUNTOF(array) (sizeof(array) / sizeof(array[0]))

#if defined (BOARD_ARDUINO_MICRO) || defined(BOARD_SPARKFUN_PROMICRO)
#define HIDRCJOY_SRXL 1
#define PPM_SIGNAL_PIN PIND
#define PPM_SIGNAL_PORT PORTD
#define PPM_SIGNAL 4 // Pin 4
#define LED_STATUS_DDR DDRB
#define LED_STATUS_PORT PORTB
#define LED_STATUS 0 // Pin 17 (built-in Rx LED)
#elif defined (BOARD_DIGISTUMP_DIGISPARK)
#define HIDRCJOY_SRXL 0
#define PPM_SIGNAL_PIN PINB
#define PPM_SIGNAL_PORT PORTB
#define PPM_SIGNAL 2 // Pin 2
#define LED_STATUS_DDR DDRB
#define LED_STATUS_PORT PORTB
#define LED_STATUS 1 // Pin 1 (built-in LED)
#elif defined (BOARD_DIGISTUMP_DIGISPARKPRO)
#define HIDRCJOY_SRXL 0
#define PPM_SIGNAL_PIN PINA
#define PPM_SIGNAL_PORT PORTA
#define PPM_SIGNAL 4
#define LED_STATUS_DDR DDRB
#define LED_STATUS_PORT PORTB
#define LED_STATUS 1 // Pin 1 (built-in LED)
#elif defined (BOARD_FABISP)
#define HIDRCJOY_SRXL 0
#define PPM_SIGNAL_PIN PINA
#define PPM_SIGNAL_PORT PORTA
#define PPM_SIGNAL 6 // ADC6/MOSI
#define LED_STATUS_DDR DDRA
#define LED_STATUS_PORT PORTA
#define LED_STATUS 5 // PA5/MISO
#else
#error Unsupported board
#endif

//---------------------------------------------------------------------------

static Timer g_Timer;
#include "Receiver.h"
#include "UsbReports.h"

//---------------------------------------------------------------------------

static Receiver g_Receiver;
static UsbReport g_UsbReport;
static UsbEnhancedReport g_UsbEnhancedReport;
static Configuration g_EepromConfiguration __attribute__((section(".eeprom")));

//---------------------------------------------------------------------------

static void PrepareUsbReport()
{
    auto status = g_Receiver.GetStatus();

    g_UsbReport.m_reportId = UsbReportId;

    for (uint8_t i = 0; i < COUNTOF(g_UsbReport.m_value); i++)
    {
        g_UsbReport.m_value[i] = status != Status::NoSignal ? g_Receiver.GetValue(i) : 0x80;
    }
}

static void PrepareUsbEnhancedReport()
{
    auto status = g_Receiver.GetStatus();

    g_UsbEnhancedReport.m_reportId = UsbEnhancedReportId;
    g_UsbEnhancedReport.m_status = status;

    for (uint8_t i = 0; i < COUNTOF(g_UsbEnhancedReport.m_channelPulseWidth); i++)
    {
        g_UsbEnhancedReport.m_channelPulseWidth[i] = status != Status::NoSignal ? g_Receiver.GetChannelData(i) : 0;
    }
}

static void LoadConfigurationDefaults()
{
    g_Receiver.LoadDefaultConfiguration();
    g_Receiver.UpdateConfiguration();
}

static void ReadConfigurationFromEeprom()
{
    eeprom_read_block(&g_Receiver.m_Configuration, &g_EepromConfiguration, sizeof(g_EepromConfiguration));

    if (!g_Receiver.IsValidConfiguration())
    {
        g_Receiver.LoadDefaultConfiguration();
    }

    g_Receiver.UpdateConfiguration();
}

static void WriteConfigurationToEeprom()
{
    eeprom_write_block(&g_Receiver.m_Configuration, &g_EepromConfiguration, sizeof(g_EepromConfiguration));
}

static void JumpToBootloader()
{
    asm volatile ("jmp __vectors - 4"); // jump to application reset vector at end of flash
}

//---------------------------------------------------------------------------
// USB

#if defined(USB_ATL)

class UsbDevice : public UsbDeviceT<UsbDevice>
{
    using base = UsbDeviceT<UsbDevice>;

    static const uint16_t idVendor = 0x16C0;
    static const uint16_t idProduct = 0x03E8;
    static const uint16_t bcdDevice = 0x101;

    static const uint8_t HidInterface = 0;
    static const uint8_t HidEndpoint = 1;
    static const uint8_t HidEndpointSize = 8;

public:
    void OnEventConfigurationChanged()
    {
        ConfigureEndpoint(HidEndpoint, EndpointType::Interrupt, EndpointDirection::In, HidEndpointSize, EndpointBanks::Two);
    }

    RequestStatus GetDescriptor(const UsbRequest& request)
    {
        enum StringIds : uint8_t
        {
            StringIdLanguage,
            StringIdManufacturer,
            StringIdProduct,
            StringIdSerial,
        };

        static const uint8_t hidReportDescriptor[] PROGMEM =
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

        static const struct ATL_ATTRIBUTE_PACKED Descriptor
        {
            UsbConfigurationDescriptor configuration;
            UsbInterfaceDescriptor interface;
            HidDescriptor hid;
            UsbEndpointDescriptor endpoint;
        }
        configurationDescriptor PROGMEM =
        {
            {
                sizeof(UsbConfigurationDescriptor),
                UsbDescriptorTypeConfiguration,
                sizeof(configurationDescriptor),
                1, // bNumInterfaces
                1, // bConfigurationValue
                0, // iConfiguration
                UsbConfigurationAttributeBusPowered,
                UsbMaxPower(100)
            },
            {
                sizeof(UsbInterfaceDescriptor),
                UsbDescriptorTypeInterface,
                HidInterface, // bInterfaceNumber
                0, // bAlternateSetting
                1, // bNumEndpoints
                HidInterfaceClass,
                HidInterfaceSubclassNone,
                HidInterfaceProtocolNone,
                0 // iInterface
            },
            {
                sizeof(HidDescriptor),
                HidDescriptorTypeHid,
                HidVersion,
                0, // bCountryCode
                1, // bNumDescriptors
                HidDescriptorTypeReport,
                sizeof(hidReportDescriptor),
            },
            {
                sizeof(UsbEndpointDescriptor),
                UsbDescriptorTypeEndpoint,
                UsbEndpointAddressIn | HidEndpoint,
                UsbEndpointTypeInterrupt,
                HidEndpointSize,
                10 // 10ms
            },
        };

        uint8_t type = static_cast<uint8_t>(request.wValue >> 8);
        switch (type)
        {
        case UsbDescriptorTypeDevice:
        {
            static const UsbDeviceDescriptor descriptor PROGMEM =
            {
                sizeof(UsbDeviceDescriptor),
                UsbDescriptorTypeDevice,
                UsbSpecificationVersion200,
                UsbDeviceClassNone,
                UsbDeviceSubClassNone,
                UsbDeviceProtocolNone,
                DefaultControlEndpointSize,
                idVendor,
                idProduct,
                bcdDevice,
                StringIdManufacturer,
                StringIdProduct,
                StringIdSerial,
                1, // bNumConfigurations
            };

            return WriteControlData(request.wLength, &descriptor, sizeof(descriptor), MemoryType::Progmem);
        }
        case UsbDescriptorTypeConfiguration:
        {
            return WriteControlData(request.wLength, &configurationDescriptor, sizeof(configurationDescriptor), MemoryType::Progmem);
        }
        case UsbDescriptorTypeString:
        {
            uint8_t index = static_cast<uint8_t>(request.wValue);
            switch (index)
            {
            case StringIdLanguage:
            {
                return SendLanguageIdDescriptor(request.wLength, LanguageId::English);
            }
            case StringIdProduct:
            {
                static const char product[] PROGMEM = "R/C to PC Joystick";
                return SendStringDescriptor_P(request.wLength, product, strlen_P(product));
            }
            case StringIdManufacturer:
            {
                static const char manufacturer[] PROGMEM = "Marius Greuel";
                return SendStringDescriptor_P(request.wLength, manufacturer, strlen_P(manufacturer));
            }
            case StringIdSerial:
            {
                static const char id[] PROGMEM = "greuel.org:hidrcjoy";
                return SendStringDescriptor_P(request.wLength, id, strlen_P(id));
            }
            default:
                return RequestStatus::NotHandled;
            }
        }
        case HidDescriptorTypeHid:
        {
            return WriteControlData(request.wLength, &configurationDescriptor.hid, sizeof(configurationDescriptor.hid), MemoryType::Progmem);
        }
        case HidDescriptorTypeReport:
        {
            return WriteControlData(request.wLength, hidReportDescriptor, sizeof(hidReportDescriptor), MemoryType::Progmem);
        }
        default:
            return RequestStatus::NotHandled;
        }
    }

    RequestStatus ProcessRequest(const UsbRequest& request)
    {
        if (request.bmRequestType == RequestTypeClassInterfaceIn && request.bRequest == HidRequestGetReport)
        {
            uint8_t reportId = static_cast<uint8_t>(request.wValue);
            switch (reportId)
            {
            case UsbReportId:
            {
                PrepareUsbReport();
                return WriteControlData(request.wLength, &g_UsbReport, sizeof(g_UsbReport), MemoryType::Ram);
            }
            case UsbEnhancedReportId:
            {
                PrepareUsbEnhancedReport();
                return WriteControlData(request.wLength, &g_UsbEnhancedReport, sizeof(g_UsbEnhancedReport), MemoryType::Ram);
            }
            case ConfigurationReportId:
            {
                g_Receiver.m_Configuration.m_reportId = ConfigurationReportId;
                return WriteControlData(request.wLength, &g_Receiver.m_Configuration, sizeof(g_Receiver.m_Configuration), MemoryType::Ram);
            }
            default:
                return RequestStatus::NotHandled;
            }
        }
        else if (request.bmRequestType == RequestTypeClassInterfaceOut && request.bRequest == HidRequestSetReport)
        {
            uint8_t reportId = static_cast<uint8_t>(request.wValue);
            switch (reportId)
            {
            case ConfigurationReportId:
            {
                auto status = ReadControlData(&g_Receiver.m_Configuration, sizeof(g_Receiver.m_Configuration));
                g_Receiver.UpdateConfiguration();
                return status;
            }
            case LoadConfigurationDefaultsId:
            {
                LoadConfigurationDefaults();
                return ReadControlData(&reportId, sizeof(reportId));
            }
            case ReadConfigurationFromEepromId:
            {
                ReadConfigurationFromEeprom();
                return ReadControlData(&reportId, sizeof(reportId));
            }
            case WriteConfigurationToEepromId:
            {
                WriteConfigurationToEeprom();
                return ReadControlData(&reportId, sizeof(reportId));
            }
            case JumpToBootloaderId:
            {
                ReadControlData(&reportId, sizeof(reportId));
                JumpToBootloader();
                return RequestStatus::Success;
            }
            default:
                return RequestStatus::NotHandled;
            }
        }
        else
        {
            return base::ProcessRequest(request);
        }
    }
};

UsbDevice usbDevice;

ISR(USB_GEN_vect)
{
    usbDevice.OnGeneralInterrupt();
}

ISR(USB_COM_vect)
{
    usbDevice.OnEndpointInterrupt();
}

static void InitializeUsb(void)
{
    usbDevice.Attach();
}

static void ProcessUsb(void)
{
}

static bool WriteUsbReport(void)
{
    if (usbDevice.IsConfigured())
    {
        UsbInEndpoint endpoint(1);
        if (endpoint.IsWriteAllowed())
        {
            PrepareUsbReport();
            endpoint.WriteData(&g_UsbReport, sizeof(g_UsbReport), MemoryType::Ram);
            endpoint.CompleteTransfer();
            return true;
        }
    }

    return false;
}

#elif defined(USB_LUFA)

void EVENT_USB_Device_ControlRequest(void)
{
    if (!Endpoint_IsSETUPReceived())
        return;

    switch (USB_ControlRequest.bRequest)
    {
    case HID_REQ_GetReport:
        if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
        {
            uint8_t reportId = (USB_ControlRequest.wValue & 0xFF);
            switch (reportId)
            {
            case UsbReportId:
                PrepareUsbReport();
                Endpoint_ClearSETUP();
                Endpoint_Write_Control_Stream_LE(&g_UsbReport, sizeof(g_UsbReport));
                Endpoint_ClearOUT();
                break;
            case UsbEnhancedReportId:
                PrepareUsbEnhancedReport();
                Endpoint_ClearSETUP();
                Endpoint_Write_Control_Stream_LE(&g_UsbEnhancedReport, sizeof(g_UsbEnhancedReport));
                Endpoint_ClearOUT();
                break;
            case ConfigurationReportId:
                g_Receiver.m_Configuration.m_reportId = ConfigurationReportId;
                Endpoint_ClearSETUP();
                Endpoint_Write_Control_Stream_LE(&g_Receiver.m_Configuration, sizeof(g_Receiver.m_Configuration));
                Endpoint_ClearOUT();
                break;
            }
        }
        break;
    case HID_REQ_SetReport:
        if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
        {
            uint8_t reportId = (USB_ControlRequest.wValue & 0xFF);
            switch (reportId)
            {
            case ConfigurationReportId:
                Endpoint_ClearSETUP();
                Endpoint_Read_Control_Stream_LE(&g_Receiver.m_Configuration, sizeof(g_Receiver.m_Configuration));
                Endpoint_ClearIN();
                g_Receiver.UpdateConfiguration();
                break;
            case LoadConfigurationDefaultsId:
                Endpoint_ClearSETUP();
                Endpoint_Read_Control_Stream_LE(&reportId, sizeof(reportId));
                Endpoint_ClearIN();
                LoadConfigurationDefaults();
                break;
            case ReadConfigurationFromEepromId:
                Endpoint_ClearSETUP();
                Endpoint_Read_Control_Stream_LE(&reportId, sizeof(reportId));
                Endpoint_ClearIN();
                ReadConfigurationFromEeprom();
                break;
            case WriteConfigurationToEepromId:
                Endpoint_ClearSETUP();
                Endpoint_Read_Control_Stream_LE(&reportId, sizeof(reportId));
                Endpoint_ClearIN();
                WriteConfigurationToEeprom();
                break;
            case JumpToBootloaderId:
                Endpoint_ClearSETUP();
                Endpoint_Read_Control_Stream_LE(&reportId, sizeof(reportId));
                Endpoint_ClearIN();
                JumpToBootloader();
                break;
            }
        }
        break;
    }
}

void EVENT_USB_Device_ConfigurationChanged(void)
{
    Endpoint_ConfigureEndpoint(JOYSTICK_EPADDR, EP_TYPE_INTERRUPT, JOYSTICK_EPSIZE, 1);
}

static void InitializeUsb(void)
{
    USB_Init();
}

static void ProcessUsb(void)
{
    USB_USBTask();
}


static bool WriteUsbReport(void)
{
    if (USB_DeviceState != DEVICE_STATE_Configured)
    {
        return false;
    }

    Endpoint_SelectEndpoint(JOYSTICK_EPADDR);

    if (!Endpoint_IsINReady())
    {
        return false;
    }

    PrepareUsbReport();
    Endpoint_Write_Stream_LE(&g_UsbReport, sizeof(g_UsbReport), NULL);
    Endpoint_ClearIN();

    return true;
}

#elif defined(USB_V_USB)

static uint8_t g_UsbWriteReportId;
static uint8_t g_UsbWritePosition;
static uint8_t g_UsbWriteBytesRemaining;

static void SetupUsbWrite(uint8_t reportId, uint8_t transferSize)
{
    g_UsbWriteReportId = reportId;
    g_UsbWritePosition = 0;
    g_UsbWriteBytesRemaining = transferSize;
}

extern "C" uchar usbFunctionWrite(uchar* data, uchar length)
{
    if (g_UsbWriteBytesRemaining == 0)
        return 1; // end of transfer

    if (length > g_UsbWriteBytesRemaining)
        length = g_UsbWriteBytesRemaining;

    memcpy(reinterpret_cast<uint8_t*>(&g_Receiver.m_Configuration) + g_UsbWritePosition, data, length);
    g_UsbWritePosition += length;
    g_UsbWriteBytesRemaining -= length;

    if (g_UsbWriteBytesRemaining == 0)
    {
        g_Receiver.UpdateConfiguration();
    }

    return g_UsbWriteBytesRemaining == 0; // return 1 if this was the last chunk
}

extern "C" usbMsgLen_t usbFunctionSetup(uchar data[8])
{
    static uint8_t idleRate;
    const usbRequest_t* request = (const usbRequest_t*)data;

    if ((request->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS)
    {
        if (request->bRequest == USBRQ_HID_GET_REPORT)
        {
            uint8_t reportId = request->wValue.bytes[0];
            switch (reportId)
            {
            case UsbReportId:
                PrepareUsbReport();
                usbMsgPtr = (usbMsgPtr_t)&g_UsbReport;
                return sizeof(g_UsbReport);
            case UsbEnhancedReportId:
                PrepareUsbEnhancedReport();
                usbMsgPtr = (usbMsgPtr_t)&g_UsbEnhancedReport;
                return sizeof(g_UsbEnhancedReport);
            case ConfigurationReportId:
                g_Receiver.m_Configuration.m_reportId = ConfigurationReportId;
                usbMsgPtr = (usbMsgPtr_t)&g_Receiver.m_Configuration;
                return sizeof(g_Receiver.m_Configuration);
            default:
                return 0;
            }
        }
        else if (request->bRequest == USBRQ_HID_SET_REPORT)
        {
            uint8_t reportId = request->wValue.bytes[0];
            switch (reportId)
            {
            case ConfigurationReportId:
                SetupUsbWrite(reportId, sizeof(g_Receiver.m_Configuration));
                return USB_NO_MSG;
            case LoadConfigurationDefaultsId:
                LoadConfigurationDefaults();
                return 0;
            case ReadConfigurationFromEepromId:
                ReadConfigurationFromEeprom();
                return 0;
            case WriteConfigurationToEepromId:
                WriteConfigurationToEeprom();
                return 0;
            case JumpToBootloaderId:
                JumpToBootloader();
                return 0;
            default:
                return 0;
            }
        }
        else if (request->bRequest == USBRQ_HID_GET_IDLE)
        {
            usbMsgPtr = (usbMsgPtr_t)&idleRate;
            return 1;
        }
        else if (request->bRequest == USBRQ_HID_SET_IDLE)
        {
            idleRate = request->wValue.bytes[1];
        }
    }

    return 0;
}

static void InitializeUsb(void)
{
    usbInit();
    usbDeviceDisconnect();

    for (int i = 0; i < 256; i++)
    {
        wdt_reset();
        _delay_ms(1);
    }

    usbDeviceConnect();
}

static void ProcessUsb(void)
{
    usbPoll();

    if (usbInterruptIsReady())
    {
        PrepareUsbReport();
        usbSetInterrupt((uchar*)&g_UsbReport, sizeof(g_UsbReport));
    }
}

#endif

//---------------------------------------------------------------------------

static void InitializePorts(void)
{
    // LED_STATUS is output
    LED_STATUS_DDR = _BV(LED_STATUS);

    // Pull-up on PPM_SIGNAL
    PPM_SIGNAL_PORT = _BV(PPM_SIGNAL);
}

//---------------------------------------------------------------------------

#ifndef TIMER0_OVF_vect
#define TIMER0_OVF_vect TIM0_OVF_vect
#endif

ISR(TIMER0_OVF_vect)
{
    g_Timer.OnOverflow();
}

//---------------------------------------------------------------------------

#if 0
static void BlinkStatusLed(bool good, uint32_t time)
{
    static uint32_t lastTime;

    if (good)
    {
        uint32_t period = 800000;
        if (time - lastTime > period)
        {
            lastTime = time;
            LED_STATUS_PORT ^= _BV(LED_STATUS);
        }
    }
    else
    {
        LED_STATUS_PORT &= ~_BV(LED_STATUS);
    }
}
#endif

//---------------------------------------------------------------------------

int main(void)
{
    Watchdog::Enable(Watchdog::Timeout::Time250ms);

    InitializePorts();
    g_Timer.Initialize();
    g_Receiver.Initialize();

#if ATL_DEBUG
    StdStreams::SetupStdout([](char ch) { g_serial.WriteChar(ch); });
    g_serial.Open(1000000);
    g_serial.Write_P(PSTR("Hello from hidrcjoy!\n"));
    g_pinDebug10.Configure();
    g_pinDebug11.Configure();
#endif

    g_BuiltinLed.Configure();

    InitializeUsb();
    ReadConfigurationFromEeprom();
    
    Interrupts::Enable();

    for (;;)
    {
        Watchdog::Reset();

        ProcessUsb();

        if (g_Receiver.IsReceiving())
        {
            if (g_Receiver.HasNewData())
            {
                if (WriteUsbReport())
                {
                    g_BuiltinLed.Toggle();
                    g_Receiver.AcknowledgeNewData();
                }
            }
        }
        else
        {
            g_BuiltinLed = true;
            WriteUsbReport();
        }

#if 0
        uint32_t time = g_Timer.GetTimeInUs();
        //g_Receiver.Update(time);
        //g_PpmReceiver.Update();
        //BlinkStatusLed(g_Receiver.GetStatus() != NoSignal, time);
#endif
        g_pinDebug11.Toggle();
    }

    return 0;
}
