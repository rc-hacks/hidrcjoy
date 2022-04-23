//
// usb_cdc_device.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <atl/usb_cdc_spec.h>
#include <atl/usb_device.h>

namespace atl
{
    template<typename T>
    class UsbCdcDeviceT : public UsbDeviceT<T>
    {
        static const uint8_t acmInterface = 0;
        static const uint8_t dataInterface = 1;
        static const uint8_t acmEndpoint = 1;
        static const uint8_t txEndpoint = 2;
        static const uint8_t rxEndpoint = 3;
        static const uint16_t acmEndpointSize = 8;
        static const uint16_t txEndpointSize = 64;
        static const uint16_t rxEndpointSize = 64;

        using base = UsbDeviceT<T>;

    public:
        UsbCdcDeviceT() = default;
        UsbCdcDeviceT(const UsbCdcDeviceT&) = delete;
        UsbCdcDeviceT& operator=(const UsbCdcDeviceT& other) = delete;
        UsbCdcDeviceT(UsbCdcDeviceT&&) = delete;
        UsbCdcDeviceT& operator=(UsbCdcDeviceT&& other) = delete;

    public:
        uint32_t GetBaudRate() const { return m_lineCoding.dwDTERate; }
        uint8_t GetDataBits() const { return m_lineCoding.bDataBits; }
        CdcStopBits GetStopBits() const { return static_cast<CdcStopBits>(m_lineCoding.bCharFormat); }
        CdcParity GetParity() const { return static_cast<CdcParity>(m_lineCoding.bParityType); }

        uint8_t GetControlLineState() const { return m_controlLineState; }
        bool IsOpen() const { return m_controlLineState != 0; }
        bool IsDtrActive() const { return (m_controlLineState & CdcControlLineStateDtr) != 0; }
        bool IsRtsActive() const { return (m_controlLineState & CdcControlLineStateRts) != 0; }

    public:
        void Open()
        {
            m_flushRequired = false;
        }

        void Close()
        {
        }

        bool IsCharAvailable() const
        {
            return GetCharsAvailable() > 0;
        }

        uint8_t GetCharsAvailable() const
        {
            UsbSaveEndpoint endpoint(rxEndpoint);
            return base::GetEndpointByteCount8();
        }

        char ReadChar()
        {
            char ch;
            ReadData(&ch, sizeof(ch));
            return ch;
        }

        void WriteChar(char ch)
        {
            WriteData(&ch, 1);
        }

        void ReadData(void* buffer, size_t size)
        {
            UsbSaveOutEndpoint endpoint(rxEndpoint);
            endpoint.ReadData(buffer, size);
        }

        void WriteData(const void* buffer, size_t size, MemoryType memoryType = MemoryType::Ram)
        {
            if (IsOpen())
            {
                m_flushRequired = false;
                UsbSaveInEndpoint endpoint(txEndpoint);
                endpoint.WriteData(buffer, size, memoryType);
                m_flushRequired = true;
            }
        }

        void Flush()
        {
            if (m_flushRequired)
            {
                UsbSaveInEndpoint endpoint(txEndpoint);
                endpoint.CompleteTransfer(true);
                m_flushRequired = false;
            }
        }

    protected:
        void OnEventLineCodingChanged()
        {
        }

        void OnEventControlLineStateChanged()
        {
        }

        void OnEventBreakSent(uint16_t /* breakLength */)
        {
        }

    protected:
        void ConfigureEndpoints()
        {
            base::ConfigureEndpoint(acmEndpoint, base::EndpointType::Interrupt, base::EndpointDirection::In, acmEndpointSize, base::EndpointBanks::One);
            base::ConfigureEndpoint(txEndpoint, base::EndpointType::Bulk, base::EndpointDirection::In, txEndpointSize, base::EndpointBanks::One);
            base::ConfigureEndpoint(rxEndpoint, base::EndpointType::Bulk, base::EndpointDirection::Out, rxEndpointSize, base::EndpointBanks::One);
        }

        RequestStatus ProcessRequest(const UsbRequest& request)
        {
            uint8_t interface = static_cast<uint8_t>(request.wIndex);
            if (interface == acmInterface)
            {
                if (request.bmRequestType == RequestTypeClassInterfaceIn)
                {
                    switch (request.bRequest)
                    {
                    case CdcRequestGetLineCoding:
                    {
                        UsbControlInEndpoint endpoint(request.wLength);
                        endpoint.WriteData(const_cast<const CdcLineCoding*>(&m_lineCoding), sizeof(CdcLineCoding));
                        return base::MapStatus(endpoint.CompleteTransfer());
                    }
                    }
                }
                else if (request.bmRequestType == RequestTypeClassInterfaceOut)
                {
                    switch (request.bRequest)
                    {
                    case CdcRequestSetLineCoding:
                    {
                        UsbControlOutEndpoint endpoint;
                        endpoint.ReadData(const_cast<CdcLineCoding*>(&m_lineCoding), sizeof(CdcLineCoding));
                        static_cast<T*>(this)->OnEventLineCodingChanged();
                        return base::MapStatus(endpoint.CompleteTransfer());
                    }
                    case CdcRequestSetControlLineState:
                        m_controlLineState = static_cast<uint8_t>(request.wValue);
                        static_cast<T*>(this)->OnEventControlLineStateChanged();
                        return base::CompleteControlRequest();
                    case CdcRequestSendBreak:
                        static_cast<T*>(this)->OnEventBreakSent(request.wValue);
                        return base::CompleteControlRequest();
                    }
                }
            }

            return base::ProcessRequest(request);
        }

