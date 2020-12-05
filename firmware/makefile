#
# makefile
# Copyright (C) 2018 Marius Greuel. All rights reserved.
#

BOARD ?= BOARD_ARDUINO_MICRO
#BOARD ?= BOARD_SPARKFUN_PROMICRO
#BOARD ?= BOARD_FABISP
#BOARD ?= BOARD_DIGISTUMP_DIGISPARK
#BOARD ?= BOARD_DIGISTUMP_DIGISPARKPRO

USB = USB_ATL
xUSB = USB_LUFA
AVRDUDE_FLAGS = -c avrisp2 -B 1

include atl/boards.mk

TARGET = hidrcjoy
SOURCES = hidrcjoy.cpp Descriptors.c
CPPFLAGS += -D$(BOARD) -D$(USB) -Iatl/include

ifeq ($(USB),USB_LUFA)
    ARCH = AVR8
    F_USB = $(F_CPU)

ifndef LUFA_PATH
    $(error error : environment variable LUFA_PATH is not defined. LUFA_PATH must point to the LUFA folder 'lufa-LUFA-170418')
endif

    DMBS_LUFA_PATH ?= $(LUFA_PATH)/Build/LUFA
    include $(DMBS_LUFA_PATH)/lufa-sources.mk
    include $(DMBS_LUFA_PATH)/lufa-gcc.mk

    SOURCES += $(LUFA_SRC_USB)
    CPPFLAGS += $(LUFA_CXX_INCLUDES) $(LUFA_CXX_DEFINES) $(LUFA_CXX_FLAGS)
    CPPFLAGS += -D USB_DEVICE_ONLY -D USE_STATIC_OPTIONS="(USB_DEVICE_OPT_FULLSPEED | USB_OPT_REG_ENABLED | USB_OPT_AUTO_PLL)"
    CPPFLAGS += -D USE_FLASH_DESCRIPTORS -D FIXED_CONTROL_ENDPOINT_SIZE=8 -D FIXED_NUM_CONFIGURATIONS=1
    CPPFLAGS += -D NO_INTERNAL_SERIAL -D NO_DEVICE_REMOTE_WAKEUP -D NO_DEVICE_SELF_POWER
else ifeq ($(USB),USB_V_USB)
    SOURCES += usbdrv/usbdrv.c usbdrv/usbdrvasm.S
    CPPFLAGS += -I. -DDEBUG_LEVEL=0
endif

include atl/avr8.mk
