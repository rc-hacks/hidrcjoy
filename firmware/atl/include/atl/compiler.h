//
// compiler.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#define ATL_UNUSED(x) ((void)(x))

#ifdef __GNUC__
#define ATL_ATTRIBUTE_PACKED __attribute__((packed))
#define ATL_ATTRIBUTE_ALWAYS_INLINE __attribute__((always_inline))
#define ATL_ATTRIBUTE_OPTIMIZE_O2 __attribute__((optimize(2)))
#else
#error Unsupported compiler
#endif
