#
# makefile
# Copyright (C) 2018 Marius Greuel. All rights reserved.
#

BOARD ?= BOARD_ARDUINO_MICRO
#BOARD ?= BOARD_SPARKFUN_PROMICRO
#BOARD ?= BOARD_FABISP
#BOARD ?= BOARD_DIGISTUMP_DIGISPARK
#BOARD ?= BOARD_DIGISTUMP_DIGISPARKPRO

AVRDUDE_FLAGS = -c avrisp2 -B 1

include atl/boards.mk

TARGET = hidrcjoy
SOURCES = hidrcjoy.cpp
CPPFLAGS += -D$(BOARD) -D$(USB) -Iatl/include

include atl/avr8.mk
