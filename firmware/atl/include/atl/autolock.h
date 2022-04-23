//
// autolock.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <avr/interrupt.h>
#include <stdint.h>

namespace atl
{
    class AutoLock
    {
    public:
        AutoLock() : m_oldSREG(SREG)
        {
            cli();
        }

        ~AutoLock()
        {
            SREG = m_oldSREG;
        }

    private:
        uint8_t m_oldSREG;
    };
}
