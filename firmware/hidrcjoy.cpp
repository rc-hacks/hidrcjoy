//
// hidrcjoy.cpp
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#define HIDRCJOY_PPM 1
#define HIDRCJOY_PCM 1
#define HIDRCJOY_SRXL 1
#define HIDRCJOY_DEBUG 1

#include <stdint.h>
#include <string.h>
#include <avr/eeprom.h>
#include <avr/io.h>
#include <util/delay.h>

#include <atl/debug.h>
#include <atl/interrupts.h>
#include <atl/watchdog.h>
#include <atl/std_streams.h>
#include <atl/usb_device.h>
#include <atl/usb_hid_spec.h>
#include <boards/boards.h>

using namespace atl;

#if HIDRCJOY_DEBUG
DigitalOutputPin<10> g_pinDebug10;
DigitalOutputPin<11> g_pinDebug11;
DigitalOutputPin<12> g_pinDebug12;
SerialT<SerialUnbuffered<SerialDriverUsart1>> g_serial;
#endif

#include "TaskTimer1.h"
#include "PpmReceiver.h"
#include "PpmReceiverTimer1B.h"
#include "PcmReceiver.h"
#include "PcmReceiverTimer1.h"
#include "SrxlReceiver.h"
#include "SrxlReceiverTimer1C.h"
#include "SrxlReceiverUsart1.h"
#include "UsbReports.h"

/////////////////////////////////////////////////////////////////////////////

#define COUNTOF(x) (sizeof(x) / sizeof(x[0]))

static BuiltinLed g_BuiltinLed;
static DigitalInputPin<4> g_SignalPin;
static TaskTimer1 g_TaskTimer;
#if HIDRCJOY_PPM
static PpmReceiver<PpmReceiverTimer1B> g_PpmReceiver;
#endif
#if HIDRCJOY_PCM
static PcmReceiver<PcmReceiverTimer1> g_PcmReceiver;
#endif
#if HIDRCJOY_SRXL
static SrxlReceiver<SrxlReceiverTimer1C, SrxlReceiverUsart1> g_SrxlReceiver;
#endif
static Configuration g_Configuration;
static Configuration g_EepromConfiguration __attribute__((section(".eeprom")));
static uint32_t g_updateRate;

//---------------------------------------------------------------------------

class Receiver
{
public:
    void Initialize()
    {
#if HIDRCJOY_PPM
        g_PpmReceiver.Initialize();
#endif
#if HIDRCJOY_PCM
        g_PcmReceiver.Initialize();
#endif
#if HIDRCJOY_SRXL
        g_SrxlReceiver.Initialize();
#endif
    }

    void Terminate()
    {
#if HIDRCJOY_PPM
        g_PpmReceiver.Terminate();
#endif
#if HIDRCJOY_PCM
        g_PcmReceiver.Terminate();
#endif
#if HIDRCJOY_SRXL
        g_SrxlReceiver.Terminate();
#endif
    }

    void Update()
    {
        if (false)
        {
        }
#if HIDRCJOY_PPM
        else if (g_PpmReceiver.IsReceiving())
        {
            m_signalSource = SignalSource::PPM;
        }
#endif
#if HIDRCJOY_PCM
        else if (g_PcmReceiver.IsReceiving())
        {
            m_signalSource = SignalSource::PCM;
        }
#endif
#if HIDRCJOY_SRXL
        else if (g_SrxlReceiver.IsReceiving())
        {
            m_signalSource = SignalSource::SRXL;
        }
#endif
        else
        {
            m_signalSource = SignalSource::None;
        }
    }

    void LoadDefaultConfiguration()
    {
        g_Configuration.m_version = Configuration::version;
        g_Configuration.m_flags = 0;
        g_Configuration.m_minSyncPulseWidth = 3500;
        g_Configuration.m_centerChannelPulseWidth = 1500;
        g_Configuration.m_channelPulseWidthRange = 550;
        g_Configuration.m_polarity = 0;

        for (uint8_t i = 0; i < COUNTOF(g_Configuration.m_mapping); i++)
        {
            g_Configuration.m_mapping[i] = i;
        }
    }

