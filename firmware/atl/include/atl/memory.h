//
// memory.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <stdint.h>

namespace atl
{
    enum class MemoryType
    {
        Ram,
        Progmem,
        Eeprom,
    };

    class RamTraits
    {
    public:
        static uint8_t ReadUInt8(const uint8_t* buffer)
        {
            return *buffer;
        }

        static uint16_t ReadUInt16(const uint16_t* buffer)
        {
            return *buffer;
        }

        static uint32_t ReadUInt32(const uint32_t* buffer)
        {
            return *buffer;
        }

        static float ReadFloat(const float* buffer)
        {
            return *buffer;
        }

        static void WriteUInt8(uint8_t* buffer, uint8_t value)
        {
            *buffer = value;
        }

        static void WriteUInt16(uint16_t* buffer, uint16_t value)
        {
            *buffer = value;
        }

        static void WriteUInt32(uint32_t* buffer, uint32_t value)
        {
            *buffer = value;
        }

        static void WriteFloat(float* buffer, float value)
        {
            *buffer = value;
        }
    };

    class ProgmemTraits
    {
    public:
        static uint8_t ReadUInt8(const uint8_t* buffer)
        {
            return pgm_read_byte(buffer);
        }

        static uint16_t ReadUInt16(const uint16_t* buffer)
        {
            return pgm_read_word(buffer);
        }

        static uint32_t ReadUInt32(const uint32_t* buffer)
        {
            return pgm_read_dword(buffer);
        }

        static float ReadFloat(const float* buffer)
        {
            return pgm_read_float(buffer);
        }
    };

    class EepromTraits
    {
    public:
        static uint8_t ReadUInt8(const uint8_t* buffer)
        {
            return eeprom_read_byte(buffer);
        }

        static uint16_t ReadUInt16(const uint16_t* buffer)
        {
            return eeprom_read_word(buffer);
        }

        static uint32_t ReadUInt32(const uint32_t* buffer)
        {
            return eeprom_read_dword(buffer);
        }

        static float ReadFloat(const float* buffer)
        {
            return eeprom_read_float(buffer);
        }

        static void WriteUInt8(uint8_t* buffer, uint8_t value)
        {
            eeprom_write_byte(buffer, value);
        }

        static void WriteUInt16(uint16_t* buffer, uint16_t value)
        {
            eeprom_write_word(buffer, value);
        }

        static void WriteUInt32(uint32_t* buffer, uint32_t value)
        {
            eeprom_write_dword(buffer, value);
        }

        static void WriteFloat(float* buffer, float value)
        {
            eeprom_write_float(buffer, value);
        }
    };

    class Memory
    {
    public:
        static uint8_t ReadUInt8(const uint8_t* buffer, MemoryType memoryType)
        {
            switch (memoryType)
            {
            case MemoryType::Ram:
                return RamTraits::ReadUInt8(buffer);
            case MemoryType::Progmem:
                return ProgmemTraits::ReadUInt8(buffer);
            case MemoryType::Eeprom:
                return EepromTraits::ReadUInt8(buffer);
            default:
                return *buffer;
            }
        }

        static uint16_t ReadUInt16(const uint16_t* buffer, MemoryType memoryType)
        {
            switch (memoryType)
            {
            case MemoryType::Ram:
                return RamTraits::ReadUInt16(buffer);
            case MemoryType::Progmem:
                return ProgmemTraits::ReadUInt16(buffer);
            case MemoryType::Eeprom:
                return EepromTraits::ReadUInt16(buffer);
            default:
                return *buffer;
            }
        }

        static uint32_t ReadUInt32(const uint32_t* buffer, MemoryType memoryType)
        {
            switch (memoryType)
            {
            case MemoryType::Ram:
                return RamTraits::ReadUInt32(buffer);
            case MemoryType::Progmem:
                return ProgmemTraits::ReadUInt32(buffer);
            case MemoryType::Eeprom:
                return EepromTraits::ReadUInt32(buffer);
            default:
                return *buffer;
            }
        }

        static float ReadFloat(const float* buffer, MemoryType memoryType)
        {
            switch (memoryType)
            {
            case MemoryType::Ram:
                return RamTraits::ReadFloat(buffer);
            case MemoryType::Progmem:
                return ProgmemTraits::ReadFloat(buffer);
            case MemoryType::Eeprom:
                return EepromTraits::ReadFloat(buffer);
            default:
                return *buffer;
            }
        }
    };

    class MemoryBuffer
    {
    public:
        MemoryBuffer(const void* data, size_t size, MemoryType memoryType) : m_data(data), m_size(size), m_memoryType(memoryType)
        {
        }

        const void* GetData() const { return m_data; }
        size_t GetSize() const { return m_size; }
        MemoryType GetMemoryType() const { return m_memoryType; }

    private:
        const void* m_data;
        size_t m_size;
        MemoryType m_memoryType;
    };
}
