#pragma once
#include <algorithm>
#include <assert.h>
#include <memory>

class Buffer
{
public:
	static const int initialsize;
	explicit Buffer();
	~Buffer();

	int readable() const { return m_writeindex - m_readindex; }
	int writeable() const { return m_buffersize - m_writeindex; }
	int prependable() const { return m_readindex; }
	const char* peek() const { return begin() + m_readindex; }
	char* peek() { return begin() + m_readindex; }
	const char* beginwrite() const { return begin() + m_writeindex; }
	char* beginwrite() { return begin() + m_writeindex; }
	void retrieveAll() { m_readindex = m_writeindex = 0; }

	const char* findCRLF() const;
	const char* findCRLF(const char* start) const;
	const char* findLastCRLF() const;

	void retrieve(int len);
	void retrieveUntil(const char* end);

	void unwrite(int len);
	void makeSpace(int len);
	void ensurewriteable(int len);

	void append(const char* data, int len);
	void append(const void* data, int len);

	int read(int fd);
	int write(int fd);

private:
	const char* begin() const { return m_buffer; }
	char* begin() { return m_buffer; }

private:
	char* m_buffer;
	int m_buffersize;
	int m_readindex; 
	int m_writeindex;
	static const char* m_kCRLF;
};