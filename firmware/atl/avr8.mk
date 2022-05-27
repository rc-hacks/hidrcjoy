#
# avr8.mk - A simple makefile for AVR8 microcontroller
# Copyright (C) 2018 Marius Greuel
# SPDX-License-Identifier: GPL-3.0-or-later
#

ifndef MCU
    $(error error : variable MCU is not defined)
endif

ifndef F_CPU
    $(error error : variable F_CPU is not defined)
endif

ifndef SOURCES
    $(error error : variable SOURCES is not defined)
endif

# Default tools from the AVR 8-bit toolchain
CC = avr-gcc
CXX = avr-g++
AS = avr-as
AR = avr-ar
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size
NM = avr-nm
AVRDUDE = avrdude

# C/C++ compiler flags
C_AND_CXX_FLAGS += -mmcu=$(MCU)
C_AND_CXX_FLAGS += -MMD -MP
C_AND_CXX_FLAGS += $(OPTIMIZATION_FLAGS)
C_AND_CXX_FLAGS += -Wall -Wextra
C_AND_CXX_FLAGS += -fpack-struct -fshort-enums
C_AND_CXX_FLAGS += -ffunction-sections -fdata-sections
C_AND_CXX_FLAGS += -g

# Preprocessor flags
CPPFLAGS += -DF_CPU=$(F_CPU)

# C compiler flags
CFLAGS += -std=c11
CFLAGS += $(C_AND_CXX_FLAGS)
CFLAGS += -Wstrict-prototypes

# C++ compiler flags
CXXFLAGS += -std=c++11
CXXFLAGS += $(C_AND_CXX_FLAGS)
CXXFLAGS += -fno-exceptions -fno-threadsafe-statics

# Assembler flags
ASFLAGS += $(C_AND_CXX_FLAGS)
ASFLAGS += -x assembler-with-cpp

# Linker flags
LDFLAGS += $(C_AND_CXX_FLAGS)
LDFLAGS += -Wl,-Map=$(MAPFILE) -Wl,--relax -Wl,--gc-sections
LDLIBS += -lm

# Make flags
MAKEFLAGS += -r

ifdef DEBUG
    OPTIMIZATION_FLAGS ?= -Og
else
    CPPFLAGS += -DNDEBUG
    OPTIMIZATION_FLAGS ?= -Os
endif

OBJDIR ?= objs
TARGET ?= main
ELFFILE ?= $(OBJDIR)/$(TARGET).elf
BINFILE ?= $(OBJDIR)/$(TARGET).bin
HEXFILE ?= $(OBJDIR)/$(TARGET).hex
MAPFILE ?= $(OBJDIR)/$(TARGET).map
LSTFILE ?= $(OBJDIR)/$(TARGET).lst

VPATH += $(dir $(SOURCES))
OBJECTS += $(addprefix $(OBJDIR)/,$(addsuffix .o, $(basename $(notdir $(SOURCES)))))
DEPENDENCIES += $(OBJECTS:.o=.d)

# Select the command-line tools used in this makefile.
# The enviroment variable 'ComSpec' implies cmd.exe on Windows
ifdef ComSpec
    SHELL = cmd.exe
    RM = del
    MKDIR = mkdir
    RMDIR = rmdir /s /q
    NULL = 2>nul
    ospath = $(subst /,\,$1)
else
    RM = rm -f
    MKDIR = mkdir -p
    RMDIR = rm -r -f
    NULL =
    ospath = $1
endif

all: hex list size done

elf: $(ELFFILE)
bin: $(BINFILE)
hex: $(HEXFILE)
list: $(LSTFILE)

size: $(ELFFILE)
	$(info Project size:)
	$(SIZE) $(SIZEFLAGS) $(ELFFILE)

done: $(HEXFILE)
	$(info Done: $(call ospath,$(HEXFILE)))

flash: $(HEXFILE) size
	$(AVRDUDE) $(AVRDUDE_FLAGS) -U flash:w:$(HEXFILE):i

fuse:
	$(AVRDUDE) $(AVRDUDE_FLAGS) -U lfuse:w:$(FUSE_L):m -U hfuse:w:$(FUSE_H):m -U efuse:w:$(FUSE_E):m

clean:
	$(info Cleaning $(TARGET)...)
	-$(RM) $(call ospath,$(ELFFILE)) $(call ospath,$(HEXFILE)) $(call ospath,$(LSTFILE)) $(NULL)
	-$(RMDIR) $(call ospath,$(OBJDIR)) $(NULL)

.PHONY: elf bin hex list size done flash fuse clean

$(BINFILE): $(ELFFILE)
	$(OBJCOPY) -j .text -j .data -O binary $< $@

$(HEXFILE): $(ELFFILE)
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

$(LSTFILE): $(ELFFILE)
	$(OBJDUMP) -x -S $< >$@

$(ELFFILE): $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(OBJECTS): | $(OBJDIR)

$(OBJDIR):
	-$(MKDIR) $(call ospath,$(OBJDIR))

$(OBJDIR)/%.o: %.c
	$(info $(call ospath,$<))
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: %.cc
	$(info $(call ospath,$<))
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: %.cxx
	$(info $(call ospath,$<))
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: %.cpp
	$(info $(call ospath,$<))
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: %.S
	$(info $(call ospath,$<))
	$(CC) $(ASFLAGS) $(CPPFLAGS) -c -o $@ $<

.SUFFIXES:

-include $(DEPENDENCIES)
