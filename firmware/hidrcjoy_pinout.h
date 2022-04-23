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
#define PPM_ICP_DDR DDRF
#define PPM_ICP_PORT PORTF
#define PPM_ICP_PIN PINF
#define PPM_ICP_BIT 7
#else
// D4/PD4/ICP1
#define PPM_ICP_DDR DDRD
#define PPM_ICP_PORT PORTD
#define PPM_ICP_PIN PIND
#define PPM_ICP_BIT 4
#endif
#endif

#if HIDRCJOY_PCINT
// D14/PB3/PCINT3/MISO
#define PPM_PCINT_DDR DDRB
#define PPM_PCINT_PORT PORTB
#define PPM_PCINT_PIN PINB
#define PPM_PCINT_BIT 3
#endif

#define DEBUG_DDR   DDRB
#define DEBUG_PORT  PORTB
#define DEBUG_PIN   PINB
#define DEBUG_D9    5
#define DEBUG_D10   6
#define DEBUG_D11   7
