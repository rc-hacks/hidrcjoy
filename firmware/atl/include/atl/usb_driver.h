//
// usb_driver.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <atl/debug.h>
#include <avr/io.h>
#include <stdint.h>

#ifndef ATL_USB_DEBUG_PRINT
#if ATL_USB_DEBUG
#define ATL_USB_DEBUG_PRINT(...) ATL_DEBUG_PRINT(__VA_ARGS__)
#else
#define ATL_USB_DEBUG_PRINT(...) ((void)0)
#endif
#endif

#if defined(__AVR_ATmega8U2__) || defined(__AVR_ATmega16U2__) || defined(__AVR_ATmega32U2__) || defined(__AVR_AT90USB82__) || defined(__AVR_AT90USB162__)
#define ATL_USB_SERIES2 1
#elif defined(__AVR_ATmega16U4__) || defined(__AVR_ATmega32U4__)
#define ATL_USB_SERIES4 1
#elif defined(__AVR_ATmega32U6__) || defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__)
#define ATL_USB_SERIES6 1
#elif defined(__AVR_AT90USB647__) || defined(__AVR_AT90USB1287__)
#define ATL_USB_SERIES7 1
#else
#error Unsupported device.
#endif

namespace atl
{
    class UsbDriver
    {
    public:
#if defined(ATL_USB_SERIES4) || defined(ATL_USB_SERIES6)|| defined(ATL_USB_SERIES7)
        static const uint8_t MaxEndpoints = 7;
#else
        static const uint8_t MaxEndpoints = 5;
#endif
        static const uint8_t EndpointMask = 0x7F;
        static const uint8_t AddressMask = 0x7F;
        static const uint8_t ControlEndpoint = 0;
        static const uint8_t DefaultControlEndpointSize = 8;

        enum class EndpointType : uint8_t
        {
            Control = 0,
            Isochronous = 1,
            Bulk = 2,
            Interrupt = 3,
        };

        enum class EndpointDirection : uint8_t
        {
            Out = 0,
            In = 1,
        };

        enum class EndpointSize : uint8_t
        {
            Size8 = 0,
            Size16 = 1,
            Size32 = 2,
            Size64 = 3,
#if defined(ATL_USB_SERIES4)
            Size128 = 4,
            Size256 = 5,
            Size512 = 6,
            Size1024 = 7,
#endif
        };

        enum class EndpointBanks : uint8_t
        {
            One = 0,
            Two = 1,
        };

    public:
        static void InitializeDriver()
        {
            EnableRegulator();
            DisableController();
            EnableController();
            EnableVBusPad();
        }

        static void ShutdownDriver()
        {
            DisableAllInterrupts();
            ClearAllInterrupts();
            DetachDevice();
            DisableClock();
            DisableVBusPad();
            DisableController();
            DisableRegulator();
        }

        static void StartDevice()
        {
            FreezeClock();
            EnableClock();
            AttachDevice();
        }

        static void EnableClock()
        {
            if (!IsPllReady())
            {
                EnablePll();
                while (!IsPllReady());
            }

            UnfreezeClock();
        }

        static void DisableClock()
        {
            FreezeClock();
            DisablePll();
        }

    public:
        static void EnableRegulator()
        {
#if defined(ATL_USB_SERIES4) || defined(ATL_USB_SERIES6)|| defined(ATL_USB_SERIES7)
            UHWCON |= _BV(UVREGE);
#else
            REGCR &= ~_BV(REGDIS);
#endif
        }

        static void DisableRegulator()
        {
#if defined(ATL_USB_SERIES4) || defined(ATL_USB_SERIES6)|| defined(ATL_USB_SERIES7)
            UHWCON &= ~_BV(UVREGE);
#else
            REGCR |= _BV(REGDIS);
#endif
        }

        static void EnableVBusPad()
        {
#if defined(ATL_USB_SERIES4) || defined(ATL_USB_SERIES6)|| defined(ATL_USB_SERIES7)
            USBCON |= _BV(OTGPADE);
#endif
        }

        static void DisableVBusPad()
        {
#if defined(ATL_USB_SERIES4) || defined(ATL_USB_SERIES6)|| defined(ATL_USB_SERIES7)
            USBCON &= ~_BV(OTGPADE);
#endif
        }

        static void FreezeClock()
        {
            USBCON |= _BV(FRZCLK);
        }

        static void UnfreezeClock()
        {
            USBCON &= ~_BV(FRZCLK);
        }

        static void EnableController()
        {
            USBCON |= _BV(USBE);
        }

        static void DisableController()
        {
            USBCON &= ~_BV(USBE);
        }

        static void AttachDevice()
        {
            UDCON &= ~_BV(DETACH);
        }