        static MemoryBuffer GetConfigurationDescriptor()
        {
            static const ConfigurationDescriptor descriptor PROGMEM =
            {
                {
                    sizeof(UsbInterfaceAssociationDescriptor),
                    UsbDescriptorTypeInterfaceAssociation,
                    acmInterface,
                    2, // bInterfaceCount
                    CdcInterfaceClassCommunications,
                    CdcInterfaceSubclassCommunicationsAcm,
                    CdcInterfaceProtocolCommunicationsNone,
                    0 // iFunction
                },
                {
                    {
                        sizeof(UsbInterfaceDescriptor),
                        UsbDescriptorTypeInterface,
                        acmInterface,
                        0, // bAlternateSetting
                        1, // bNumEndpoints
                        CdcInterfaceClassCommunications,
                        CdcInterfaceSubclassCommunicationsAcm,
                        CdcInterfaceProtocolCommunicationsNone,
                        0 // iInterface
                    },
                    {
                        sizeof(CdcHeaderFunctionalDescriptor),
                        CdcDescriptorTypeInterface,
                        CdcDescriptorSubtypeHeader,
                        CdcVersion
                    },
                    {
                        sizeof(CdcAbstractControlManagementFunctionalDescriptor),
                        CdcDescriptorTypeInterface,
                        CdcDescriptorSubtypeAbstractControlManagement,
                        0x06 // bmCapabilities: sends break, line coding and serial state
                    },
                    {
                        sizeof(CdcUnionFunctionalDescriptor),
                        CdcDescriptorTypeInterface,
                        CdcDescriptorSubtypeUnion,
                        acmInterface,
                        dataInterface
                    },
                    {
                        sizeof(CdcCallManagementFunctionalDescriptor),
                        CdcDescriptorTypeInterface,
                        CdcDescriptorSubtypeCallManagement,
                        1, // bmCapabilities
                        1 // bDataInterface
                    },
                    {
                        sizeof(UsbEndpointDescriptor),
                        UsbDescriptorTypeEndpoint,
                        UsbEndpointAddressIn | acmEndpoint,
                        UsbEndpointTypeInterrupt,
                        acmEndpointSize,
                        /* TODO */ 0x40 // bInterval
                    },
                },
                {
                    {
                        sizeof(UsbInterfaceDescriptor),
                        UsbDescriptorTypeInterface,
                        dataInterface,
                        0, // bAlternateSetting
                        2, // bNumEndpoints
                        CdcInterfaceClassData,
                        CdcInterfaceSubclassDataNone,
                        CdcInterfaceProtocolDataNone,
                        0 // iInterface
                    },
                    {
                        sizeof(UsbEndpointDescriptor),
                        UsbDescriptorTypeEndpoint,
                        UsbEndpointAddressIn | txEndpoint,
                        UsbEndpointTypeBulk,
                        txEndpointSize,
                        0 // bInterval
                    },
                    {
                        sizeof(UsbEndpointDescriptor),
                        UsbDescriptorTypeEndpoint,
                        UsbEndpointAddressOut | rxEndpoint,
                        UsbEndpointTypeBulk,
                        rxEndpointSize,
                        0 // bInterval
                    },
                },
            };

            return MemoryBuffer(&descriptor, sizeof(descriptor), MemoryType::Progmem);
        }

        struct ATL_ATTRIBUTE_PACKED ConfigurationDescriptor
        {
            UsbInterfaceAssociationDescriptor iad;
            struct ATL_ATTRIBUTE_PACKED Control
            {
                UsbInterfaceDescriptor id;
                CdcHeaderFunctionalDescriptor hfd;
                CdcAbstractControlManagementFunctionalDescriptor acmfd;
                CdcUnionFunctionalDescriptor ufd;
                CdcCallManagementFunctionalDescriptor cmfd;
                UsbEndpointDescriptor ed;
            } cd;
            struct ATL_ATTRIBUTE_PACKED Data
            {
                UsbInterfaceDescriptor id;
                UsbEndpointDescriptor in;
                UsbEndpointDescriptor out;
            } dd;
        };

    private:
        volatile CdcLineCoding m_lineCoding = { 57600, 0, 0, 0 };
        volatile uint8_t m_controlLineState = 0;
        volatile bool m_flushRequired = false;
    };
}
