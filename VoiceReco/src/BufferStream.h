#pragma once

#include "Stream.h"


class BufferStream : public Stream
{
    uint8_t *m_buffer;
    uint8_t *m_write_ptr;
    uint8_t *m_end_ptr;

public:
    explicit BufferStream(uint8_t *buffer, size_t capacity);

    size_t write(const uint8_t *buffer, size_t size) override;
    size_t write(uint8_t data) override;

    int available() override;
    int read() override;
    int peek() override;
    void flush() override;

    uint8_t *data() const { return m_buffer; }
    size_t size() const { return m_write_ptr - m_buffer; }
    bool isEmpty() const { return m_write_ptr == m_buffer; }
    size_t capacity() const { return m_end_ptr - m_buffer; }
};