        static void DetachDevice()
        {
            UDCON |= _BV(DETACH);
        }

        static bool IsAttached()
        {
            return (UDCON & _BV(DETACH)) == 0;
        }

        static void InitiateRemoteWakeUp()
        {
            UDCON |= _BV(RMWKUP);
        }

        static bool IsRemoteWakeUpPending()
        {
            return (UDCON & _BV(RMWKUP)) != 0;
        }

        static void SelectLowSpeedMode()
        {
#if defined(ATL_USB_SERIES4) || defined(ATL_USB_SERIES6)|| defined(ATL_USB_SERIES7)
            UDCON |= _BV(LSM);
#endif
        }

        static void SelectFullSpeedMode()
        {
#if defined(ATL_USB_SERIES4) || defined(ATL_USB_SERIES6)|| defined(ATL_USB_SERIES7)
            UDCON &= ~_BV(LSM);
#endif
        }

        static void ResetCpuOnSignal()
        {
#if defined(ATL_USB_SERIES4)
            UDCON |= _BV(RSTCPU);
#endif
        }

        static void ResetUsbOnSignal()
        {
#if defined(ATL_USB_SERIES4)
            UDCON &= ~_BV(RSTCPU);
#endif
        }

        static void EnablePll()
        {
#if (F_CPU == 8000000)
#if defined(ATL_USB_SERIES6) || defined(ATL_USB_SERIES7)
#define ATL_USB_PLLP (_BV(PLLP1) | _BV(PLLP0))
#else
#define ATL_USB_PLLP 0
#endif
#elif (F_CPU == 16000000)
#if defined(ATL_USB_SERIES6) || defined(ATL_USB_SERIES7)
#if defined(__AVR_AT90USB1286__) || defined(__AVR_AT90USB1287__)
#define ATL_USB_PLLP (_BV(PLLP2) | _BV(PLLP0))
#else
#define ATL_USB_PLLP (_BV(PLLP2) | _BV(PLLP1))
#endif
#elif defined(ATL_USB_SERIES4)
#define ATL_USB_PLLP _BV(PINDIV)
#else
#define ATL_USB_PLLP _BV(PLLP0)
#endif
#else
#error F_CPU must be 8MHz or 16MHz
#endif
            PLLCSR = ATL_USB_PLLP | _BV(PLLE);
        }

        static void DisablePll()
        {
            PLLCSR = 0;
        }

        static bool IsPllReady()
        {
            return (PLLCSR & _BV(PLOCK)) != 0;
        }

        static void EnableSuspendInterrupt()
        {
            UDIEN |= _BV(SUSPE);
        }

        static void DisableSuspendInterrupt()
        {
            UDIEN &= ~_BV(SUSPE);
        }

        static bool IsSuspendInterruptEnabled()
        {
            return (UDIEN & _BV(SUSPE)) != 0;
        }

        static bool IsSuspendInterrupt()
        {
            return (UDINT & _BV(SUSPI)) != 0;
        }

        static void ClearSuspendInterrupt()
        {
            UDINT &= ~_BV(SUSPI);
        }

        static void EnableStartOfFrameInterrupt()
        {
            UDIEN |= _BV(SOFE);
        }

        static void DisableStartOfFrameInterrupt()
        {
            UDIEN &= ~_BV(SOFE);
        }

        static bool IsStartOfFrameInterruptEnabled()
        {
            return (UDIEN & _BV(SOFE)) != 0;
        }

        static bool IsStartOfFrameInterrupt()
        {
            return (UDINT & _BV(SOFI)) != 0;
        }

        static void ClearStartOfFrameInterrupt()
        {
            UDINT &= ~_BV(SOFI);
        }

        static void EnableEndOfResetInterrupt()
        {
            UDIEN |= _BV(EORSTE);
        }

        static void DisableEndOfResetInterrupt()
        {
            UDIEN &= ~_BV(EORSTE);
        }

        static bool IsEndOfResetInterruptEnabled()
        {
            return (UDIEN & _BV(EORSTE)) != 0;
        }

        static bool IsEndOfResetInterrupt()
        {
            return (UDINT & _BV(EORSTI)) != 0;
        }

        static void ClearEndOfResetInterrupt()
        {
            UDINT &= ~_BV(EORSTI);
        }

        static void EnableWakeUpInterrupt()
        {
            UDIEN |= _BV(WAKEUPE);
        }

        static void DisableWakeUpInterrupt()
        {
            UDIEN &= ~_BV(WAKEUPE);
        }

        static bool IsWakeUpInterruptEnabled()
        {
            return (UDIEN & _BV(WAKEUPE)) != 0;
        }

