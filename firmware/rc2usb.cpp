//
// rc2usb.cpp
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#define ATL_DEBUG 1
#define RC2USB_PPM 1
#define RC2USB_PCM 1
#define RC2USB_SRXL 1
#define RC2USB_PCINT 1
#define RC2USB_ACIC_A0 1
#define RC2USB_DEBUG 1

#include <stdint.h>
#include <string.h>
#include <avr/eeprom.h>
#include <avr/io.h>
#include <util/delay.h>

#include <atl/debug.h>
#include <atl/interrupts.h>
#include <atl/usb_cdc_device.h>
#include <atl/usb_hid_spec.h>
#include <atl/watchdog.h>
#include <boards/boards.h>

using namespace atl;

#include "TaskTimer.h"
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

#if RC2USB_DEBUG
static DigitalOutputPin<10> g_pinDebug10;
static DigitalOutputPin<11> g_pinDebug11;
static DigitalOutputPin<12> g_pinDebug12;
#endif

static BuiltinLed g_BuiltinLed;
static DigitalInputPin<4> g_SignalCapturePin;
static DigitalInputPin<14> g_SignalChangePin;
static TaskTimer g_TaskTimer;
static Configuration g_Configuration;
static Configuration g_EepromConfiguration __attribute__((section(".eeprom")));
static uint32_t g_updateRate;

#if RC2USB_PPM
class PpmReceiver : public PpmReceiverT<PpmReceiver, PpmReceiverTimer1B>
{
};

static PpmReceiver g_PpmReceiver;
#endif

#if RC2USB_PCM
class PcmReceiver : public PcmReceiverT<PcmReceiver, PcmReceiverTimer1>
{
};

static PcmReceiver g_PcmReceiver;
#endif

#if RC2USB_SRXL
class SrxlReceiver : public SrxlReceiverT<SrxlReceiver, SrxlReceiverTimer1C, SrxlReceiverUsart1>
{
};

static SrxlReceiver g_SrxlReceiver;
#endif

//---------------------------------------------------------------------------

class Receiver
{
public:
    void Initialize()
    {
#if RC2USB_PPM
        g_PpmReceiver.Initialize();
#endif
#if RC2USB_PCM
        g_PcmReceiver.Initialize();
#endif
#if RC2USB_SRXL
        g_SrxlReceiver.Initialize();
#endif
    }

    void Terminate()
    {
#if RC2USB_PPM
        g_PpmReceiver.Terminate();
#endif
#if RC2USB_PCM
        g_PcmReceiver.Terminate();
#endif
#if RC2USB_SRXL
        g_SrxlReceiver.Terminate();
#endif
    }

