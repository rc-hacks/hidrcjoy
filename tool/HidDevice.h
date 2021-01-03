//
// HidDevice.h
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#pragma once
#include "Buffer.h"

/////////////////////////////////////////////////////////////////////////////

class HidDevice
{
    static const uint16_t m_vendorID = 0x16C0;
    static const uint16_t m_productID = 0x03E8;

public:
    ~HidDevice()
    {
        Close();
    }

    HRESULT Open(LPCTSTR pszDevicePath)
    {
        DWORD dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
        DWORD dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
        HANDLE hHidDevice = CreateFile(pszDevicePath, dwDesiredAccess, dwShareMode, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hHidDevice == INVALID_HANDLE_VALUE)
            return AtlHresultFromLastError();

        m_hDevice = hHidDevice;

        HRESULT hr = GetProperties();
        if (FAILED(hr))
            return hr;

        hr = CheckDeviceMatch();
        if (FAILED(hr))
            return hr;

        HidD_SetNumInputBuffers(m_hDevice, 2);

        return S_OK;
    }

    void Close()
    {
        if (m_preparsedData != nullptr)
        {
            HidD_FreePreparsedData(m_preparsedData);
            m_preparsedData = nullptr;
        }

        if (m_hDevice != nullptr)
        {
            CloseHandle(m_hDevice);
            m_hDevice = nullptr;
        }
    }

    std::wstring GetProduct() const { return m_product; }
    std::wstring GetManufacturer() const { return m_manufacturer; }

    Buffer<uint8_t> Read()
    {
        Buffer<uint8_t> buffer(m_caps.InputReportByteLength);

        DWORD dwBytesRead = 0;
        if (!ReadFile(m_hDevice, buffer.data(), static_cast<DWORD>(buffer.size()), &dwBytesRead, nullptr))
            throw std::runtime_error("ReadFile failed");

        return buffer;
    }

    Buffer<uint8_t> GetFeatureReport(uint8_t index)
    {
        Buffer<uint8_t> buffer(m_caps.FeatureReportByteLength);

        buffer[0] = index;
        if (!HidD_GetFeature(m_hDevice, buffer.data(), m_caps.FeatureReportByteLength))
            throw std::runtime_error("HidD_GetFeature failed");

        return buffer;
    }

    void SetFeatureReport(uint8_t index, const Buffer<uint8_t>& buffer)
    {
        if (buffer.size() > m_caps.FeatureReportByteLength)
            throw std::runtime_error("Buffer too big");

        Buffer<uint8_t> report(m_caps.FeatureReportByteLength);

        std::memcpy(report.data(), buffer.data(), buffer.size());
        std::memset(report.data() + buffer.size(), 0, m_caps.FeatureReportByteLength - buffer.size());
        report[0] = index;

        if (!HidD_SetFeature(m_hDevice, report.data(), m_caps.FeatureReportByteLength))
            throw std::runtime_error("HidD_SetFeature failed");
    }

private:
    HRESULT GetProperties()
    {
        if (!HidD_GetPreparsedData(m_hDevice, &m_preparsedData))
            return AtlHresultFromLastError();

        if (!HidP_GetCaps(m_preparsedData, &m_caps))
            return AtlHresultFromLastError();

        WCHAR product[128] = {};
        if (!HidD_GetProductString(m_hDevice, product, sizeof(product)))
            return AtlHresultFromLastError();

        m_product = product;

        WCHAR manufacturer[128] = {};
        if (!HidD_GetManufacturerString(m_hDevice, manufacturer, sizeof(manufacturer)))
            return AtlHresultFromLastError();

        m_manufacturer = manufacturer;

        WCHAR serialNumber[128] = {};
        if (HidD_GetSerialNumberString(m_hDevice, serialNumber, sizeof(serialNumber)))
        {
            m_serialNumber = serialNumber;
        }

        return S_OK;
    }

    HRESULT CheckDeviceMatch()
    {
        HIDD_ATTRIBUTES attributes = {};
        if (!HidD_GetAttributes(m_hDevice, &attributes))
            return AtlHresultFromLastError();

        //if (attributes.VendorID != m_vendorID || attributes.ProductID != m_productID)
        //    return AtlHresultFromWin32(ERROR_INVALID_DATA);

        if (m_caps.UsagePage != HID_USAGE_PAGE_GENERIC || m_caps.Usage != HID_USAGE_GENERIC_JOYSTICK)
            return AtlHresultFromWin32(ERROR_INVALID_DATA);

        return S_OK;
    }

protected:
    HANDLE m_hDevice = nullptr;
    PHIDP_PREPARSED_DATA m_preparsedData = nullptr;
    HIDP_CAPS m_caps = {};
    std::wstring m_product;
    std::wstring m_manufacturer;
    std::wstring m_serialNumber;
};
