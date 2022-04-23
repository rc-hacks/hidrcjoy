//
// srxl_receiver_usart1.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <avr/io.h>
#include <stdint.h>

class SrxlReceiverUsart1
{
public:
    static void Initialize(uint32_t baudrate)
    {
        // Set baudrate
        UBRR1 = ((F_CPU / 4 / baudrate) - 1) / 2;
        UCSR1A = _BV(U2X1);

        // Empty data register.
        //UDR1;

        // Enable receiver and RX IRQ.
        UCSR1B = _BV(RXEN1) | _BV(RXCIE1);
        UCSR1C = _BV(UCSZ11) | _BV(UCSZ10);
    }
};
