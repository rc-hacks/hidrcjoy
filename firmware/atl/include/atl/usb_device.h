//
// usb_device.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <atl/bootloader.h>
#include <atl/interrupts.h>
#include <atl/usb_driver.h>
#include <atl/usb_endpoint.h>
#include <atl/usb_spec.h>
#include <stdint.h>
#include <stddef.h>

namespace atl
{
    enum class RequestStatus : uint8_t
    {
        Success,
        NotHandled,
        Error,
    };

    template<typename T, bool enableControlEndpointInterrupts = true>
    class UsbDeviceT : protected UsbDriver
    {
    public:
        enum class State : uint8_t
        {
            Disconnected,
            Attached,
            Powered,
            Default,
            Addressed,
            Configured,
            Suspended,
        };

        enum class LanguageId : uint16_t
        {
            German = 0x0407,    // German (de-DE)
            English = 0x0409,   // English (en-US)
            Spanish = 0x040A,   // Spanish (es-ES)
            French = 0x040C,    // French (fr-FR)
            Japanese = 0x0411,  // Japanese (ja-JP)
            Russian = 0x0419,   // Russian (ru-RU)
            Chinese = 0x0804,   // Chinese (zh-CN)
        };

    public:
        void Attach()
        {
            m_state = State::Attached;
            m_status = 0;
            m_configuration = 0;

            InitializeDriver();
            StartDevice();
            ClearWakeUpInterrupt();
            ClearSuspendInterrupt();
            EnableDefaultInterrupts();

            m_state = State::Powered;
        }

        void Detach()
        {
            UsbEndpoint::CancelWait();

            DetachDevice();
            DisableClock();
            m_state = State::Disconnected;
        }

        void Shutdown()
        {
            ShutdownDriver();
        }

        void ProcessTask()
        {
            if (!enableControlEndpointInterrupts)
            {
                UsbSaveEndpoint endpoint(ControlEndpoint);
                if (IsSetupReceived())
                {
                    ATL_USB_DEBUG_PRINT("+++ RXSTPI\n");
                    static_cast<T*>(this)->ProcessSetupPacket();
                    ATL_USB_DEBUG_PRINT("--- RXSTPI\n");
                }
            }
        }

        void SendRemoteWakeUp()
        {
            EnableClock();

            if ((m_status & UsbDeviceStatusRemoteWakeup) != 0)
            {
                InitiateRemoteWakeUp();
                m_status &= ~UsbDeviceStatusRemoteWakeup;
            }
        }

    public:
        State GetState() const { return m_state; }
        uint8_t GetConfiguration() const { return m_configuration; }
        bool IsEnumerated() const { return m_configuration != 0; }
        bool IsSuspended() const { return m_state == State::Suspended; }
        bool IsConfigured() const { return m_state == State::Configured; }
        bool IsSelfPowered() const { return (m_status & UsbDeviceStatusSelfPowered) != 0; }
        bool IsRemoteWakeupEnabled() const { return (m_status & UsbDeviceStatusRemoteWakeup) != 0; }

