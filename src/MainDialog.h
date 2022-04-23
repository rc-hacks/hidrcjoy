//
// MainDialog.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include "resource.h"
#include "AtlHelper.h"
#include "HidDevice.h"

class CMainDialog : public CDialogImpl<CMainDialog>
{
    class CAxisView : public CWindowImpl<CAxisView>
    {
    public:
        DECLARE_WND_CLASS_EX(nullptr, CS_HREDRAW | CS_VREDRAW, COLOR_WINDOW)

        BEGIN_MSG_MAP(CAxisView)
            MESSAGE_HANDLER(WM_PAINT, OnPaint)
        END_MSG_MAP()

        LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
        {
            CPaintDC dcPaint(m_hWnd);
            CMemoryDC dcMemory(dcPaint, dcPaint.m_ps.rcPaint);

            CRect rcClient;
            GetClientRect(&rcClient);

            dcMemory.FillRect(&rcClient, COLOR_WINDOW);

            int size = GetSystemMetrics(SM_CXSIZEFRAME);

            CPoint center = rcClient.CenterPoint();
            center.x += (rcClient.Width() / 2 - size - 1) * m_x / 128;
            center.y -= (rcClient.Height() / 2 - size - 1) * m_y / 128;

            CRect rcHorizontal(center, center);
            rcHorizontal.left -= size;
            rcHorizontal.right += size + 1;
            rcHorizontal.bottom++;
            dcMemory.FillRect(&rcHorizontal, COLOR_WINDOWTEXT);

            CRect rcVertical(center, center);
            rcVertical.top -= size;
            rcVertical.bottom += size + 1;
            rcVertical.right++;
            dcMemory.FillRect(&rcVertical, COLOR_WINDOWTEXT);

            return 0;
        }

        void SetPosition(int32_t x, int32_t y)
        {
            if (x != m_x || y != m_y)
            {
                m_x = x;
                m_y = y;
                Invalidate();
            }
        }

    private:
        int32_t m_x = 0;
        int32_t m_y = 0;
    };

    class CSliderView : public CWindowImpl<CSliderView>
    {
    public:
        DECLARE_WND_CLASS_EX(nullptr, CS_HREDRAW | CS_VREDRAW, COLOR_WINDOW)

        BEGIN_MSG_MAP(CSliderView)
            MESSAGE_HANDLER(WM_PAINT, OnPaint)
        END_MSG_MAP()

        LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
        {
            CPaintDC dcPaint(m_hWnd);
            CMemoryDC dcMemory(dcPaint, dcPaint.m_ps.rcPaint);

            CRect rcClient;
            GetClientRect(&rcClient);

            dcMemory.FillRect(&rcClient, COLOR_WINDOW);

            rcClient.DeflateRect(1, 1);
            rcClient.right = rcClient.right * (m_x + 128) / 256;
            dcMemory.FillRect(&rcClient, COLOR_BTNSHADOW);

            return 0;
        }

        void SetPosition(int32_t x)
        {
            if (x != m_x)
            {
                m_x = x;
                Invalidate();
            }
        }

    private:
        int32_t m_x = 0;
    };

public:
    enum { IDD = IDD_MAIN };

