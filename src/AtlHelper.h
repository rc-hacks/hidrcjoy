//
// AtlHelper.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

class CDC
{
public:
    HDC m_hdc = nullptr;

    ~CDC()
    {
        Delete();
    }

    operator HDC() const { return m_hdc; }

    void Attach(HDC hdc)
    {
        if (hdc != m_hdc)
        {
            Delete();
        }

        m_hdc = hdc;
    }

    HDC Detach()
    {
        HDC hdc = m_hdc;
        m_hdc = nullptr;
        return hdc;
    }

    void CreateCompatibleDC(HDC hdc)
    {
        Delete();
        m_hdc = ::CreateCompatibleDC(hdc);
    }

    void Delete()
    {
        if (m_hdc != nullptr)
        {
            ::DeleteDC(m_hdc);
            m_hdc = nullptr;
        }
    }

    HGDIOBJ SelectObject(HGDIOBJ hgdiobj)
    {
        return ::SelectObject(m_hdc, hgdiobj);
    }

    BOOL SetViewportOrg(int x, int y, LPPOINT lppt = nullptr)
    {
        return ::SetViewportOrgEx(m_hdc, x, y, lppt);
    }

    BOOL FillRect(LPCRECT lprc, int index)
    {
        return ::FillRect(m_hdc, lprc, (HBRUSH)(INT_PTR)(index + 1));
    }

    int SetBkMode(int mode)
    {
        return ::SetBkMode(m_hdc, mode);
    }

    COLORREF SetBkColor(COLORREF color)
    {
        return ::SetBkColor(m_hdc, color);
    }

    COLORREF SetTextColor(COLORREF color)
    {
        return ::SetTextColor(m_hdc, color);
    }

    int DrawText(LPCWSTR lpchText, int cchText, LPRECT lprc, UINT format)
    {
        return ::DrawText(m_hdc, lpchText, cchText, lprc, format);
    }

    BOOL DrawFocusRect(const RECT* lprc)
    {
        return ::DrawFocusRect(m_hdc, lprc);
    }
};

//---------------------------------------------------------------------------

class CMemoryDC : public CDC
{
public:
    HDC m_hdcPaint = nullptr;
    CRect m_rcPaint;
    HBITMAP m_hBitmap = nullptr;
    HGDIOBJ m_hOldBitmap = nullptr;

    CMemoryDC(HDC hdc, const RECT& rcPaint) : m_hdcPaint(hdc)
    {
        m_rcPaint = rcPaint;
        CreateCompatibleDC(m_hdcPaint);
        m_hBitmap = ::CreateCompatibleBitmap(m_hdcPaint, m_rcPaint.right - m_rcPaint.left, m_rcPaint.bottom - m_rcPaint.top);
        m_hOldBitmap = SelectObject(m_hBitmap);
        SetViewportOrg(-m_rcPaint.left, -m_rcPaint.top);
    }

    ~CMemoryDC()
    {
        ::BitBlt(m_hdcPaint, m_rcPaint.left, m_rcPaint.top, m_rcPaint.right - m_rcPaint.left, m_rcPaint.bottom - m_rcPaint.top, m_hdc, m_rcPaint.left, m_rcPaint.top, SRCCOPY);
        SelectObject(m_hOldBitmap);
        ::DeleteObject(m_hBitmap);
    }
};

//---------------------------------------------------------------------------

class CPaintDC : public CDC
{
public:
    HWND m_hWnd = nullptr;
    PAINTSTRUCT m_ps = {};

    CPaintDC(HWND hWnd)
    {
        m_hWnd = hWnd;
        m_hdc = ::BeginPaint(hWnd, &m_ps);
    }

    ~CPaintDC()
    {
        ::EndPaint(m_hWnd, &m_ps);
        m_hdc = nullptr;
    }
};

//---------------------------------------------------------------------------

class CButton : public CWindow
{
public:
    CButton(HWND hWnd = nullptr) : CWindow(hWnd) {}
    int GetCheck() const { return Button_GetCheck(m_hWnd); }
    void SetCheck(int check) { Button_SetCheck(m_hWnd, check); }
    int GetState() const { return Button_GetState(m_hWnd); }
    void SetState(int state) { Button_SetState(m_hWnd, state); }
};

//---------------------------------------------------------------------------

class CEdit : public CWindow
{
public:
    CEdit(HWND hWnd = nullptr) : CWindow(hWnd) {}
};

//---------------------------------------------------------------------------

class CComboBox : public CWindow
{
public:
    CComboBox(HWND hWnd = nullptr) : CWindow(hWnd) {}
    int GetCurSel() const { return ComboBox_GetCurSel(m_hWnd); }
    int SetCurSel(int index) { return ComboBox_SetCurSel(m_hWnd, index); }
    void ResetContent() { ComboBox_ResetContent(m_hWnd); }
    int AddString(LPCTSTR lpsz) { return ComboBox_AddString(m_hWnd, lpsz); }
};

//---------------------------------------------------------------------------

class CUpDownCtrl : public CWindow
{
public:
    CUpDownCtrl(HWND hWnd = nullptr) : CWindow(hWnd) {}
    void SetRange(int lower, int upper) { ::SendMessage(m_hWnd, UDM_SETRANGE, 0, MAKELPARAM(upper, lower)); }
};

//---------------------------------------------------------------------------

class CHyperlink : public CWindowImpl<CHyperlink>
{
public:
    DECLARE_WND_CLASS_EX(nullptr, CS_HREDRAW | CS_VREDRAW, COLOR_WINDOW)

    BEGIN_MSG_MAP(CHyperlink)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
    END_MSG_MAP()

    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        CPaintDC dc(m_hWnd);

        CRect rcClient;
        GetClientRect(&rcClient);
        CString strText;
        GetWindowText(strText);

        dc.FillRect(&rcClient, COLOR_BTNFACE);

        if (GetFocus() == m_hWnd)
        {
            dc.DrawFocusRect(&rcClient);
        }

        rcClient.DeflateRect(4, 1);

        HFONT hFontOld = SelectFont(dc.m_hdc, SendMessage(WM_GETFONT));
        dc.SetBkMode(TRANSPARENT);
        dc.SetTextColor(RGB(0, 0, 255));
        dc.DrawText(strText, strText.GetLength(), &rcClient, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectFont(dc.m_hdc, hFontOld);

        return 0;
    }
};