    public:
        void OnGeneralInterrupt()
        {
            if (IsStartOfFrameInterrupt() && IsStartOfFrameInterruptEnabled())
            {
                ClearStartOfFrameInterrupt();
                static_cast<T*>(this)->OnEventStartOfFrame();
            }

            if (IsSuspendInterrupt() && IsSuspendInterruptEnabled())
            {
                ATL_USB_DEBUG_PRINT("+++ SUSPI\n");
                UsbEndpoint::CancelWait();
                ClearWakeUpInterrupt();
                ClearSuspendInterrupt();
                DisableSuspendInterrupt();
                EnableWakeUpInterrupt();
                //DisableClock();
                m_state = State::Suspended;
                static_cast<T*>(this)->OnEventSuspend();
                ATL_USB_DEBUG_PRINT("--- SUSPI\n");
            }

            if (IsWakeUpInterrupt() && IsWakeUpInterruptEnabled())
            {
                ATL_USB_DEBUG_PRINT("+++ WAKEUPI\n");
                //EnableClock();
                ClearWakeUpInterrupt();
                DisableWakeUpInterrupt();
                EnableSuspendInterrupt();

                if (m_state == State::Suspended)
                {
                    if (m_configuration > 0)
                    {
                        m_state = State::Configured;
                    }
                    else if (IsAddressed())
                    {
                        m_state = State::Addressed;
                    }
                    else
                    {
                        m_state = State::Default;
                    }
                }

                static_cast<T*>(this)->OnEventWakeUp();
                ATL_USB_DEBUG_PRINT("--- WAKEUPI\n");
            }

            if (IsResumeInterrupt() && IsResumeInterruptEnabled())
            {
                ATL_USB_DEBUG_PRINT("+++ EORSMI\n");
                ClearResumeInterrupt();
                ATL_USB_DEBUG_PRINT("--- EORSMI\n");
            }

            if (IsEndOfResetInterrupt() && IsEndOfResetInterruptEnabled())
            {
                ATL_USB_DEBUG_PRINT("+++ EORSTI\n");
                UsbEndpoint::CancelWait();
                ClearEndOfResetInterrupt();
                static_cast<T*>(this)->ConfigureControlEndpoint();

                if (enableControlEndpointInterrupts)
                {
                    EnableSetupReceivedInterrupt();
                }

                m_state = State::Default;
                m_configuration = 0;
                static_cast<T*>(this)->OnEventReset();
                ATL_USB_DEBUG_PRINT("--- EORSTI\n");
            }
        }

        void OnEndpointInterrupt()
        {
            UsbSaveEndpoint endpoint(ControlEndpoint);
            if (IsSetupReceived())
            {
                ATL_USB_DEBUG_PRINT("+++ RXSTPI\n");
                DisableSetupReceivedInterrupt();
                Interrupts::Enable();
                static_cast<T*>(this)->ProcessSetupPacket();
                SelectEndpoint(ControlEndpoint);
                Interrupts::Disable();
                EnableSetupReceivedInterrupt();
                ATL_USB_DEBUG_PRINT("--- RXSTPI\n");
            }
        }

    protected:
        void ProcessSetupPacket()
        {
            AcknowledgeControlOutReceived();

            UsbRequest request;
            ReadData(reinterpret_cast<uint8_t*>(&request), sizeof(request));

            ATL_USB_DEBUG_PRINT("Request: bmRequestType=%02X, bRequest=%02X, wValue=%04X, wIndex=%04X, wLength=%04X\n",
                request.bmRequestType, request.bRequest, request.wValue, request.wIndex, request.wLength);

            AcknowledgeSetupReceived();
            if (static_cast<T*>(this)->ProcessRequest(request) != RequestStatus::Success)
            {
                EnableStallHandshake();
            }
        }

        RequestStatus ProcessRequest(const UsbRequest& request)
        {
            switch (request.bRequest)
            {
            case UsbRequestGetStatus:
                return static_cast<T*>(this)->ProcessGetStatus(request);
            case UsbRequestClearFeature:
                return static_cast<T*>(this)->ProcessSetFeature(request, false);
            case UsbRequestSetFeature:
                return static_cast<T*>(this)->ProcessSetFeature(request, true);
            case UsbRequestSetAddress:
                return static_cast<T*>(this)->ProcessSetAddress(request);
            case UsbRequestGetDescriptor:
                return static_cast<T*>(this)->ProcessGetDescriptor(request);
            case UsbRequestGetConfiguration:
                return static_cast<T*>(this)->ProcessGetConfiguration(request);
            case UsbRequestSetConfiguration:
                return static_cast<T*>(this)->ProcessSetConfiguration(request);
            default:
                return RequestStatus::NotHandled;
            }
        }

        RequestStatus ProcessGetStatus(const UsbRequest& request)
        {
            uint8_t status = 0;
            if (request.bmRequestType == RequestTypeStandardDeviceIn)
            {
                status = m_status;
            }
            else if (request.bmRequestType == RequestTypeStandardInterfaceIn)
            {
                status = 0;
            }
            else if (request.bmRequestType == RequestTypeStandardEndpointIn)
            {
                uint8_t endpoint = GetEndpointFromIndex(request.wIndex);
                if (endpoint >= MaxEndpoints)
                    return RequestStatus::Error;

                SelectEndpoint(endpoint);
                status = IsStallHandshakeRequested() ? UsbEndpointStatusHalt : 0;
                SelectEndpoint(ControlEndpoint);
            }
            else
            {
                return RequestStatus::NotHandled;
            }

            uint16_t result = status;
            return WriteControlData(request.wLength, &result, sizeof(result), MemoryType::Ram);
        }

