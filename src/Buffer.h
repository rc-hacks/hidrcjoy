//
// Buffer.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <cstdint>
#include <algorithm>
#include <stdexcept>

template <typename T = uint8_t>
class Buffer
{
public:
    Buffer() = default;

    Buffer(size_t size, bool initialize = true)
    {
        Allocate(size, initialize);
    }

    Buffer(const T* data, size_t size)
    {
        Allocate(size, false);
        std::memcpy(m_data, data, size);
    }

    Buffer(const Buffer&) = delete;

    Buffer(Buffer&& buffer)
    {
        this->operator=(std::move(buffer));
    }

    Buffer& operator=(const Buffer&) = delete;

    Buffer& operator=(Buffer&& buffer)
    {
        Free();
        std::swap(m_size, buffer.m_size);
        std::swap(m_data, buffer.m_data);
        return *this;
    }

    ~Buffer()
    {
        Free();
    }

    T* GetData() const { return m_data; }
    size_t GetSize() const { return m_size; }

    T& operator[](size_t index)
    {
        if (index >= m_size)
            throw std::runtime_error("index out of range");

        return m_data[index];
    }

    T operator[](size_t index) const
    {
        if (index >= m_size)
            throw std::runtime_error("index out of range");

        return m_data[index];
    }

    void Allocate(size_t size, bool initialize = true)
    {
        Free();
        m_size = size;
        m_data = new T[m_size];

        if (initialize)
        {
            std::memset(m_data, 0, m_size);
        }
    }

    void Free()
    {
        if (m_data != nullptr)
        {
            delete[] m_data;
            m_data = nullptr;
        }
    }

private:
    T* m_data = nullptr;
    size_t m_size = 0;
};
