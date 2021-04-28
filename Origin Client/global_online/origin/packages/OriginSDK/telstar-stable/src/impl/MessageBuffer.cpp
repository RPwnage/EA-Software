#include "StdAfx.h"
#include "MessageBuffer.h"
#include "OriginSDKMemory.h"


MessageBuffer::MessageBuffer(size_t length, size_t minSpaceThreshold) :
	m_Length(length),
	m_ReadOffset(0),
	m_WriteOffset(0),
	m_MessageEndOffset((size_t)-1),
    m_MinSpaceThreshold(minSpaceThreshold != 0 ? minSpaceThreshold : length/16)
{
	m_Buffer = TYPE_NEW(char, length);
}


MessageBuffer::~MessageBuffer(void)
{
	TYPE_DELETE(m_Buffer);
}

size_t MessageBuffer::AvailableSpace() const
{
	// This is a resetting buffer. (not a ring buffer)
	//		----------------------------------------------------------------------
	//		|			   |#############0####################|                   |
	//		---------------^-------------^--------------------^-------------------^
	//				m_ReadOffset  m_MessageEndOffset	m_WriteOffset		   m_Length

	return m_Length - m_WriteOffset;
}

char * MessageBuffer::WritePointer()
{
    if(AvailableSpace() + m_ReadOffset < m_MinSpaceThreshold)
    {
        char *pNewBuffer = TYPE_NEW(char, m_Length*2);
        memcpy(pNewBuffer, m_Buffer, m_Length);
        m_Length *= 2;
        TYPE_DELETE(m_Buffer);
        m_Buffer = pNewBuffer;
    }

	if(AvailableSpace() + m_ReadOffset)
	{
		if(m_WriteOffset >= m_ReadOffset && m_ReadOffset > 0 )
		{
            if(m_WriteOffset - m_ReadOffset > 0)
            {
			    // Move the data to the start of the buffer.
			    memmove(m_Buffer, m_Buffer + m_ReadOffset, m_WriteOffset - m_ReadOffset);
            }

			// Adjust the pointers back
			m_WriteOffset -= m_ReadOffset;
			m_MessageEndOffset = m_MessageEndOffset != -1 ? m_MessageEndOffset - m_ReadOffset : -1;
			m_ReadOffset = 0;
		}

		return m_Buffer + m_WriteOffset;
	}

	// Buffer is full
	return NULL;
}		

bool MessageBuffer::Write(size_t bytes)
{
	m_WriteOffset += bytes;
	return FindMessageMarker();
}

char * MessageBuffer::ReadPointer() const
{
	return m_ReadOffset != m_WriteOffset && m_MessageEndOffset != -1 ? m_Buffer + m_ReadOffset : NULL;
}

size_t MessageBuffer::ReadSize() const
{
	return m_MessageEndOffset + 1 - m_ReadOffset;
}

bool MessageBuffer::Read(size_t bytes)
{
	m_ReadOffset += bytes;

	m_MessageEndOffset = (size_t)-1;

	return FindMessageMarker();
}

bool MessageBuffer::FindMessageMarker()
{
	if(m_MessageEndOffset == -1)
	{
		for(size_t i=m_ReadOffset; i<m_WriteOffset; i++)
		{
			if(m_Buffer[i] == '\0')
			{
				m_MessageEndOffset = i;
				return true;
			}
		}
	}
	return false;
}