        RequestStatus ProcessSetFeature(const UsbRequest& request, bool value)
        {
            if (request.bmRequestType == RequestTypeStandardDeviceOut)
            {
                uint8_t feature = static_cast<uint8_t>(request.wValue);
                if (feature == UsbFeatureDeviceRemoteWakeup)
                {
                    if (value)
                    {
                        m_status |= UsbDeviceStatusRemoteWakeup;
                    }
                    else
                    {
                        m_status &= ~UsbDeviceStatusRemoteWakeup;
                    }

                    return CompleteControlRequest();
                }
            }
            else if (request.bmRequestType == RequestTypeStandardEndpointOut)
            {
                uint8_t feature = static_cast<uint8_t>(request.wValue);
                if (feature == UsbFeatureEndpointHalt)
                {
                    uint8_t endpoint = GetEndpointFromIndex(request.wIndex);
                    if (endpoint >= MaxEndpoints)
                        return RequestStatus::Error;

                    if (endpoint != ControlEndpoint)
                    {
                        SelectEndpoint(endpoint);

                        if (!IsEndpointEnabled())
                        {
                            SelectEndpoint(ControlEndpoint);
                            return RequestStatus::Error;
                        }

                        if (value)
                        {
                            EnableStallHandshake();
                        }
                        else
                        {
                            DisableStallHandshake();
                            ResetEndpoint(endpoint);
                            ResetDataToggle();
                        }

                        SelectEndpoint(ControlEndpoint);
                    }

                    return CompleteControlRequest();
                }
            }

            return RequestStatus::NotHandled;
        }

        RequestStatus ProcessSetAddress(const UsbRequest& request)
        {
            if (request.bmRequestType == RequestTypeStandardDeviceOut)
            {
                uint8_t address = static_cast<uint8_t>(request.wValue);
                ConfigureAddress(address);
                SendControlIn();
                while (!IsInReady());
                EnableAddress();
                m_state = State::Addressed;
                return RequestStatus::Success;
            }
            else
            {
                return RequestStatus::NotHandled;
            }
        }

        RequestStatus ProcessGetDescriptor(const UsbRequest& request)
        {
            if ((request.bmRequestType & (UsbRequestTypeDirection | UsbRequestTypeType)) == (UsbRequestTypeDeviceToHost | UsbRequestTypeStandard))
            {
                return static_cast<T*>(this)->GetDescriptor(request);
            }
            else
            {
                return RequestStatus::NotHandled;
            }
        }

        RequestStatus ProcessGetConfiguration(const UsbRequest& request)
        {
            if (request.bmRequestType == RequestTypeStandardDeviceIn)
            {
                uint8_t configuration = m_configuration;
                return WriteControlData(request.wLength, &configuration, sizeof(configuration), MemoryType::Ram);
            }
            else
            {
                return RequestStatus::NotHandled;
            }
        }

        RequestStatus ProcessSetConfiguration(const UsbRequest& request)
        {
            if (request.bmRequestType == RequestTypeStandardDeviceOut)
            {
                auto status = CompleteControlRequest();
                m_state = State::Configured;
                m_configuration = static_cast<uint8_t>(request.wValue);
                static_cast<T*>(this)->OnEventConfigurationChanged();
                return status;
            }
            else
            {
                return RequestStatus::NotHandled;
            }
        }

    protected:
        void ConfigureControlEndpoint()
        {
            ConfigureEndpoint(ControlEndpoint, EndpointType::Control, EndpointDirection::Out, DefaultControlEndpointSize, EndpointBanks::One);
        }

        RequestStatus GetDescriptor(const UsbRequest& request)
        {
            return RequestStatus::NotHandled;
        }

        void OnEventReset()
        {
        }

        void OnEventStartOfFrame()
        {
        }

        void OnEventConfigurationChanged()
        {
        }

        void OnEventSuspend()
        {
        }

        void OnEventWakeUp()
        {
        }

    protected:
        static RequestStatus CompleteControlRequest()
        {
            SendControlIn();
            return RequestStatus::Success;
        }

