#pragma once
#include <stdlib.h>
#include <algorithm>
#include <stdint.h>
#include <assert.h>

class Buffer
{
public:
	static const int initialSize;
	explicit Buffer();
	~Buffer();

	int Readable_Bytes() const
	{
		return m_writeIndex - m_readIndex;
	}

	int Writable_Bytes() const // all - writted
	{
		return m_bufferSize - m_writeIndex;
	}

	int Prependable_Bytes() const
	{
		return m_readIndex;
	}

	char* Peek()
	{
		return begin() + m_readIndex;
	}

	const char* Peek() const
	{
		return begin() + m_readIndex;
	}

	char* Begin_Write()
	{
		return begin() + m_writeIndex;
	}

	const char* Begin_Write() const
	{
		return begin() + m_writeIndex;
	}

	const char* Find_CRLF() const
	{
		const char* crlf = std::search(Peek(), Begin_Write(), m_kCRLF, m_kCRLF + 2);
		return crlf == Begin_Write() ? NULL : crlf; //return second false
	}

	const char* Find_CRLF(const char* start) const
	{
		assert(Peek() <= start);
		assert(start <= Begin_Write());
		const char* crlf = std::search(start, Begin_Write(), m_kCRLF, m_kCRLF + 2);
		return crlf == Begin_Write() ? nullptr : crlf;
	}

	const char* Find_LastCRLF() const
	{
		const char* crlf = std::find_end(Peek(), Begin_Write(), m_kCRLF, m_kCRLF + 2);
		return crlf == Begin_Write() ? nullptr : crlf;
	}

	void Retrieve(int len)
	{
		assert(len <= Readable_Bytes());
		if (len < Readable_Bytes())
			m_readIndex += len;
		else
			Retrieve_All();
	}

	void Retrieve_Until(const char* end)
	{
		assert(Peek() <= end); //read - end - beginW
		assert(end <= Begin_Write());
		Retrieve(end - Peek());
	}

	void Retrieve_All()//恢复索引
	{
		m_readIndex = 0;
		m_writeIndex = 0;
	}

	void Unwrite(int len)
	{
		assert(len <= Readable_Bytes());
		m_writeIndex -= len;
	}

	void Ensure_WritableBytes(int len)
	{
		if (Writable_Bytes() < len)
			Make_Space(len);
		assert(Writable_Bytes() >= len);
	}

	void Make_Space(int len)
	{
		if (Writable_Bytes() + Prependable_Bytes() < len)
		{
			m_bufferSize = m_writeIndex + len;
			m_buffer = (char*)realloc(m_buffer, m_bufferSize);
		}
		else
		{
			int readable = Readable_Bytes();
			std::copy(begin() + m_readIndex, begin() + m_writeIndex, begin());
			m_readIndex = 0;
			m_writeIndex = m_readIndex + readable;
			assert(readable == Readable_Bytes());
		}
	}

	void Append(const char* data, int len)
	{
		Ensure_WritableBytes(len);
		std::copy(data, data + len, Begin_Write());
		assert(len <= Writable_Bytes());
		m_writeIndex += len;
	}

	void Append(const void* data, int len)
	{
		Append((const char*)(data), len);
	}

	int Read(int fd);
	int Write(int fd);

private:
	char* begin()
	{
		return m_buffer;
	}

	const char* begin() const
	{
		return m_buffer;
	}
private:
	char* m_buffer;
	int m_bufferSize;
	int m_readIndex;
	int m_writeIndex; //从socket实际读取到的字节长度
	static const char* m_kCRLF;
};