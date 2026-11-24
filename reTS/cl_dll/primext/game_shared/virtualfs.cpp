// virtualfs.cpp - CVirtualFS, a file kept in memory.
// H9: written new for reTS from the interface and the measured behaviour of PrimeXT's class (see virtualfs.h).
#include "virtualfs.h"
#include <stdlib.h>
#include <string.h>
#include <vector>

namespace
{
	const size_t kFirstCapacity = 64 * 1024; // a new empty file starts with this much room
	const size_t kRefused = (size_t)-1;
}

CVirtualFS::CVirtualFS()
{
	m_state.data = static_cast<byte *>(malloc(kFirstCapacity));
	m_state.capacity = m_state.data ? kFirstCapacity : 0;
	m_state.pos = 0;
	m_state.length = 0;
	m_saved = m_state;
}

CVirtualFS::CVirtualFS(const byte *file, size_t size)
{
	m_state.data = nullptr;
	m_state.capacity = 0;
	m_state.pos = 0;
	m_state.length = 0;

	if (file && size > 0)
	{
		m_state.data = static_cast<byte *>(malloc(size));
		if (m_state.data)
		{
			memcpy(m_state.data, file, size);
			m_state.capacity = size;
			m_state.length = size;
		}
	}
	m_saved = m_state;
}

CVirtualFS::~CVirtualFS()
{
	free(m_state.data);
}

// Room for `needed` bytes in all. A file made from no bytes has no storage and stays read-only.
bool CVirtualFS::Reserve(size_t needed)
{
	if (!m_state.data)
		return false;
	if (needed <= m_state.capacity)
		return true;

	size_t grown = m_state.capacity ? m_state.capacity : kFirstCapacity;
	while (grown < needed)
		grown *= 2;

	byte *moved = static_cast<byte *>(realloc(m_state.data, grown));
	if (!moved)
		return false;
	m_state.data = moved;
	m_state.capacity = grown;
	return true;
}

size_t CVirtualFS::Put(const void *in, size_t size, bool insert)
{
	if (!m_state.data)
		return kRefused;
	if (size == 0)
		return m_state.length;

	const size_t tail = (m_state.pos < m_state.length) ? m_state.length - m_state.pos : 0;
	const size_t end = insert ? m_state.length + size : (m_state.pos + size > m_state.length ? m_state.pos + size : m_state.length);
	if (!Reserve(end))
		return kRefused;

	if (insert && tail)
		memmove(m_state.data + m_state.pos + size, m_state.data + m_state.pos, tail);
	memcpy(m_state.data + m_state.pos, in, size);
	m_state.pos += size;
	m_state.length = end;
	return m_state.length;
}

size_t CVirtualFS::Read(void *out, size_t size)
{
	if (!m_state.data || m_state.pos >= m_state.length)
		return 0;

	size_t left = m_state.length - m_state.pos;
	size_t count = size < left ? size : left;
	memcpy(out, m_state.data + m_state.pos, count);
	m_state.pos += count;
	return count;
}

size_t CVirtualFS::Write(const void *in, size_t size)
{
	return Put(in, size, false);
}

size_t CVirtualFS::Insert(const void *in, size_t size)
{
	return Put(in, size, true);
}

int CVirtualFS::Seek(size_t offset, int whence)
{
	size_t target;
	switch (whence)
	{
	case SEEK_SET: target = offset; break;
	case SEEK_CUR: target = m_state.pos + offset; break;
	case SEEK_END: target = m_state.length + offset; break;
	default: return -1;
	}

	if (target > m_state.length)
		return -1;
	m_state.pos = target;
	return 0;
}

int CVirtualFS::Getc()
{
	if (!m_state.data || m_state.pos >= m_state.length)
		return EOF;
	return m_state.data[m_state.pos++];
}

// Reads one line into `string` (at most size - 1 bytes kept, always terminated; the rest of a long line is
// skipped). A line ends at "\n", "\r\n" or a lone "\r" (the byte after a lone "\r" is left unread; a "\r" that
// is the last byte is itself left unread, and every later call returns EOF without moving). Returns
// what ended the line: '\n', the byte that followed a lone '\r', or EOF when the data ran out first (the
// bytes read so far are still in `string`).
int CVirtualFS::Gets(char *string, size_t size)
{
	size_t kept = 0;
	int c;

	for (;;)
	{
		c = Getc();
		if (c == EOF || c == '\n')
			break;
		if (c == '\r')
		{
			c = Getc();
			if (c != '\n')
				m_state.pos--; // the byte after it is left for the next line; at the end, the '\r' itself is
			break;
		}
		if (kept + 1 < size)
			string[kept++] = static_cast<char>(c);
	}

	if (size > 0)
		string[kept] = '\0';
	return c;
}

size_t CVirtualFS::Print(const char *text)
{
	return Write(text, strlen(text));
}

size_t CVirtualFS::IPrint(const char *text)
{
	return Insert(text, strlen(text));
}

size_t CVirtualFS::Format(bool insert, const char *fmt, va_list args)
{
	va_list measure;
	va_copy(measure, args);
	int needed = vsnprintf(nullptr, 0, fmt, measure);
	va_end(measure);
	if (needed < 0)
		return kRefused;

	std::vector<char> text(static_cast<size_t>(needed) + 1);
	vsnprintf(text.data(), text.size(), fmt, args);
	return Put(text.data(), static_cast<size_t>(needed), insert);
}

size_t CVirtualFS::VPrintf(const char *fmt, va_list args)
{
	return Format(false, fmt, args);
}

size_t CVirtualFS::IVPrintf(const char *fmt, va_list args)
{
	return Format(true, fmt, args);
}

size_t CVirtualFS::Printf(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	size_t result = Format(false, fmt, args);
	va_end(args);
	return result;
}

size_t CVirtualFS::IPrintf(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	size_t result = Format(true, fmt, args);
	va_end(args);
	return result;
}
