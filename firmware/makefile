#
# Makefile
# Copyright (C) 2018 Marius Greuel
# SPDX-License-Identifier: GPL-3.0-or-later
#

MCU = atmega32u4
F_CPU = 16000000

TARGET = hidrcjoy
SOURCES = hidrcjoy.cpp

CPPFLAGS += -DBOARD_ARDUINO_LEONARDO=1
CPPFLAGS += -Iatl/include

AVRDUDE_FLAGS ?= -c avr109 -p $(MCU) -P usb:2341:0036 -D 

include atl/avr8.mk
