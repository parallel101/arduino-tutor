#include "BufferStream.h"
#include <algorithm>
#include <cstring>

BufferStream::BufferStream(uint8_t *buffer, size_t capacity)
    : m_buffer(buffer), m_write_ptr(buffer), m_end_ptr(buffer + capacity)
{
}

size_t BufferStream::write(const uint8_t *buffer, size_t size)
{
    size_t written = std::min<size_t>(size, m_end_ptr - m_write_ptr);
    memcpy(m_write_ptr, buffer, written);
    m_write_ptr += written;
    return written;
}

size_t BufferStream::write(uint8_t data)
{
    return write(&data, 1);
}

int BufferStream::available()
{
    return 0;
}

int BufferStream::read()
{
    return -1;
}

int BufferStream::peek()
{
    return -1;
}

void BufferStream::flush()
{
}