    CComboBox m_cbDevices;
    CWindow m_stDeviceStatus;
    CEdit m_ecMinSyncPulseWidth;
    CEdit m_ecCenterChannelPulseWidth;
    CEdit m_ecChannelPulseWidthRange;
    CUpDownCtrl m_udMinSyncWidth;
    CUpDownCtrl m_udCenterChannelPulseWidth;
    CUpDownCtrl m_udChannelPulseWidthRange;
    CButton m_btInvertedSignal;
    CAxisView m_stLeftStick;
    CAxisView m_stRightStick;
    CSliderView m_stSlider1;
    CSliderView m_stSlider2;
    CSliderView m_stSlider3;
    CHyperlink m_stAboutLink;

public:
    BEGIN_MSG_MAP(CMainDialog)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_DEVICECHANGE, OnDeviceChange)
        MESSAGE_HANDLER(WM_RECEIVED_REPORT, OnReceivedReport)
        MESSAGE_HANDLER(WM_RECEIVED_ENHANCED_REPORT, OnReceivedEnhancedReport)
        COMMAND_HANDLER(IDC_DEVICES, CBN_SELCHANGE, OnDevicesSelChange)
        COMMAND_HANDLER(IDC_MIN_SYNC_PULSE_WIDTH, EN_CHANGE, OnTimingChange)
        COMMAND_HANDLER(IDC_CENTER_CHANNEL_PULSE_WIDTH, EN_CHANGE, OnTimingChange)
        COMMAND_HANDLER(IDC_CHANNEL_PULSE_WIDTH_RANGE, EN_CHANGE, OnTimingChange)
        COMMAND_HANDLER(IDC_INVERTED_SIGNAL, BN_CLICKED, OnTimingChange)
        COMMAND_RANGE_HANDLER(IDC_CHANNEL1_POLARITY, IDC_CHANNEL7_POLARITY, OnChannelPolarity)
        COMMAND_RANGE_HANDLER(IDC_CHANNEL1_SOURCE1, IDC_CHANNEL7_SOURCE7, OnChannelSource)
        COMMAND_ID_HANDLER(IDC_LOAD_DEFAULT_VALUES, OnLoadDefaultValues)
        COMMAND_ID_HANDLER(IDC_READ_FROM_EEPROM, OnReadFromEeprom)
        COMMAND_ID_HANDLER(IDC_WRITE_TO_EEPROM, OnWriteToEeprom)
        COMMAND_ID_HANDLER(IDC_ABOUT_LINK, OnAboutLink)
        COMMAND_ID_HANDLER(IDCANCEL, OnClose)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        m_cbDevices.Attach(GetDlgItem(IDC_DEVICES));
        m_stDeviceStatus.Attach(GetDlgItem(IDC_DEVICE_STATUS));
        m_ecMinSyncPulseWidth.Attach(GetDlgItem(IDC_MIN_SYNC_PULSE_WIDTH));
        m_ecCenterChannelPulseWidth.Attach(GetDlgItem(IDC_CENTER_CHANNEL_PULSE_WIDTH));
        m_ecChannelPulseWidthRange.Attach(GetDlgItem(IDC_CHANNEL_PULSE_WIDTH_RANGE));
        m_udMinSyncWidth.Attach(GetDlgItem(IDC_MIN_SYNC_PULSE_WIDTH_SPIN));
        m_udCenterChannelPulseWidth.Attach(GetDlgItem(IDC_CENTER_CHANNEL_PULSE_WIDTH_SPIN));
        m_udChannelPulseWidthRange.Attach(GetDlgItem(IDC_CHANNEL_PULSE_WIDTH_RANGE_SPIN));
        m_btInvertedSignal.Attach(GetDlgItem(IDC_INVERTED_SIGNAL));
        m_stLeftStick.SubclassWindow(GetDlgItem(IDC_LEFT_STICK));
        m_stRightStick.SubclassWindow(GetDlgItem(IDC_RIGHT_STICK));
        m_stSlider1.SubclassWindow(GetDlgItem(IDC_SLIDER1));
        m_stSlider2.SubclassWindow(GetDlgItem(IDC_SLIDER2));
        m_stSlider3.SubclassWindow(GetDlgItem(IDC_SLIDER3));
        m_stAboutLink.SubclassWindow(GetDlgItem(IDC_ABOUT_LINK));

        m_udMinSyncWidth.SetRange(Configuration::minSyncWidth, Configuration::maxSyncWidth);
        m_udCenterChannelPulseWidth.SetRange(Configuration::minChannelPulseWidth, Configuration::maxChannelPulseWidth);
        m_udChannelPulseWidthRange.SetRange(Configuration::minChannelPulseWidth, Configuration::maxChannelPulseWidth);

        PopulateDeviceList();

        DWORD dwThreadID = 0;
        m_hThread = ::CreateThread(nullptr, 0, ThreadProc, this, 0, &dwThreadID);
        m_hTerminate = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);

        CenterWindow();

        return TRUE;
    }

    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        SetEvent(m_hTerminate);
        if (WaitForSingleObject(m_hThread, 5000) == WAIT_TIMEOUT)
        {
            TerminateThread(m_hThread, 0);
        }

        CloseHandle(m_hTerminate);
        CloseHandle(m_hThread);

        return 0;
    }

    LRESULT OnDeviceChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        UINT nEventType = (UINT)wParam;
        switch (nEventType)
        {
        case DBT_DEVNODES_CHANGED:
            PopulateDeviceList();
            break;
        }

        return TRUE;
    }

    LRESULT OnReceivedReport(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        UpdateReportControls(m_report);
        return FALSE;
    }

    LRESULT OnReceivedEnhancedReport(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        UpdateEnhancedReportControls(m_enhancedReport);
        return FALSE;
    }

    LRESULT OnDevicesSelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        int selection = m_cbDevices.GetCurSel();
        auto count = m_collection.GetDeviceCount();
        if (selection >= 0 && selection < count)
        {
            m_pDevice = m_collection.GetDevice(selection);
            UpdateControls();
        }

        return 0;
    }

    LRESULT OnTimingChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        if (m_pDevice && !m_lockControlUpdate)
        {
            auto pConfiguration = m_pDevice->GetConfiguration();
            pConfiguration->m_minSyncPulseWidth = static_cast<uint16_t>(GetIntegerValue(m_ecMinSyncPulseWidth));
            pConfiguration->m_centerChannelPulseWidth = static_cast<uint16_t>(GetIntegerValue(m_ecCenterChannelPulseWidth));
            pConfiguration->m_channelPulseWidthRange = static_cast<uint16_t>(GetIntegerValue(m_ecChannelPulseWidthRange));
            pConfiguration->m_flags = static_cast<uint8_t>(m_btInvertedSignal.GetCheck() == BST_CHECKED ? Configuration::InvertedSignal : 0);

            UpdateDeviceConfiguration();
        }

        return 0;
    }

    LRESULT OnChannelPolarity(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        if (m_pDevice && !m_lockControlUpdate)
        {
            int channel = wID - IDC_CHANNEL1_POLARITY;

            auto pConfiguration = m_pDevice->GetConfiguration();
            pConfiguration->m_polarity ^= 1 << channel;

            UpdateDeviceConfiguration();
            UpdatePolarityButtons(pConfiguration);
        }

        return 0;
    }

    LRESULT OnChannelSource(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        if (m_pDevice && !m_lockControlUpdate)
        {
            int channel = (wID - IDC_CHANNEL1_SOURCE1) / 10;
            int source = (wID - IDC_CHANNEL1_SOURCE1) % 10;

            auto pConfiguration = m_pDevice->GetConfiguration();
            pConfiguration->m_mapping[channel] = static_cast<uint8_t>(source);

            UpdateDeviceConfiguration();
            UpdateSourceButtons(pConfiguration);
        }

        return 0;
    }

    LRESULT OnLoadDefaultValues(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        if (m_pDevice)
        {
            try
            {
                m_pDevice->LoadDefaultConfiguration();
                UpdateControls();
            }
            catch (std::exception&)
            {
            }
        }

        return 0;
    }

    LRESULT OnReadFromEeprom(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        if (m_pDevice)
        {
            try
            {
                m_pDevice->ReadConfigurationFromEeprom();
                UpdateControls();
            }
            catch (std::exception&)
            {
            }
        }

        return 0;
    }

    LRESULT OnWriteToEeprom(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        if (m_pDevice)
        {
            try
            {
                m_pDevice->WriteConfigurationToEeprom();
            }
            catch (std::exception&)
            {
            }
        }

        return 0;
    }

    LRESULT OnAboutLink(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        CString strText;
        CWindow(hWndCtl).GetWindowText(strText);
        ::ShellExecute(0, _T("open"), strText, 0, 0, SW_SHOWNORMAL);
        return 0;
    }

    LRESULT OnClose(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        EndDialog(wID);
        return 0;
    }

