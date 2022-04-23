//
// hidrcjoy.cpp
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#define ATL_DEBUG 1
#define HIDRCJOY_PPM 1
#define HIDRCJOY_PCM 1
#define HIDRCJOY_SRXL 1
#define HIDRCJOY_ICP 1
#define HIDRCJOY_PCINT 0
#define HIDRCJOY_ICP_ACIC_A0 1
#define HIDRCJOY_DEBUG 1

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

using namespace atl;

#include "shared/system_timer.h"
#include "shared/ppm_receiver.h"
#include "shared/ppm_receiver_timer1b.h"
#include "shared/pcm_receiver.h"
#include "shared/pcm_receiver_timer1.h"
#include "shared/srxl_receiver.h"
#include "shared/srxl_receiver_timer1c.h"
#include "shared/srxl_receiver_usart1.h"
#include "hidrcjoy_board.h"
//#include "hidrcjoy_usb.h"
#include "usb_reports.h"

/////////////////////////////////////////////////////////////////////////////

#define COUNTOF(x) (sizeof(x) / sizeof(x[0]))

static Board g_board;
static SystemTimer g_timer;
static Configuration g_configuration;
static Configuration g_eepromConfiguration __attribute__((section(".eeprom")));
static uint16_t g_updateRate;
static bool g_invertedSignal;

#if HIDRCJOY_PPM
class PpmReceiver : public PpmReceiverT<PpmReceiver, PpmReceiverTimer1B>
{
};

static PpmReceiver g_ppmReceiver;
#endif

#if HIDRCJOY_PCM
class PcmReceiver : public PcmReceiverT<PcmReceiver, PcmReceiverTimer1>
{
};

static PcmReceiver g_pcmReceiver;
#endif

#if HIDRCJOY_SRXL
class SrxlReceiver : public SrxlReceiverT<SrxlReceiver, SrxlReceiverTimer1C, SrxlReceiverUsart1>
{
};

static SrxlReceiver g_srxlReceiver;
#endif

//---------------------------------------------------------------------------

class Receiver
{
public:
    void Initialize()
    {
#if HIDRCJOY_PPM
        g_ppmReceiver.Initialize();
#endif
#if HIDRCJOY_PCM
        g_pcmReceiver.Initialize();
#endif
#if HIDRCJOY_SRXL
        g_srxlReceiver.Initialize();
#endif
    }

    void Update()
    {
        if (false)
        {
        }
#if HIDRCJOY_PPM
        else if (g_ppmReceiver.IsReceiving())
        {
            m_signalSource = SignalSource::PPM;
        }
#endif
#if HIDRCJOY_PCM
        else if (g_pcmReceiver.IsReceiving())
        {
            m_signalSource = SignalSource::PCM;
        }
#endif
#if HIDRCJOY_SRXL
        else if (g_srxlReceiver.IsReceiving())
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
        g_configuration.m_version = Configuration::version;
        g_configuration.m_flags = 0;
        g_configuration.m_minSyncPulseWidth = 3500;
        g_configuration.m_centerChannelPulseWidth = 1500;
        g_configuration.m_channelPulseWidthRange = 550;
        g_configuration.m_polarity = 0;

        for (uint8_t i = 0; i < COUNTOF(g_configuration.m_mapping); i++)
        {
            g_configuration.m_mapping[i] = i;
        }
    }

    void UpdateConfiguration()
    {
#if HIDRCJOY_PPM
        auto minSyncPulseWidth = g_configuration.m_minSyncPulseWidth;
        auto invertedSignal = (g_configuration.m_flags & Configuration::Flags::InvertedSignal) != 0;
        ATL_DEBUG_PRINT("Configuration: MinSyncPulseWidth: %u\n", minSyncPulseWidth);
        ATL_DEBUG_PRINT("Configuration: InvertedSignal: %d\n", invertedSignal);
        g_ppmReceiver.SetMinSyncPulseWidth(minSyncPulseWidth);
        g_invertedSignal = invertedSignal;
#endif
    }

    bool IsValidConfiguration() const
    {
        if (g_configuration.m_version != Configuration::version)
            return false;

        if (g_configuration.m_minSyncPulseWidth < Configuration::minSyncWidth ||
            g_configuration.m_minSyncPulseWidth > Configuration::maxSyncWidth)
            return false;

        if (g_configuration.m_centerChannelPulseWidth < Configuration::minChannelPulseWidth ||
            g_configuration.m_centerChannelPulseWidth > Configuration::maxChannelPulseWidth)
            return false;

        if (g_configuration.m_channelPulseWidthRange < 10 ||
            g_configuration.m_channelPulseWidthRange > Configuration::maxChannelPulseWidth)
            return false;

        for (uint8_t i = 0; i < COUNTOF(g_configuration.m_mapping); i++)
        {
            if (g_configuration.m_mapping[i] >= Configuration::maxInputChannels)
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
            return g_ppmReceiver.GetChannelCount();
#endif
#if HIDRCJOY_PCM
        case SignalSource::PCM:
            return g_pcmReceiver.GetChannelCount();
#endif
#if HIDRCJOY_SRXL
        case SignalSource::SRXL:
            return g_srxlReceiver.GetChannelCount();
#endif
        default:
            return 0;
        }
    }

