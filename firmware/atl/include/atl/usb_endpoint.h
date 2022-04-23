//
// usb_endpoint.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <atl/memory.h>
#include <atl/usb_driver.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <stddef.h>
#include <stdint.h>

namespace atl
{
    enum class EndpointStatus : uint8_t
    {
        Success,
        Cancelled,
        Timeout,
    };

    class UsbEndpoint : public UsbDriver
    {
    public:
        UsbEndpoint() = default;
        UsbEndpoint(const UsbEndpoint&) = delete;
        UsbEndpoint& operator=(const UsbEndpoint& other) = delete;
        UsbEndpoint(UsbEndpoint&&) = delete;
        UsbEndpoint& operator=(UsbEndpoint&& other) = delete;

    public:
        static void CancelWait()
        {
            GetContext().cancel = true;
        }

        static EndpointStatus WaitForInReady()
        {
            BeginWait();

#if ATL_USB_DEBUG && ATL_USB_DEBUG_EXT
            uint8_t lastUEINTX = 0xFF;
            uint8_t lastUDINT = 0xFF;
#endif
            while (true)
            {
                uint8_t ueintx = UEINTX;
                uint8_t udint = UDINT;
#if ATL_USB_DEBUG && ATL_USB_DEBUG_EXT
                if (ueintx != lastUEINTX || udint != lastUDINT)
                {
                    lastUEINTX = ueintx;
                    lastUDINT = udint;
                    ATL_USB_DEBUG_PRINT("[%02X-%02X-%02X] ?TXINI\n", ueintx, udint, UEBCLX);
                }
#endif
                if (IsWaitCancelled(udint))
                {
                    ATL_USB_DEBUG_PRINT("Cancelled!\n");
                    return EndpointStatus::Cancelled;
                }
                else if (IsNakOutReceived(ueintx))
                {
                    ATL_USB_DEBUG_PRINT("NAKOUT!\n");
                    return EndpointStatus::Cancelled;
                }
                else if (IsInReady(ueintx))
                {
                    return EndpointStatus::Success;
                }
            }
        }

        static EndpointStatus WaitForOutReceived()
        {
            BeginWait();

#if ATL_USB_DEBUG && ATL_USB_DEBUG_EXT
            uint8_t lastUEINTX = 0xFF;
            uint8_t lastUDINT = 0xFF;
#endif
            while (true)
            {
                uint8_t ueintx = UEINTX;
                uint8_t udint = UDINT;
#if ATL_USB_DEBUG && ATL_USB_DEBUG_EXT
                if (ueintx != lastUEINTX || udint != lastUDINT)
                {
                    lastUEINTX = ueintx;
                    lastUDINT = udint;
                    ATL_USB_DEBUG_PRINT("[%02X-%02X-%02X] ?RXOUTI\n", ueintx, udint, UEBCLX);
                }
#endif
                if (IsWaitCancelled(udint))
                {
                    ATL_USB_DEBUG_PRINT("Cancelled!\n");
                    return EndpointStatus::Cancelled;
                }
                else if (IsOutReceived(ueintx))
                {
                    return EndpointStatus::Success;
                }
            }
        }

    private:
        struct Context
        {
            volatile bool cancel;

        };

        static Context& GetContext()
        {
            static Context context;
            return context;
        }

        static void BeginWait()
        {
            GetContext().cancel = false;
        }

        static bool IsWaitCancelled(uint8_t udint = UDINT)
        {
            return GetContext().cancel || (udint & ((1 << EORSTI) | (1 << SUSPI))) != 0;
        }
    };

    class UsbInEndpoint : protected UsbEndpoint
    {
    protected:
        UsbInEndpoint() = default;

    public:
        UsbInEndpoint(uint8_t endpoint)
        {
            SelectEndpoint(endpoint);
        }

    public:
        bool IsWriteAllowed() const
        {
            return IsReadWriteAllowed();
        }

    public:
        EndpointStatus WriteByte(uint8_t data)
        {
            return WriteData(&data, sizeof(data));
        }

        EndpointStatus WriteData(const void* buffer, size_t size, MemoryType memoryType = MemoryType::Ram)
        {
            const uint8_t* data = static_cast<const uint8_t*>(buffer);

            size_t bytesToTransmit = size;
            while (bytesToTransmit > 0)
            {
                EndpointStatus status = WaitForInReady();
                if (status != EndpointStatus::Success)
                {
                    return status;
                }

                while (bytesToTransmit > 0 && IsReadWriteAllowed())
                {
                    UsbDriver::WriteByte(Memory::ReadUInt8(data, memoryType));
                    data++;
                    bytesToTransmit--;
                }

                if (!IsReadWriteAllowed())
                {
                    SendIn();
                    m_flushRequired = false;
                }
                else
                {
                    m_flushRequired = true;
                }
            }

            return EndpointStatus::Success;
        }

        EndpointStatus CompleteTransfer(bool zlpRequired = false)
        {
            if (m_flushRequired || zlpRequired)
            {
                if (WaitForInReady() == EndpointStatus::Success)
                {
                    SendIn();
                }
            }

#if 0
            EndpointStatus status = WaitForOutReceived();
            if (status != EndpointStatus::Success)
            {
                return status;
            }

            AcknowledgeOutReceived();
#endif
            return EndpointStatus::Success;
        }

