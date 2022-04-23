//
// stdafx.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#pragma warning(disable: 4100) // unreferenced formal parameter

//---------------------------------------------------------------------------
// Windows SDK

#define STRICT
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT _WIN32_WINNT_WIN7

#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <dbt.h>
#include <setupapi.h>
#include <hidsdi.h>

#pragma comment(lib, "comctl32")
#pragma comment(lib, "setupapi")
#pragma comment(lib, "hid")

//---------------------------------------------------------------------------
// C/C++

#include <cassert>
#include <memory>
#include <vector>
#include <string>

//---------------------------------------------------------------------------
// ATL

#define _ATL_ALL_WARNINGS
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS

#include <atlbase.h>
#include <atlwin.h>
#include <atlstr.h>
#include <atltypes.h>

//---------------------------------------------------------------------------

#if defined(_M_IX86)
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df'\"")
#elif defined(_M_X64)
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df'\"")
#else
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df'\"")
#endif