    uint16_t GetChannelData(uint8_t channel) const
    {
        uint8_t index = g_configuration.m_mapping[channel];

        switch (m_signalSource)
        {
#if HIDRCJOY_PPM
        case SignalSource::PPM:
            return g_ppmReceiver.GetChannelPulseWidth(index);
#endif
#if HIDRCJOY_PCM
        case SignalSource::PCM:
            return g_pcmReceiver.GetChannelPulseWidth(index);
#endif
#if HIDRCJOY_SRXL
        case SignalSource::SRXL:
            return g_srxlReceiver.GetChannelPulseWidth(index);
#endif
        default:
            return 0;
        }
    }

    uint8_t GetChannelValue(uint8_t channel) const
    {
        uint8_t index = g_configuration.m_mapping[channel];

        switch (m_signalSource)
        {
#if HIDRCJOY_PPM
        case SignalSource::PPM:
            return PulseWidthToValue(channel, g_ppmReceiver.GetChannelPulseWidth(index));
#endif
#if HIDRCJOY_PCM
        case SignalSource::PCM:
            return PulseWidthToValue(channel, g_pcmReceiver.GetChannelPulseWidth(index));
#endif
#if HIDRCJOY_SRXL
        case SignalSource::SRXL:
            return PulseWidthToValue(channel, g_srxlReceiver.GetChannelPulseWidth(index));
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
            return g_ppmReceiver.IsReceiving();
#endif
#if HIDRCJOY_PCM
        case SignalSource::PCM:
            return g_pcmReceiver.IsReceiving();
#endif
#if HIDRCJOY_SRXL
        case SignalSource::SRXL:
            return g_srxlReceiver.IsReceiving();
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
            return g_ppmReceiver.HasNewData();
#endif
#if HIDRCJOY_PCM
        case SignalSource::PCM:
            return g_pcmReceiver.HasNewData();
#endif
#if HIDRCJOY_SRXL
        case SignalSource::SRXL:
            return g_srxlReceiver.HasNewData();
#endif
        default:
            return false;
        }
    }