    void Update()
    {
        if (false)
        {
        }
#if RC2USB_PPM
        else if (g_PpmReceiver.IsReceiving())
        {
            m_signalSource = SignalSource::PPM;
        }
#endif
#if RC2USB_PCM
        else if (g_PcmReceiver.IsReceiving())
        {
            m_signalSource = SignalSource::PCM;
        }
#endif
#if RC2USB_SRXL
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
#if RC2USB_PPM
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
#if RC2USB_PPM
        case SignalSource::PPM:
            return g_PpmReceiver.GetChannelCount();
#endif
#if RC2USB_PCM
        case SignalSource::PCM:
            return g_PcmReceiver.GetChannelCount();
#endif
#if RC2USB_SRXL
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
#if RC2USB_PPM
        case SignalSource::PPM:
            return g_PpmReceiver.GetChannelPulseWidth(index);
#endif
#if RC2USB_PCM
        case SignalSource::PCM:
            return g_PcmReceiver.GetChannelPulseWidth(index);
#endif
#if RC2USB_SRXL
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
#if RC2USB_PPM
        case SignalSource::PPM:
            return PulseWidthToValue(channel, g_PpmReceiver.GetChannelPulseWidth(index));
#endif
#if RC2USB_PCM
        case SignalSource::PCM:
            return PulseWidthToValue(channel, g_PcmReceiver.GetChannelPulseWidth(index));
#endif
#if RC2USB_SRXL
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
#if RC2USB_PPM
        case SignalSource::PPM:
            return g_PpmReceiver.IsReceiving();
#endif
#if RC2USB_PCM
        case SignalSource::PCM:
            return g_PcmReceiver.IsReceiving();
#endif
#if RC2USB_SRXL
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
#if RC2USB_PPM
        case SignalSource::PPM:
            return g_PpmReceiver.HasNewData();
#endif
#if RC2USB_PCM
        case SignalSource::PCM:
            return g_PcmReceiver.HasNewData();
#endif
#if RC2USB_SRXL
        case SignalSource::SRXL:
            return g_SrxlReceiver.HasNewData();
#endif
        default:
            return false;
        }
    }

    void ClearNewData()
    {
#if RC2USB_PPM
        g_PpmReceiver.ClearNewData();
#endif
#if RC2USB_PCM
        g_PcmReceiver.ClearNewData();
#endif
#if RC2USB_SRXL
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

//---------------------------------------------------------------------------

#if defined(BOARD_ARDUINO_LEONARDO)
#define USB_VID 0x2341
#define USB_PID 0x8036
#elif defined(BOARD_ARDUINO_MICRO)
#define USB_VID 0x2341
#define USB_PID 0x8037
#elif defined(BOARD_SPARKFUN_PROMICRO)
#define USB_VID 0x1B4F
#define USB_PID 0x9206
#elif defined(BOARD_ADAFRUIT_CIRCUITPLAYGROUND)
#define USB_VID 0x239A
#define USB_PID 0x8011
#else
#error 'BOARD_XXX' not defined or unsupported board.
#endif

class UsbDevice : public UsbCdcDeviceT<UsbDevice>
{
    using base = UsbCdcDeviceT<UsbDevice>;

    static const uint16_t idVendor = USB_VID;
    static const uint16_t idProduct = USB_PID;
    static const uint16_t bcdDevice = 0x101;

    static const uint8_t hidInterface = 2;
    static const uint8_t hidEndpoint = 4;
    static const uint8_t hidEndpointSize = 8;

public:
    bool WriteReport()
    {
        if (IsConfigured())
        {
            UsbInEndpoint endpoint(hidEndpoint);
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
    friend UsbDeviceT;
    friend UsbCdcDeviceT;

    void OnEventStartOfFrame()
    {
        base::Flush();
    }

    void OnEventConfigurationChanged()
    {
        base::ConfigureEndpoints();
        ConfigureEndpoint(hidEndpoint, EndpointType::Interrupt, EndpointDirection::In, hidEndpointSize, EndpointBanks::Two);
        ResetAllEndpoints();
    }

    void OnEventControlLineStateChanged()
    {
        ATL_DEBUG_PRINT("OnEventControlLineStateChanged: BaudRate=%lu, ControlLineState=0x%02X!\n", GetBaudRate(), GetControlLineState());
        if (GetBaudRate() == 1200 && !IsDtrActive())
        {
            ResetToBootloader();
        }
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
            UsbInterfaceDescriptor interface;
            HidDescriptor hid;
            UsbEndpointDescriptor endpoint;
        }
        hidDescriptor PROGMEM =
        {
            {
                sizeof(UsbInterfaceDescriptor),
                UsbDescriptorTypeInterface,
                hidInterface, // bInterfaceNumber
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
                UsbEndpointAddressIn | hidEndpoint,
                UsbEndpointTypeInterrupt,
                hidEndpointSize,
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
                1 // bNumConfigurations
            };

            return WriteControlData(request.wLength, &descriptor, sizeof(descriptor), MemoryType::Progmem);
        }
        case UsbDescriptorTypeConfiguration:
        {
            static const UsbConfigurationDescriptor descriptor PROGMEM =
            {
                sizeof(UsbConfigurationDescriptor),
                UsbDescriptorTypeConfiguration,
                sizeof(descriptor) + sizeof(UsbCdcDeviceT::ConfigurationDescriptor) + sizeof(hidDescriptor),
                3, // bNumInterfaces
                1, // bConfigurationValue
                0, // iConfiguration
                UsbConfigurationAttributeBusPowered,
                UsbMaxPower(100)
            };

            auto cdcDescriptor = UsbCdcDeviceT::GetConfigurationDescriptor();

            UsbControlInEndpoint endpoint(request.wLength);
            endpoint.WriteData(&descriptor, sizeof(descriptor), MemoryType::Progmem);
            endpoint.WriteData(cdcDescriptor.GetData(), cdcDescriptor.GetSize(), cdcDescriptor.GetMemoryType());
            endpoint.WriteData(&hidDescriptor, sizeof(hidDescriptor), MemoryType::Progmem);
            return MapStatus(endpoint.CompleteTransfer());
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
                static const char product[] PROGMEM = "R/C to USB Joystick";
                return SendStringDescriptor_P(request.wLength, product, strlen_P(product));
            }
            case StringIdManufacturer:
            {
                static const char manufacturer[] PROGMEM = "Marius Greuel";
                return SendStringDescriptor_P(request.wLength, manufacturer, strlen_P(manufacturer));
            }
            case StringIdSerial:
            {
                static const char id[] PROGMEM = "greuel.org:rc2usb";
                return SendStringDescriptor_P(request.wLength, id, strlen_P(id));
            }
            default:
                return RequestStatus::NotHandled;
            }
        }
        case HidDescriptorTypeHid:
        {
            return WriteControlData(request.wLength, &hidDescriptor.hid, sizeof(hidDescriptor.hid), MemoryType::Progmem);
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
                ResetToBootloader();
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

    void ResetToBootloader()
    {
        Detach();
        Bootloader::ResetToBootloader();
    }
} g_UsbDevice;

//---------------------------------------------------------------------------

#if RC2USB_PPM || RC2USB_PCM
ISR(TIMER1_CAPT_vect)
{
    static bool risingEdge = false;
    TaskTimer::SetCaptureEdge(!risingEdge);
    uint16_t time = TaskTimer::ICR();

#if RC2USB_DEBUG
    g_pinDebug10.Toggle();
#endif

#if RC2USB_PPM
    g_PpmReceiver.OnInputCapture(time, risingEdge);
#endif
#if RC2USB_PCM
    g_PcmReceiver.OnInputCapture(time, risingEdge);
#endif

    risingEdge = !risingEdge;
}
#endif

#if RC2USB_PCINT
ISR(PCINT0_vect)
{
    uint16_t time = TaskTimer::TCNT();
    bool risingEdge = g_SignalChangePin;

#if RC2USB_DEBUG
    g_pinDebug11.Toggle();
#endif

#if RC2USB_PPM
    g_PpmReceiver.OnInputCapture(time, risingEdge);
#endif
#if RC2USB_PCM
    g_PcmReceiver.OnInputCapture(time, risingEdge);
#endif
}
#endif

ISR(TIMER1_COMPA_vect)
{
    g_TaskTimer.OnOutputCompare();

#if RC2USB_PPM
    g_PpmReceiver.RunTask();
#endif
#if RC2USB_PCM
    g_PcmReceiver.RunTask();
#endif
#if RC2USB_SRXL
    g_SrxlReceiver.RunTask();
#endif
}

#if RC2USB_PPM
ISR(TIMER1_COMPB_vect)
{
    g_PpmReceiver.OnOutputCompare();
}
#endif

#if RC2USB_SRXL
ISR(TIMER1_COMPC_vect)
{
    g_SrxlReceiver.OnOutputCompare();
}
#endif

#if RC2USB_SRXL
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

#if RC2USB_DEBUG
    StdStreams::SetupStdout([](char ch) { g_UsbDevice.WriteChar(ch); });
#endif

    g_SignalCapturePin.Configure(PinMode::InputPullup);

#if RC2USB_PCINT
    // Configure D14 (PB3/MISO) as PCINT3
    g_SignalChangePin.Configure(PinMode::InputPullup);
    PCMSK0 = _BV(PCINT3);
    PCIFR = _BV(PCIF0);
    PCICR = _BV(PCIE0);
#endif

#if RC2USB_ACIC_A0
    // Enable Analog Comparator Input Capture for A0/ADC7 (instead of ICP1)
    ACSR = _BV(ACBG) | _BV(ACIC);
    ADCSRA = 0;
    ADCSRB = _BV(ACME);
    ADMUX = _BV(MUX2) | _BV(MUX1) | _BV(MUX0);
#endif

#if RC2USB_DEBUG
    g_pinDebug10.Configure();
    g_pinDebug11.Configure();
    g_pinDebug12.Configure();
#endif

    g_BuiltinLed.Configure();

    g_TaskTimer.Initialize();
    g_Receiver.Initialize();
    g_UsbDevice.Attach();

    ReadConfigurationFromEeprom();
    
    Interrupts::Enable();

#if RC2USB_DEBUG
    printf_P(PSTR("Hello from rc2usb"));
#endif

    uint32_t lastLedUpdate = 0;
    uint32_t lastUsbUpdate = 0;
    for (;;)
    {
        Watchdog::Reset();

        uint32_t time = g_TaskTimer.GetMilliseconds();

        g_Receiver.Update();
        if (g_Receiver.IsReceiving())
        {
            if (g_Receiver.HasNewData())
            {
                if (g_UsbDevice.WriteReport())
                {
                    g_updateRate = time - lastUsbUpdate;
                    lastLedUpdate = time;
                    lastUsbUpdate = time;

                    g_BuiltinLed = true;
                    g_Receiver.ClearNewData();
                }
            }
        }
        else
        {
            if (time - lastLedUpdate >= 1000)
            {
                lastLedUpdate = time;
                g_BuiltinLed.Toggle();
            }

            g_UsbDevice.WriteReport();
        }

#if RC2USB_DEBUG
        g_pinDebug12.Toggle();
#endif
    }

    return 0;
}
