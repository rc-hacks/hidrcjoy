//
// hidrcjoy_pinout.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

// D13/PC7
#define LED_DDR DDRC
#define LED_PORT PORTC
#define LED_PIN PINC
#define LED_BIT 7

#if HIDRCJOY_ICP
#if HIDRCJOY_ICP_ACIC_A0
// D18/A0/PF7
#define CAPTURE_DDR DDRF
#define CAPTURE_PORT PORTF
#define CAPTURE_PIN PINF
#define CAPTURE_BIT 7
#else
// D4/PD4/ICP1
#define CAPTURE_DDR DDRD
#define CAPTURE_PORT PORTD
#define CAPTURE_PIN PIND
#define CAPTURE_BIT 4
#endif
#endif

#if HIDRCJOY_PCINT
// D14/PB3/PCINT3/MISO
#define PCINT_DDR DDRB
#define PCINT_PORT PORTB
#define PCINT_PIN PINB
#define PCINT_BIT 3
#endif
