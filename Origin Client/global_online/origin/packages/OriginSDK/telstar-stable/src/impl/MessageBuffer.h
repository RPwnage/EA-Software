#pragma once

/// \brief implements a circular buffer 
/// The user needs to ensure that only one thread has access to the buffer at any time.

#include <stddef.h>

class MessageBuffer
{
public:
	MessageBuffer(size_t length, size_t minSpaceThreshold = 0);
	~MessageBuffer(void);

public:
	/// \brief returns the available space for writing in the circular buffer
	size_t AvailableSpace() const;

	/// \brief returns a pointer to a continuous block of memory of size AvailableSize()
	char * WritePointer();

	/// \brief adjusts the write Offset to add the block written to the write pointer. 
	bool Write(size_t bytes);

	/// \brief returns the location to read from.
	char * ReadPointer() const;

	/// \brief returns the amount of data to be read from the circular buffer. Messages are separated by a '\0' character.
	size_t ReadSize() const;

	/// \brief adjusts the read index to indicate a block of memory is available for writing.
	bool Read(size_t bytes);

	/// \brief check whether the end of 
	bool HasMessage() const {return m_MessageEndOffset != -1;}

	/// \brief search the data for the first occurence of '\0'
	bool FindMessageMarker();

private:
	char * m_Buffer;
	size_t m_Length;
	size_t m_ReadOffset;
	size_t m_WriteOffset;
	size_t m_MessageEndOffset;
    size_t m_MinSpaceThreshold;
};

