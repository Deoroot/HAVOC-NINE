// virtualfs.h - CVirtualFS, a file kept in memory: the renderer builds shader sources and mesh caches in one,
// and reads loaded files through one.
// H9: written new for reTS from the interface the renderer calls and the behaviour it relies on (measured on
// PrimeXT's class, whose files had no licence notice, by a black-box test; no code taken from it).
#ifndef VIRTUALFS_H
#define VIRTUALFS_H
#include <stdio.h>
#include <stdarg.h>
#include "const.h"

class CVirtualFS
{
public:
	CVirtualFS();                              // empty, ready to write
	CVirtualFS(const byte *file, size_t size); // a copy of the given bytes, at offset 0; size 0 gives a file that refuses writes
	~CVirtualFS();

	template<class T> T Read()
	{
		T value;
		Read(&value, sizeof(T));
		return value;
	}

	template<class T> size_t Write(const T &value)
	{
		return Write(&value, sizeof(T));
	}

	size_t Read(void *out, size_t size);         // bytes copied, fewer near the end
	size_t Write(const void *in, size_t size);   // overwrites from the offset and grows the file; returns the new length, (size_t)-1 if refused
	size_t Insert(const void *in, size_t size);  // as Write, but moves what follows the offset along instead of overwriting it
	int Seek(size_t offset, int whence);         // SEEK_SET, SEEK_CUR or SEEK_END (offsets wrap as unsigned); 0, or -1 past the end
	int Gets(char *string, size_t size);         // one line, see virtualfs.cpp
	int Getc();                                  // the next byte, or EOF

	char *GetBuffer() { return reinterpret_cast<char *>(m_state.data); }
	size_t GetSize() { return m_state.length; }
	size_t Tell() { return m_state.pos; }
	bool Eof() { return m_state.pos == m_state.length; }
	void SaveState() { m_saved = m_state; }      // remembers the offset; valid while nothing is written
	void RestoreState() { m_state = m_saved; }

	size_t Print(const char *text);              // Write of the text without its terminator
	size_t IPrint(const char *text);             // Insert of the text without its terminator
	size_t Printf(const char *fmt, ...);
	size_t IPrintf(const char *fmt, ...);
	size_t VPrintf(const char *fmt, va_list args);
	size_t IVPrintf(const char *fmt, va_list args);

private:
	struct Cursor
	{
		byte *data;      // the bytes (owned)
		size_t capacity; // bytes allocated
		size_t pos;      // read/write offset
		size_t length;   // bytes in the file
	};

	bool Reserve(size_t needed);
	size_t Put(const void *in, size_t size, bool insert);
	size_t Format(bool insert, const char *fmt, va_list args);

	Cursor m_state;
	Cursor m_saved;
};

#endif // VIRTUALFS_H