    void UpdateConfiguration()
    {
#if HIDRCJOY_PPM
        auto minSyncPulseWidth = g_Configuration.m_minSyncPulseWidth;
        auto invertedSignal = (g_Configuration.m_flags & Configuration::Flags::InvertedSignal) != 0;
        ATL_DEBUG_PRINT("Configuration: MinSyncPulseWidth: %u\n", minSyncPulseWidth);
        ATL_DEBUG_PRINT("Configuration: InvertedSignal: %d\n", invertedSignal);
        g_PpmReceiver.SetConfiguration(minSyncPulseWidth, invertedSignal);
#endif
    }

    bool IsValidConfiguration() const
    {
        if (g_Configuration.m_version != Configuration::version)
            return false;

        if (g_Configuration.m_minSyncPulseWidth < Configuration::minSyncWidth ||
            g_Configuration.m_minSyncPulseWidth > Configuration::maxSyncWidth)
            return false;

        if (g_Configuration.m_centerChannelPulseWidth < Configuration::minChannelPulseWidth ||
            g_Configuration.m_centerChannelPulseWidth > Configuration::maxChannelPulseWidth)
            return false;

        if (g_Configuration.m_channelPulseWidthRange < 10 ||
            g_Configuration.m_channelPulseWidthRange > Configuration::maxChannelPulseWidth)
            return false;

        for (uint8_t i = 0; i < COUNTOF(g_Configuration.m_mapping); i++)
        {
            if (g_Configuration.m_mapping[i] >= Configuration::maxInputChannels)
            {
                return false;
            }
        }

        return true;
    }

    SignalSource GetSignalSource() const
    {
        return m_signalSource;
    }

    uint8_t GetChannelCount() const
    {
        switch (m_signalSource)
        {
#if HIDRCJOY_PPM
        case SignalSource::PPM:
            return g_PpmReceiver.GetChannelCount();
#endif
#if HIDRCJOY_PCM
        case SignalSource::PCM:
            return g_PcmReceiver.GetChannelCount();
#endif
#if HIDRCJOY_SRXL
        case SignalSource::SRXL:
            return g_SrxlReceiver.GetChannelCount();
#endif
        default:
            return 0;
        }
    }

    uint16_t GetChannelData(uint8_t channel) const
    {
        uint8_t index = g_Configuration.m_mapping[channel];

        switch (m_signalSource)
        {
#if HIDRCJOY_PPM
        case SignalSource::PPM:
            return g_PpmReceiver.GetChannelPulseWidth(index);
#endif
#if HIDRCJOY_PCM
        case SignalSource::PCM:
            return g_PcmReceiver.GetChannelPulseWidth(index);
#endif
#if HIDRCJOY_SRXL
        case SignalSource::SRXL:
            return g_SrxlReceiver.GetChannelPulseWidth(index);
#endif
        default:
            return 0;
        }
    }

    uint8_t GetChannelValue(uint8_t channel) const
    {
        uint8_t index = g_Configuration.m_mapping[channel];

        switch (m_signalSource)
        {
#if HIDRCJOY_PPM
        case SignalSource::PPM:
            return PulseWidthToValue(channel, g_PpmReceiver.GetChannelPulseWidth(index));
#endif
#if HIDRCJOY_PCM
        case SignalSource::PCM:
            return PulseWidthToValue(channel, g_PcmReceiver.GetChannelPulseWidth(index));
#endif
#if HIDRCJOY_SRXL
        case SignalSource::SRXL:
            return PulseWidthToValue(channel, g_SrxlReceiver.GetChannelPulseWidth(index));
#endif
        default:
            return 0x80;
        }
    }

    bool IsReceiving() const
    {
        switch (m_signalSource)
        {
#if HIDRCJOY_PPM
        case SignalSource::PPM:
            return g_PpmReceiver.IsReceiving();
#endif
#if HIDRCJOY_PCM
        case SignalSource::PCM:
            return g_PcmReceiver.IsReceiving();
#endif
#if HIDRCJOY_SRXL
        case SignalSource::SRXL:
            return g_SrxlReceiver.IsReceiving();
#endif
        default:
            return false;
        }
    }

