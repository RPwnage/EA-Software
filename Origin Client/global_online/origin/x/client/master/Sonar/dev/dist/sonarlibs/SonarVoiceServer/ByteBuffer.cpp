#include "ByteBuffer.h"
#include <assert.h>
#include <string.h>

namespace sonar
{

ByteBuffer::ByteBuffer (size_t _cbMaxSize)
{
	m_pBuffer = new char[_cbMaxSize];
	m_cbMaxLength = _cbMaxSize;
	m_iOffset = 0;
}

ByteBuffer::~ByteBuffer (void)
{
	delete m_pBuffer;
}

void ByteBuffer::flush (void)
{
	m_iOffset = 0;
}

size_t ByteBuffer::popUntilChar (void *_pOutData, size_t _cbMaxLength, int _ch)
{
	assert (_cbMaxLength > 0);
	bool found = false;

	size_t index = 0;
	int cv;

	if (m_iOffset == 0)
	{
		return 0;
	}

	while (index < _cbMaxLength && index < m_iOffset)
	{
		cv = (int) (unsigned char) m_pBuffer[index];

		index ++;
		if (cv == _ch)
		{
			found = true;
			break;
		}
	}

	if (!found)
	{
		return 0;
	}

	return pop (_pOutData, index);
}

size_t ByteBuffer::pop (void *_pOutData, size_t _cbMaxLength)
{
	assert (_cbMaxLength <= m_iOffset);
	assert (_cbMaxLength > 0);

	if (_pOutData) memcpy(_pOutData, m_pBuffer, _cbMaxLength);

	memmove(m_pBuffer, m_pBuffer + _cbMaxLength, (m_iOffset - _cbMaxLength));
	m_iOffset -= _cbMaxLength;

	return _cbMaxLength;
}

size_t ByteBuffer::pop (size_t cbMaxLength)
{
	return pop(NULL, cbMaxLength);
}


size_t ByteBuffer::push (const void *_pData, size_t _cbLength)
{
	assert (_cbLength > 0);
	assert (m_iOffset + _cbLength <= m_cbMaxLength);

	if (_pData)
	{
		memcpy (m_pBuffer + m_iOffset, _pData, _cbLength);
	}

	m_iOffset += _cbLength;

	return 0;
}

size_t ByteBuffer::push (size_t cbLength)
{
	return push(NULL, cbLength);
}


char *ByteBuffer::getPushPtr()
{
	return m_pBuffer + m_iOffset;
}
	
char *ByteBuffer::getEndPtr()
{
	return m_pBuffer + m_cbMaxLength;
}

char *ByteBuffer::getPopPtr(void)
{
	return m_pBuffer;
}



}