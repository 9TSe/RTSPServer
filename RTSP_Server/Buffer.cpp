#include "Buffer.h"
#include "SocketsOps.h"

const int Buffer::initialSize = 1024;
const char* Buffer::m_kCRLF = "\r\n";

Buffer::Buffer()
	:m_bufferSize(initialSize),
	m_readIndex(0),
	m_writeIndex(0)
{
	m_buffer = (char*)malloc(m_bufferSize);
}

Buffer::~Buffer()
{
	free(m_buffer);
}

int Buffer::Read(int fd)
{
	char extra_buf[65536];
	const int writable = Writable_Bytes();
	const int n = ::recv(fd, extra_buf, sizeof(extra_buf), 0);
	if (n <= 0)
		return -1;
	else if (n <= writable)
	{
		std::copy(extra_buf, extra_buf + n, Begin_Write());
		m_writeIndex += n;
	}
	else
	{
		std::copy(extra_buf, extra_buf + writable, Begin_Write());
		m_writeIndex += writable;
		Append(extra_buf + writable, n - writable);
	}
	return n;
}

int Buffer::Write(int fd)
{
	return sockets::write(fd, Peek(), Readable_Bytes());
}