    bool HasNewData() const
    {
        switch (m_signalSource)
        {
#if HIDRCJOY_PPM
        case SignalSource::PPM:
            return g_PpmReceiver.HasNewData();
#endif
#if HIDRCJOY_PCM
        case SignalSource::PCM:
            return g_PcmReceiver.HasNewData();
#endif
#if HIDRCJOY_SRXL
        case SignalSource::SRXL:
            return g_SrxlReceiver.HasNewData();
#endif
        default:
            return false;
        }
    }

    void ClearNewData()
    {
#if HIDRCJOY_PPM
        g_PpmReceiver.ClearNewData();
#endif
#if HIDRCJOY_PCM
        g_PcmReceiver.ClearNewData();
#endif
#if HIDRCJOY_SRXL
        g_SrxlReceiver.ClearNewData();
#endif
    }

private:
    uint8_t PulseWidthToValue(uint8_t channel, uint16_t value) const
    {
        if (value == 0)
            return 0x80;

        int16_t center = g_Configuration.m_centerChannelPulseWidth;
        int16_t range = g_Configuration.m_channelPulseWidthRange;
        bool inverted = (g_Configuration.m_polarity & (1 << channel)) != 0;
        return SaturateValue(ScaleValue(InvertValue(static_cast<int16_t>(value) - center, inverted), range));
    }

    static int16_t InvertValue(int16_t value, bool inverted)
    {
        return inverted ? -value : value;
    }

    static int16_t ScaleValue(int16_t value, int16_t range)
    {
        return static_cast<int16_t>(128 + (128 * static_cast<int32_t>(value) / range));
    }

    static uint8_t SaturateValue(int16_t value)
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

private:
    SignalSource m_signalSource = SignalSource::None;
} g_Receiver;

//---------------------------------------------------------------------------

static void LoadConfigurationDefaults()
{
    g_Receiver.LoadDefaultConfiguration();
    g_Receiver.UpdateConfiguration();
}

static void ReadConfigurationFromEeprom()
{
    eeprom_read_block(&g_Configuration, &g_EepromConfiguration, sizeof(g_EepromConfiguration));

    if (!g_Receiver.IsValidConfiguration())
    {
        g_Receiver.LoadDefaultConfiguration();
    }

    g_Receiver.UpdateConfiguration();
}

static void WriteConfigurationToEeprom()
{
    eeprom_write_block(&g_Configuration, &g_EepromConfiguration, sizeof(g_EepromConfiguration));
}

static void JumpToBootloader()
{
    asm volatile ("jmp __vectors - 4"); // jump to application reset vector at end of flash
}

//---------------------------------------------------------------------------

class HidRcJoyDevice : public UsbDeviceT<HidRcJoyDevice>
{
    using UsbDevice = UsbDeviceT<HidRcJoyDevice>;

    static const uint16_t idVendor = 0x16C0;
    static const uint16_t idProduct = 0x03E8;
    static const uint16_t bcdDevice = 0x101;

    static const uint8_t HidInterface = 0;
    static const uint8_t HidEndpoint = 1;
    static const uint8_t HidEndpointSize = 8;

public:
    bool WriteReport()
    {
        if (IsConfigured())
        {
            UsbInEndpoint endpoint(1);
            if (endpoint.IsWriteAllowed())
            {
                UsbReport report;
                CreateReport(report);
                endpoint.WriteData(&report, sizeof(report), MemoryType::Ram);
                endpoint.CompleteTransfer();
                return true;
            }
        }

        return false;
    }

    void CreateReport(UsbReport& report)
    {
        report.m_reportId = UsbReportId;

        for (uint8_t i = 0; i < COUNTOF(report.m_value); i++)
        {
            report.m_value[i] = g_Receiver.GetChannelValue(i);
        }
    }

