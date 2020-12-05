//
// HidRcJoy.h
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#pragma once
#include "HidDevice.h"
#include "../firmware/Configuration.h"
#include "../firmware/UsbReports.h"

/////////////////////////////////////////////////////////////////////////////

class HidRcJoyDevice : public HidDevice
{
public:
    Configuration* GetConfiguration() { return &m_configuration; }

    void ReadReport(UsbReport& report)
    {
        auto buffer = Read();
        std::memcpy(&report, buffer.data(), sizeof(report));
    }

    void ReadEnhancedReport(UsbEnhancedReport& report)
    {
        auto buffer = GetFeatureReport(UsbEnhancedReportId);
        std::memcpy(&report, buffer.data(), sizeof(report));
    }

    void ReadConfiguration()
    {
        auto buffer = GetFeatureReport(ConfigurationReportId);
        std::memcpy(&m_configuration, buffer.data(), sizeof(m_configuration));
    }

    void WriteConfiguration()
    {
        Buffer<uint8_t> buffer(reinterpret_cast<const uint8_t*>(&m_configuration), sizeof(m_configuration));
        SetFeatureReport(ConfigurationReportId, buffer);
    }

    void LoadDefaultConfiguration()
    {
        SetFeatureReport(LoadConfigurationDefaultsId, Buffer<uint8_t>());
    }

    void ReadConfigurationFromEeprom()
    {
        SetFeatureReport(ReadConfigurationFromEepromId, Buffer<uint8_t>());
    }

    void WriteConfigurationToEeprom()
    {
        SetFeatureReport(WriteConfigurationToEepromId, Buffer<uint8_t>());
    }

    void JumpToBootloader()
    {
        SetFeatureReport(JumpToBootloaderId, Buffer<uint8_t>());
    }

private:
    Configuration m_configuration{};
};

//---------------------------------------------------------------------------

class HidRcJoy
{
public:
    HRESULT EnumerateDevices()
    {
        m_devices.clear();

        GUID hidGuid = {};
        HidD_GetHidGuid(&hidGuid);

        HDEVINFO hDeviceInfoSet = SetupDiGetClassDevs(&hidGuid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
        if (hDeviceInfoSet == nullptr)
            return AtlHresultFromLastError();

        HRESULT hr = S_OK;

        DWORD index = 0;
        while (true)
        {
            SP_DEVICE_INTERFACE_DATA deviceInterfaceData = { sizeof(SP_DEVICE_INTERFACE_DATA) };
            if (!SetupDiEnumDeviceInterfaces(hDeviceInfoSet, nullptr, &hidGuid, index, &deviceInterfaceData))
            {
                DWORD dwError = GetLastError();
                if (dwError != ERROR_NO_MORE_ITEMS)
                {
                    hr = AtlHresultFromWin32(dwError);
                }

                break;
            }

            DWORD dwRequiredSize = 0;
            if (!SetupDiGetDeviceInterfaceDetail(hDeviceInfoSet, &deviceInterfaceData, nullptr, 0, &dwRequiredSize, nullptr))
            {
                DWORD dwError = GetLastError();
                if (dwError != ERROR_INSUFFICIENT_BUFFER)
                {
                    hr = AtlHresultFromWin32(dwError);
                    break;
                }
            }

            std::unique_ptr<uint8_t[]> buffer(new uint8_t[dwRequiredSize]);

            SP_DEVICE_INTERFACE_DETAIL_DATA* pDeviceInterfaceDetailData = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA*>(buffer.get());
            std::memset(pDeviceInterfaceDetailData, 0, dwRequiredSize);
            pDeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

            if (SetupDiGetDeviceInterfaceDetail(hDeviceInfoSet, &deviceInterfaceData, pDeviceInterfaceDetailData, dwRequiredSize, &dwRequiredSize, nullptr))
            {
                std::unique_ptr<HidRcJoyDevice> device = std::make_unique<HidRcJoyDevice>();
                hr = device->Open(pDeviceInterfaceDetailData->DevicePath);
                if (SUCCEEDED(hr))
                {
                    m_devices.push_back(std::move(device));
                }
            }

            index++;
        }

        SetupDiDestroyDeviceInfoList(hDeviceInfoSet);

        return S_OK;
    }

    const std::vector<std::unique_ptr<HidRcJoyDevice>>& GetDevices() const { return m_devices; }

private:
    std::vector<std::unique_ptr<HidRcJoyDevice>> m_devices;
};