        static bool IsWakeUpInterrupt()
        {
            return (UDINT & _BV(WAKEUPI)) != 0;
        }

        static void ClearWakeUpInterrupt()
        {
            UDINT &= ~_BV(WAKEUPI);
        }

        static void EnableResumeInterrupt()
        {
            UDIEN |= _BV(EORSME);
        }

        static void DisableResumeInterrupt()
        {
            UDIEN &= ~_BV(EORSME);
        }

        static bool IsResumeInterruptEnabled()
        {
            return (UDIEN & _BV(EORSME)) != 0;
        }

        static bool IsResumeInterrupt()
        {
            return (UDINT & _BV(EORSMI)) != 0;
        }

        static void ClearResumeInterrupt()
        {
            UDINT &= ~_BV(EORSMI);
        }

        static void EnableSetupReceivedInterrupt()
        {
            UEIENX |= _BV(RXSTPE);
        }

        static void DisableSetupReceivedInterrupt()
        {
            UEIENX &= ~_BV(RXSTPE);
        }

        static void EnableDefaultInterrupts()
        {
            UDIEN |= _BV(EORSTE) | _BV(SOFE) | _BV(SUSPE);
        }

        static void DisableAllInterrupts()
        {
            UDIEN = 0;
            UEIENX = 0;
        }

        static void ClearAllInterrupts()
        {
            UDINT = 0;
        }

        static void EnableAddress()
        {
            UDADDR |= _BV(ADDEN);
        }

        static void DisableAddress()
        {
            UDADDR &= ~_BV(ADDEN);
        }

        static bool IsAddressed()
        {
            return (UDADDR & _BV(ADDEN)) != 0;
        }

        static void ConfigureAddress(uint8_t address)
        {
            UDADDR = (UDADDR & _BV(ADDEN)) | (address & AddressMask);
        }

        static void SelectEndpoint(uint8_t endpoint)
        {
            UENUM = endpoint;
        }

        static uint8_t GetSelectedEndpoint()
        {
            return UENUM;
        }

        static void EnableEndpoint()
        {
            UECONX |= _BV(EPEN);
        }

        static void DisableEndpoint()
        {
            UECONX &= ~_BV(EPEN);
        }

        static bool IsEndpointEnabled()
        {
            return (UECONX & _BV(EPEN)) != 0;
        }

        static void ResetDataToggle()
        {
            UECONX |= _BV(RSTDT);
        }

        static void EnableStallHandshake()
        {
            ATL_USB_DEBUG_PRINT("STALLRQ!\n");
            UECONX |= _BV(STALLRQ);
        }

        static void DisableStallHandshake()
        {
            UECONX |= _BV(STALLRQC);
        }

        static bool IsStallHandshakeRequested()
        {
            return (UECONX & _BV(STALLRQ)) != 0;
        }

        static EndpointSize GetEndpointSize(uint16_t size)
        {
            switch (size)
            {
            case 8:
                return EndpointSize::Size8;
            case 16:
                return EndpointSize::Size16;
            case 32:
                return EndpointSize::Size32;
            case 64:
                return EndpointSize::Size64;
#if defined(ATL_USB_SERIES4)
            case 128:
                return EndpointSize::Size128;
            case 256:
                return EndpointSize::Size256;
            case 512:
                return EndpointSize::Size512;
            case 1024:
                return EndpointSize::Size1024;
#endif
            default:
                return EndpointSize::Size8;
            }
        }

        static void ConfigureEndpoint(uint8_t endpoint, EndpointType type, EndpointDirection direction, uint16_t size, EndpointBanks banks)
        {
            ConfigureEndpoint(endpoint, type, direction, GetEndpointSize(size), banks);
        }

        static void ConfigureEndpoint(uint8_t endpoint, EndpointType type, EndpointDirection direction, EndpointSize size, EndpointBanks banks)
        {
            uint8_t uecfg0x = (static_cast<uint8_t>(type) << EPTYPE0) | static_cast<uint8_t>(direction);
            uint8_t uecfg1x = (static_cast<uint8_t>(size) << EPSIZE0) | (static_cast<uint8_t>(banks) << EPBK0) | _BV(ALLOC);
            ConfigureEndpoint(endpoint, uecfg0x, uecfg1x);
        }

        static void ConfigureEndpoint(uint8_t endpoint, uint8_t uecfg0x, uint8_t uecfg1x)
        {
            ATL_USB_DEBUG_PRINT("UENUM=%d\n", endpoint);
            ATL_USB_DEBUG_PRINT("UECFG0X=%02X\n", uecfg0x);
            ATL_USB_DEBUG_PRINT("UECFG1X=%02X\n", uecfg1x);
            SelectEndpoint(endpoint);
            EnableEndpoint();
            UECFG0X = uecfg0x;
            UECFG1X = uecfg1x;
        }