    void CreateEnhancedReport(UsbEnhancedReport& report)
    {
        auto signalSource = g_Receiver.GetSignalSource();
        auto channelCount = g_Receiver.GetChannelCount();

        report.m_reportId = UsbEnhancedReportId;
        report.m_signalSource = signalSource;
        report.m_channelCount = channelCount;
        report.m_updateRate = g_updateRate;

        for (uint8_t i = 0; i < COUNTOF(report.m_channelPulseWidth); i++)
        {
            report.m_channelPulseWidth[i] = g_Receiver.GetChannelData(i);
        }
    }

private:
    friend UsbDevice;

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
                UsbReport report;
                CreateReport(report);
                return WriteControlData(request.wLength, &report, sizeof(report), MemoryType::Ram);
            }
            case UsbEnhancedReportId:
            {
                UsbEnhancedReport report;
                CreateEnhancedReport(report);
                return WriteControlData(request.wLength, &report, sizeof(report), MemoryType::Ram);
            }
            case ConfigurationReportId:
            {
                g_Configuration.m_reportId = ConfigurationReportId;
                return WriteControlData(request.wLength, &g_Configuration, sizeof(g_Configuration), MemoryType::Ram);
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
                auto status = ReadControlData(&g_Configuration, sizeof(g_Configuration));
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
            return UsbDevice::ProcessRequest(request);
        }
    }
} g_UsbDevice;

//---------------------------------------------------------------------------

#if HIDRCJOY_PPM || HIDRCJOY_PCM
ISR(TIMER1_CAPT_vect)
{
    static bool risingEdge = false;
    TaskTimer1::SetCaptureEdge(!risingEdge);
    uint16_t time = TaskTimer1::ICR();

#if HIDRCJOY_PPM
    g_PpmReceiver.OnInputCapture(time, risingEdge);
#endif
#if HIDRCJOY_PCM
    g_PcmReceiver.OnInputCapture(time, risingEdge);
#endif

    risingEdge = !risingEdge;
}

ISR(TIMER1_COMPA_vect)
{
    g_TaskTimer.OnOutputCompare();

#if HIDRCJOY_PPM
    g_PpmReceiver.RunTask();
#endif
#if HIDRCJOY_PCM
    g_PcmReceiver.RunTask();
#endif
#if HIDRCJOY_SRXL
    g_SrxlReceiver.RunTask();
#endif
}

ISR(TIMER1_COMPB_vect)
{
    g_PpmReceiver.OnOutputCompare();
}
#endif

#if HIDRCJOY_SRXL
ISR(TIMER1_COMPC_vect)
{
    g_SrxlReceiver.OnOutputCompare();
}
#endif

#if HIDRCJOY_SRXL
ISR(USART1_RX_vect)
{
    g_SrxlReceiver.OnDataReceived(UDR1);
}
#endif

ISR(USB_GEN_vect)
{
    g_UsbDevice.OnGeneralInterrupt();
}

ISR(USB_COM_vect)
{
    g_UsbDevice.OnEndpointInterrupt();
}

//---------------------------------------------------------------------------

int main(void)
{
    Watchdog::Enable(Watchdog::Timeout::Time250ms);

#if HIDRCJOY_DEBUG
    StdStreams::SetupStdout([](char ch) { g_serial.WriteChar(ch); });
    g_serial.Open(1000000);
    g_serial.Write_P(PSTR("Hello from hidrcjoy!\n"));
    g_pinDebug10.Configure();
    g_pinDebug11.Configure();
    g_pinDebug12.Configure();
#endif

    g_TaskTimer.Initialize();
    g_SignalPin.Configure(PinMode::InputPullup);
    g_BuiltinLed.Configure();
    g_Receiver.Initialize();
    g_UsbDevice.Attach();

    ReadConfigurationFromEeprom();
    
    Interrupts::Enable();

    uint32_t lastUpdate = 0;

    for (;;)
    {
        Watchdog::Reset();

        g_Receiver.Update();
        if (g_Receiver.IsReceiving())
        {
            if (g_Receiver.HasNewData())
            {
                if (g_UsbDevice.WriteReport())
                {
                    uint32_t time = g_TaskTimer.GetMilliseconds();
                    g_updateRate = time - lastUpdate;
                    lastUpdate = time;

                    g_BuiltinLed.Toggle();
                    g_Receiver.ClearNewData();
                }
            }
        }
        else
        {
            g_BuiltinLed = true;
            g_UsbDevice.WriteReport();
        }
    }

    return 0;
}