    private:
        bool m_flushRequired = false;
    };

    class UsbOutEndpoint : protected UsbEndpoint
    {
    protected:
        UsbOutEndpoint() = default;

    public:
        UsbOutEndpoint(uint8_t endpoint)
        {
            SelectEndpoint(endpoint);
        }

    public:
        bool IsReadAllowed() const { return IsReadWriteAllowed(); }

    public:
        EndpointStatus ReadData(void* buffer, size_t size)
        {
            uint8_t* data = static_cast<uint8_t*>(buffer);
            size_t bytesToRead = size;

            while (bytesToRead > 0)
            {
                EndpointStatus status = WaitForOutReceived();
                if (status != EndpointStatus::Success)
                {
                    return status;
                }

                while (bytesToRead > 0 && IsReadWriteAllowed())
                {
                    *data = ReadByte();
                    data++;
                    bytesToRead--;
                }

                if (!IsReadWriteAllowed())
                {
                    AcknowledgeOutReceived();
                }
            }

            return EndpointStatus::Success;
        }
    };

    class UsbControlInEndpoint : protected UsbEndpoint
    {
        static const uint8_t endpointSize = DefaultControlEndpointSize;

    public:
        UsbControlInEndpoint(size_t bytesRequested) : m_bytesRequested(bytesRequested)
        {
        }

    public:
        size_t GetBytesRequested() const { return m_bytesRequested; }
        size_t GetBytesWritten() const { return m_bytesWritten; }

    public:
        EndpointStatus WriteByte(uint8_t data)
        {
            return WriteData(&data, sizeof(data));
        }

        EndpointStatus WriteData(const void* buffer, size_t size, MemoryType memoryType = MemoryType::Ram)
        {
            const uint8_t* data = static_cast<const uint8_t*>(buffer);

            size_t bytesToTransmit = size;
            if (m_bytesWritten + bytesToTransmit > m_bytesRequested)
            {
                bytesToTransmit = m_bytesRequested > m_bytesWritten ? m_bytesRequested - m_bytesWritten : 0;
            }

            m_bytesWritten += size;

            while (bytesToTransmit > 0)
            {
                EndpointStatus status = WaitForInReady();
                if (status != EndpointStatus::Success)
                {
                    return status;
                }

                uint8_t bytesInEndpoint = GetEndpointByteCount8();

                while (bytesToTransmit > 0 && bytesInEndpoint < endpointSize)
                {
                    UsbDriver::WriteByte(Memory::ReadUInt8(data, memoryType));
                    data++;
                    bytesToTransmit--;
                    bytesInEndpoint++;
                }

                if (bytesInEndpoint >= endpointSize)
                {
                    SendControlIn();
                    m_flushRequired = false;
                }
                else
                {
                    m_flushRequired = true;
                }
            }

            return EndpointStatus::Success;
        }

        EndpointStatus CompleteTransfer()
        {
            bool zlpRequired = m_bytesWritten < m_bytesRequested;
            if (m_flushRequired || zlpRequired)
            {
                if (WaitForInReady() == EndpointStatus::Success)
                {
                    SendControlIn();
                }
            }

            EndpointStatus status = WaitForOutReceived();
            if (status != EndpointStatus::Success)
            {
                return status;
            }

            AcknowledgeControlOutReceived();
            return EndpointStatus::Success;
        }

    private:
        size_t m_bytesRequested = 0;
        size_t m_bytesWritten = 0;
        bool m_flushRequired = false;
    };

    class UsbControlOutEndpoint : protected UsbEndpoint
    {
    public:
        EndpointStatus ReadData(void* buffer, size_t size)
        {
            uint8_t* data = static_cast<uint8_t*>(buffer);

            while (size > 0)
            {
                EndpointStatus status = WaitForOutReceived();
                if (status != EndpointStatus::Success)
                {
                    return status;
                }

                uint8_t bytesInEndpoint = GetEndpointByteCount8();

                while (size > 0 && bytesInEndpoint > 0)
                {
                    *data = ReadByte();
                    data++;
                    size--;
                    bytesInEndpoint--;
                }

                AcknowledgeControlOutReceived();
            }

            return EndpointStatus::Success;
        }

        EndpointStatus CompleteTransfer()
        {
            SendControlIn();
            return EndpointStatus::Success;
        }
    };

    template<typename T>
    class UsbSaveAndSelectEndpointT : public T
    {
    public:
        UsbSaveAndSelectEndpointT(uint8_t endpoint)
        {
            m_previousEndpoint = T::GetSelectedEndpoint();
            T::SelectEndpoint(endpoint);
        }

        ~UsbSaveAndSelectEndpointT()
        {
            T::SelectEndpoint(m_previousEndpoint);
        }

    private:
        uint8_t m_previousEndpoint;
    };

    using UsbSaveEndpoint = UsbSaveAndSelectEndpointT<UsbEndpoint>;
    using UsbSaveInEndpoint = UsbSaveAndSelectEndpointT<UsbInEndpoint>;
    using UsbSaveOutEndpoint = UsbSaveAndSelectEndpointT<UsbOutEndpoint>;
}