        static bool IsEndpointConfigured()
        {
            return (UESTA0X & _BV(CFGOK)) != 0;
        }

        static void ResetEndpoint(uint8_t endpoint)
        {
            UERST = _BV(endpoint);
            UERST = 0;
        }

        static void ResetAllEndpoints()
        {
#if defined(ATL_USB_SERIES4) || defined(ATL_USB_SERIES6)|| defined(ATL_USB_SERIES7)
            UERST = _BV(EPRST6) | _BV(EPRST5) | _BV(EPRST4) | _BV(EPRST3) | _BV(EPRST2) | _BV(EPRST1);
#else
            UERST = _BV(EPRST4) | _BV(EPRST3) | _BV(EPRST2) | _BV(EPRST1);
#endif
            UERST = 0;
        }

        static uint8_t GetEndpointByteCount8()
        {
            return UEBCLX;
        }

#if defined(ATL_USB_SERIES4) || defined(ATL_USB_SERIES6)|| defined(ATL_USB_SERIES7)
        static uint8_t GetEndpointByteCount16()
        {
            return UEBCX;
        }
#endif

        static uint16_t GetFrameNumber()
        {
            return UDFNUM;
        }

        static uint8_t ReadByte()
        {
#if ATL_USB_DEBUG && ATL_USB_DEBUG_EXT
            ATL_USB_DEBUG_PRINT("[%02X-%02X] < ", UEINTX, UEBCLX);
            uint8_t data = UEDATX;
            ATL_USB_DEBUG_PRINT("%02X\n", data);
            return data;
#else
            return UEDATX;
#endif
        }

        static void WriteByte(uint8_t data)
        {
#if ATL_USB_DEBUG && ATL_USB_DEBUG_EXT
            ATL_USB_DEBUG_PRINT("[%02X-%02X] > %02X\n", UEINTX, UEBCLX, data);
#endif
            UEDATX = data;
        }

        static bool IsSetupReceived(uint8_t ueintx = UEINTX)
        {
            return (ueintx & _BV(RXSTPI)) != 0;
        }

        static void AcknowledgeSetupReceived()
        {
            ATL_USB_DEBUG_PRINT("[%02X-%02X] ~RXSTPI\n", UEINTX, UEBCLX);
            UEINTX &= ~_BV(RXSTPI);
        }

        static bool IsInReady(uint8_t ueintx = UEINTX)
        {
            return (ueintx & _BV(TXINI)) != 0;
        }

        static void SendIn()
        {
            ATL_USB_DEBUG_PRINT("[%02X-%02X] ~TXINI/FIFOCON\n", UEINTX, UEBCLX);
            UEINTX &= ~(_BV(FIFOCON) | _BV(NAKINI) | _BV(TXINI));
        }

        static void SendControlIn()
        {
            ATL_USB_DEBUG_PRINT("[%02X-%02X] ~TXINI\n", UEINTX, UEBCLX);
            UEINTX &= ~(_BV(NAKINI) | _BV(TXINI));
        }

        static bool IsOutReceived(uint8_t ueintx = UEINTX)
        {
            return (ueintx & _BV(RXOUTI)) != 0;
        }

        static void AcknowledgeOutReceived()
        {
            ATL_USB_DEBUG_PRINT("[%02X-%02X] ~RXOUTI/FIFOCON\n", UEINTX, UEBCLX);
            UEINTX &= ~(_BV(FIFOCON) | _BV(NAKOUTI) | _BV(RXOUTI));
        }

        static void AcknowledgeControlOutReceived()
        {
            ATL_USB_DEBUG_PRINT("[%02X-%02X] ~RXOUTI\n", UEINTX, UEBCLX);
            UEINTX &= ~(_BV(NAKOUTI) | _BV(RXOUTI));
        }

        static bool IsNakInReceived(uint8_t ueintx = UEINTX)
        {
            return (ueintx & _BV(NAKINI)) != 0;
        }

        static void AcknowledgeNakInReceived()
        {
            UEINTX &= ~_BV(NAKINI);
        }

        static bool IsNakOutReceived(uint8_t ueintx = UEINTX)
        {
            return (ueintx & _BV(NAKOUTI)) != 0;
        }

        static void AcknowledgeNakOutReceived()
        {
            UEINTX &= ~_BV(NAKOUTI);
        }

        static bool IsReadWriteAllowed(uint8_t ueintx = UEINTX)
        {
            return (ueintx & _BV(RWAL)) != 0;
        }
    };
}
