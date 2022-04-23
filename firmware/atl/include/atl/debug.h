//
// debug.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <atl/std_streams.h>

// By default, ATL_DEBUG_PRINT uses printf() to print text.
// For details on how to setup printf() support, see 'atl/std_streams.h'.

#ifndef ATL_DEBUG_PRINT
#if ATL_DEBUG
#define ATL_DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define ATL_DEBUG_PRINT(...) ((void)0)
#endif
#endif