        RequestStatus ReadControlData(void* buffer, size_t size)
        {
            UsbControlOutEndpoint endpoint;
            endpoint.ReadData(buffer, size);
            return MapStatus(endpoint.CompleteTransfer());
        }

        RequestStatus WriteControlData(uint16_t bytesRequested, const void* buffer, size_t size, MemoryType memoryType)
        {
            UsbControlInEndpoint endpoint(bytesRequested);
            endpoint.WriteData(buffer, size, memoryType);
            return MapStatus(endpoint.CompleteTransfer());
        }

        RequestStatus SendLanguageIdDescriptor(uint16_t bytesRequested, LanguageId language)
        {
            return SendLanguageIdDescriptor(bytesRequested, static_cast<uint16_t>(language));
        }

        RequestStatus SendLanguageIdDescriptor(uint16_t bytesRequested, uint16_t language)
        {
            return SendLanguageIdDescriptor(bytesRequested, &language, 1, MemoryType::Ram);
        }

        RequestStatus SendLanguageIdDescriptor(uint16_t bytesRequested, const uint16_t* languages, uint8_t count, MemoryType memoryType)
        {
            UsbControlInEndpoint endpoint(bytesRequested);
            endpoint.WriteByte(2 + count * 2);
            endpoint.WriteByte(UsbDescriptorTypeString);

            for (uint8_t i = 0; i < count; i++)
            {
                uint16_t language = Memory::ReadUInt16(languages + i, memoryType);
                endpoint.WriteByte(static_cast<uint8_t>(language >> 0));
                endpoint.WriteByte(static_cast<uint8_t>(language >> 8));
            }

            return MapStatus(endpoint.CompleteTransfer());
        }

        RequestStatus SendStringDescriptor(uint16_t bytesRequested, const char* string, uint8_t chars)
        {
            return SendStringDescriptor(bytesRequested, string, chars, MemoryType::Ram);
        }

        RequestStatus SendStringDescriptor_P(uint16_t bytesRequested, const char* string, uint8_t chars)
        {
            return SendStringDescriptor(bytesRequested, string, chars, MemoryType::Progmem);
        }

        RequestStatus SendStringDescriptor(uint16_t bytesRequested, const char* string, uint8_t chars, MemoryType memoryType)
        {
            UsbControlInEndpoint endpoint(bytesRequested);
            endpoint.WriteByte(2 + chars * 2);
            endpoint.WriteByte(UsbDescriptorTypeString);

            for (uint8_t i = 0; i < chars; i++)
            {
                endpoint.WriteData(string + i, sizeof(char), memoryType);
                endpoint.WriteByte(0);
            }

            return MapStatus(endpoint.CompleteTransfer());
        }

        RequestStatus SendBuiltinSerialStringDescriptor(uint16_t bytesRequested)
        {
            uint8_t serial[10];
            Bootloader::GetSerialNumber(serial, sizeof(serial));

            UsbControlInEndpoint endpoint(bytesRequested);
            endpoint.WriteByte(2 + sizeof(serial) * 4);
            endpoint.WriteByte(UsbDescriptorTypeString);

            for (uint8_t i = 0; i < sizeof(serial); i++)
            {
                uint8_t byte = serial[i];
                endpoint.WriteByte(NibbleToHex(byte >> 4));
                endpoint.WriteByte(0);
                endpoint.WriteByte(NibbleToHex(byte & 0xF));
                endpoint.WriteByte(0);
            }

            return MapStatus(endpoint.CompleteTransfer());
        }

        static RequestStatus MapStatus(EndpointStatus status)
        {
            switch (status)
            {
            case EndpointStatus::Success:
                return RequestStatus::Success;
            default:
                return RequestStatus::Error;
            }
        }

    private:
        static uint8_t GetEndpointFromIndex(uint8_t index)
        {
            return static_cast<uint8_t>(index) & EndpointMask;
        }

        static void ReadData(uint8_t* buffer, size_t size)
        {
            for (size_t i = 0; i < size; i++)
            {
                buffer[i] = ReadByte();
            }
        }

        static uint8_t NibbleToHex(uint8_t nibble)
        {
            return nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
        }

    private:
        volatile State m_state = State::Disconnected;
        volatile uint8_t m_status = 0;
        volatile uint8_t m_configuration = 0;
    };
}