    void ClearNewData()
    {
#if HIDRCJOY_PPM
        g_ppmReceiver.ClearNewData();
#endif
#if HIDRCJOY_PCM
        g_pcmReceiver.ClearNewData();
#endif
#if HIDRCJOY_SRXL
        g_srxlReceiver.ClearNewData();
#endif
    }

private:
    uint8_t PulseWidthToValue(uint8_t channel, uint16_t value) const
    {
        if (value == 0)
            return 0x80;

        int16_t center = g_configuration.m_centerChannelPulseWidth;
        int16_t range = g_configuration.m_channelPulseWidthRange;
        bool inverted = (g_configuration.m_polarity & (1 << channel)) != 0;
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
} g_receiver;

//---------------------------------------------------------------------------

static void LoadConfigurationDefaults()
{
    g_receiver.LoadDefaultConfiguration();
    g_receiver.UpdateConfiguration();
}

static void ReadConfigurationFromEeprom()
{
    eeprom_read_block(&g_configuration, &g_eepromConfiguration, sizeof(g_eepromConfiguration));

    if (!g_receiver.IsValidConfiguration())
    {
        g_receiver.LoadDefaultConfiguration();
    }

    g_receiver.UpdateConfiguration();
}

static void WriteConfigurationToEeprom()
{
    eeprom_write_block(&g_configuration, &g_eepromConfiguration, sizeof(g_eepromConfiguration));
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
            report.m_value[i] = g_receiver.GetChannelValue(i);
        }
    }

    void CreateEnhancedReport(UsbEnhancedReport& report)
    {
        auto signalSource = g_receiver.GetSignalSource();
        auto channelCount = g_receiver.GetChannelCount();

        report.m_reportId = UsbEnhancedReportId;
        report.m_signalSource = signalSource;
        report.m_channelCount = channelCount;
        report.m_updateRate = g_updateRate;

        for (uint8_t i = 0; i < COUNTOF(report.m_channelPulseWidth); i++)
        {
            report.m_channelPulseWidth[i] = g_receiver.GetChannelData(i);
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
                static const char id[] PROGMEM = "greuel.org:hidrcjoy";
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
                g_configuration.m_reportId = ConfigurationReportId;
                return WriteControlData(request.wLength, &g_configuration, sizeof(g_configuration), MemoryType::Ram);
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
                auto status = ReadControlData(&g_configuration, sizeof(g_configuration));
                g_receiver.UpdateConfiguration();
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
} g_usbDevice;

//---------------------------------------------------------------------------

#if HIDRCJOY_ICP & (HIDRCJOY_PPM || HIDRCJOY_PCM)
ISR(TIMER1_CAPT_vect)
{
    uint16_t time = ICR1;
    bool risingEdge = (TCCR1B & _BV(ICES1)) != 0;

#if HIDRCJOY_ICP_ACIC_A0
    risingEdge = !risingEdge;
#endif

    if (g_invertedSignal)
    {
        risingEdge = !risingEdge;
    }

#if HIDRCJOY_DEBUG
    g_board.m_debug.SetD9(risingEdge);
#endif

#if HIDRCJOY_PPM
    if (risingEdge)
    {
        g_ppmReceiver.OnInputEdge(time);
    }
#endif
#if HIDRCJOY_PCM
    g_pcmReceiver.OnInputEdge(time, risingEdge);
#endif

    TCCR1B ^= _BV(ICES1);
}
#endif

#if HIDRCJOY_PCINT
ISR(PCINT0_vect)
{
    uint16_t time = TCNT1;
    bool risingEdge = (PPM_PCINT_PIN & _BV(PPM_PCINT_BIT)) != 0;

    if (g_invertedSignal)
    {
        risingEdge = !risingEdge;
    }

#if HIDRCJOY_DEBUG
    g_board.m_debug.SetD9(risingEdge);
    g_board.m_debug.ToggleD10();
#endif

#if HIDRCJOY_PPM
    if (risingEdge)
    {
        g_ppmReceiver.OnInputEdge(time);
    }
#endif
#if HIDRCJOY_PCM
    g_pcmReceiver.OnInputEdge(time, risingEdge);
#endif
}
#endif

ISR(TIMER1_COMPA_vect)
{
    g_timer.OnOutputCompare();

#if HIDRCJOY_PCM
    g_pcmReceiver.RunTask();
#endif
#if HIDRCJOY_SRXL
    g_srxlReceiver.RunTask();
#endif
}

#if HIDRCJOY_PPM
ISR(TIMER1_COMPB_vect)
{
    g_ppmReceiver.OnOutputCompare();
}
#endif

#if HIDRCJOY_SRXL
ISR(TIMER1_COMPC_vect)
{
    g_srxlReceiver.OnOutputCompare();
}
#endif

#if HIDRCJOY_SRXL
ISR(USART1_RX_vect)
{
    g_srxlReceiver.OnDataReceived(UDR1);
}
#endif

ISR(USB_GEN_vect)
{
    g_usbDevice.OnGeneralInterrupt();
}

ISR(USB_COM_vect)
{
    g_usbDevice.OnEndpointInterrupt();
}

//---------------------------------------------------------------------------

int main(void)
{
    g_board.Initialize();
    g_timer.Initialize();
    g_receiver.Initialize();
    g_usbDevice.Attach();

#if HIDRCJOY_ICP
    // Configure D4/PD4 as ICP1
    PPM_ICP_DDR &= ~_BV(PPM_ICP_BIT);
    PPM_ICP_PORT |= _BV(PPM_ICP_BIT);
#endif

#if HIDRCJOY_PCINT
    // Configure D14/PB3 as PCINT3
    PPM_PCINT_DDR &= ~_BV(PPM_PCINT_BIT);
    PPM_PCINT_PORT |= _BV(PPM_PCINT_BIT);
    PCMSK0 = _BV(PCINT3);
    PCIFR = _BV(PCIF0);
    PCICR = _BV(PCIE0);
#endif

#if HIDRCJOY_ICP_ACIC_A0
    // Enable Analog Comparator Input Capture for D18/A0/PF7 instead of ICP1
    ACSR = _BV(ACBG) | _BV(ACIC);
    ADCSRA = 0;
    ADCSRB = _BV(ACME);
    ADMUX = _BV(MUX2) | _BV(MUX1) | _BV(MUX0);
#endif

    ReadConfigurationFromEeprom();

    Watchdog::Enable(Watchdog::Timeout::Time250ms);
    Interrupts::Enable();

    StdStreams::SetupStdout([](char ch) { g_usbDevice.WriteChar(ch); });

    printf_P(PSTR("Hello from hidrcjoy!\n"));

    SignalSource lastSource = SignalSource::None;
    uint16_t lastLedUpdate = 0;
    uint16_t lastUsbUpdate = 0;
    for (;;)
    {
        Watchdog::Reset();

        auto time = g_timer.GetMilliseconds();
        g_board.RunTask(time);

        g_receiver.Update();
        if (g_receiver.IsReceiving())
        {
            if (g_receiver.HasNewData())
            {
                if (g_usbDevice.WriteReport())
                {
                    g_updateRate = time - lastUsbUpdate;
                    lastLedUpdate = time;
                    lastUsbUpdate = time;

                    LED_PORT |= _BV(LED_BIT);
                    g_receiver.ClearNewData();
                }
            }
        }
        else
        {
            if (time - lastLedUpdate >= 1000)
            {
                lastLedUpdate = time;
                LED_PIN |= _BV(LED_BIT);
            }

            g_usbDevice.WriteReport();
        }

        auto signalSource = g_receiver.GetSignalSource();
        if (signalSource != lastSource)
        {
            printf_P(PSTR("Signal source: %u\n"), (uint8_t)signalSource);
            lastSource = signalSource;
        }

#if HIDRCJOY_DEBUG
        g_board.m_debug.ToggleD11();
#endif
    }

    return 0;
}