private:
    static DWORD WINAPI ThreadProc(LPVOID lpParameter)
    {
        return static_cast<CMainDialog*>(lpParameter)->ThreadProc();
    }

    DWORD ThreadProc()
    {
        while (WaitForSingleObject(m_hTerminate, 10) == WAIT_TIMEOUT)
        {
            std::shared_ptr<HidDevice> pDevice = m_pDevice;
            if (pDevice)
            {
                try
                {
                    m_pDevice->ReadReport(m_report);
                    PostMessage(WM_RECEIVED_REPORT);

                    static int count = 0;
                    if (++count % 10 == 0)
                    {
                        m_pDevice->ReadEnhancedReport(m_enhancedReport);
                        PostMessage(WM_RECEIVED_ENHANCED_REPORT);
                    }
                }
                catch (std::exception&)
                {
                }
            }
        }

        return 0;
    }

    HRESULT PopulateDeviceList()
    {
        m_cbDevices.ResetContent();
        m_pDevice = nullptr;

        HRESULT hr = m_collection.EnumerateDevices();
        if (FAILED(hr))
        {
            m_stDeviceStatus.SetWindowText(FormatString(_T("Failed to enumerate devices: hr=0x08X"), hr));
            return hr;
        }

        size_t count = m_collection.GetDeviceCount();
        if (count == 0)
        {
            m_stDeviceStatus.SetWindowText(_T("No devices found"));
            m_cbDevices.AddString(_T("(None)"));
            m_cbDevices.SetCurSel(0);
            ClearControls();
            return S_FALSE;
        }
        else
        {
            m_stDeviceStatus.SetWindowText(FormatString(_T("Found %zu device(s)"), count));

            for (size_t i = 0; i < count; i++)
            {
                auto device = m_collection.GetDevice(i);
                m_cbDevices.AddString(GetFriendlyDeviceName(device.get()));
            }

            m_cbDevices.SetCurSel(0);
            m_pDevice = m_collection.GetDevice(0);

            UpdateControls();

            return S_OK;
        }
    }

    void UpdateDeviceConfiguration()
    {
        if (m_pDevice)
        {
            try
            {
                m_pDevice->WriteConfiguration();
            }
            catch (std::exception&)
            {
            }
        }
    }

    void ClearControls()
    {
        m_ecMinSyncPulseWidth.SetWindowText(_T(""));
        m_ecCenterChannelPulseWidth.SetWindowText(_T(""));
        m_ecChannelPulseWidthRange.SetWindowText(_T(""));
        m_btInvertedSignal.SetCheck(BST_UNCHECKED);

        m_stLeftStick.SetPosition(0, 0);
        m_stRightStick.SetPosition(0, 0);
        m_stSlider1.SetPosition(0);
        m_stSlider2.SetPosition(0);
        m_stSlider3.SetPosition(0);

        for (int i = 0; i < Configuration::maxOutputChannels; i++)
        {
            CEdit(GetDlgItem(IDC_CHANNEL_WIDTH1 + i)).SetWindowText(_T(""));
            CEdit(GetDlgItem(IDC_CHANNEL_VALUE1 + i)).SetWindowText(_T(""));
        }

        for (int i = 0; i < Configuration::maxOutputChannels; i++)
        {
            CButton(GetDlgItem(IDC_CHANNEL1_POLARITY + i)).SetWindowText(_T("+"));
        }

        for (int y = 0; y < Configuration::maxOutputChannels; y++)
        {
            for (int x = 0; x < Configuration::maxInputChannels; x++)
            {
                CButton(GetDlgItem(IDC_CHANNEL1_SOURCE1 + y * 10 + x)).SetState(false);
            }
        }
    }

    void UpdateControls()
    {
        if (m_pDevice)
        {
            try
            {
                m_pDevice->ReadConfiguration();

                auto pConfiguration = m_pDevice->GetConfiguration();
                UpdateSignalControls(pConfiguration);
                UpdatePolarityButtons(pConfiguration);
                UpdateSourceButtons(pConfiguration);
            }
            catch (std::exception&)
            {
            }
        }
    }

    void UpdateSignalControls(const Configuration* pConfiguration)
    {
        m_lockControlUpdate = true;

        m_ecMinSyncPulseWidth.SetWindowText(FormatString(_T("%d"), pConfiguration->m_minSyncPulseWidth));
        m_ecCenterChannelPulseWidth.SetWindowText(FormatString(_T("%d"), pConfiguration->m_centerChannelPulseWidth));
        m_ecChannelPulseWidthRange.SetWindowText(FormatString(_T("%d"), pConfiguration->m_channelPulseWidthRange));
        m_btInvertedSignal.SetCheck((pConfiguration->m_flags & Configuration::InvertedSignal) != 0 ? BST_CHECKED : BST_UNCHECKED);

        m_lockControlUpdate = false;
    }

    void UpdatePolarityButtons(const Configuration* pConfiguration)
    {
        m_lockControlUpdate = true;

        for (int i = 0; i < Configuration::maxOutputChannels; i++)
        {
            CButton(GetDlgItem(IDC_CHANNEL1_POLARITY + i)).SetWindowText((pConfiguration->m_polarity & (1 << i)) == 0 ? _T("+") : _T("-"));
        }

        m_lockControlUpdate = false;
    }

    void UpdateSourceButtons(const Configuration* pConfiguration)
    {
        m_lockControlUpdate = true;

        for (int y = 0; y < Configuration::maxOutputChannels; y++)
        {
            for (int x = 0; x < Configuration::maxInputChannels; x++)
            {
                CButton(GetDlgItem(IDC_CHANNEL1_SOURCE1 + 10 * y + x)).SetState(pConfiguration->m_mapping[y] == x);
            }
        }

        m_lockControlUpdate = false;
    }

    void UpdateSourceButtons(int nIdStart, int value)
    {
    }

    void UpdateReportControls(const UsbReport& report)
    {
        m_stLeftStick.SetPosition(ValueToPosition(report.m_value[0]), ValueToPosition(report.m_value[1]));
        m_stRightStick.SetPosition(ValueToPosition(report.m_value[2]), ValueToPosition(report.m_value[3]));
        m_stSlider1.SetPosition(ValueToPosition(report.m_value[4]));
        m_stSlider2.SetPosition(ValueToPosition(report.m_value[5]));
        m_stSlider3.SetPosition(ValueToPosition(report.m_value[6]));

        for (int i = 0; i < Configuration::maxOutputChannels; i++)
        {
            CEdit(GetDlgItem(IDC_CHANNEL_VALUE1 + i)).SetWindowText(FormatString(_T("%d"), ValueToPosition(report.m_value[i])));
        }
    }

    void UpdateEnhancedReportControls(const UsbEnhancedReport& report)
    {
        if (report.m_signalSource == SignalSource::None)
        {
            m_stDeviceStatus.SetWindowText(_T("Device found, no signal"));
        }
        else
        {
            CString strSignalSource;
            switch (report.m_signalSource)
            {
            case SignalSource::PPM:
                strSignalSource = FormatString(_T("PPM%u"), report.m_channelCount);
                break;
            case SignalSource::PCM:
                strSignalSource = FormatString(_T("PCM%u"), report.m_channelCount);
                break;
            case SignalSource::SRXL:
                strSignalSource = FormatString(_T("SRXL%u"), report.m_channelCount);
                break;
            default:
                strSignalSource = _T("Unknown");
                break;
            }

            uint32_t updateRate = report.m_updateRate > 0 ? 1000 / report.m_updateRate : 0;
            m_stDeviceStatus.SetWindowText(FormatString(_T("Receiving data using %s at %uHz"), strSignalSource.GetString(), updateRate));
        }

        for (int i = 0; i < Configuration::maxOutputChannels; i++)
        {
            CEdit(GetDlgItem(IDC_CHANNEL_WIDTH1 + i)).SetWindowText(FormatString(_T("%d"), report.m_channelPulseWidth[i]));
        }
    }

    static int32_t ValueToPosition(uint8_t value)
    {
        return value - 0x80;
    }

    CString GetFriendlyDeviceName(const HidDevice* pDevice)
    {
        return FormatString(_T("%s (%s)"), pDevice->GetProduct().c_str(), pDevice->GetManufacturer().c_str());
    }

    static CString FormatString(_In_z_ _Printf_format_string_ LPCTSTR pszFormat, ...)
    {
        CString str;

        va_list args;
        va_start(args, pszFormat);
        str.FormatV(pszFormat, args);
        va_end(args);

        return str;
    }

    static int GetIntegerValue(CEdit& control)
    {
        CString str;
        control.GetWindowText(str);
        return _tstoi(str);
    }

private:
    static const int WM_RECEIVED_REPORT = WM_USER + 1;
    static const int WM_RECEIVED_ENHANCED_REPORT = WM_USER + 2;
    HidDeviceCollection m_collection;
    std::shared_ptr<HidDevice> m_pDevice;
    UsbReport m_report = {};
    UsbEnhancedReport m_enhancedReport = {};
    HANDLE m_hThread = nullptr;
    HANDLE m_hTerminate = nullptr;
    bool m_lockControlUpdate = false;
};
