/**
Memory buffer
*/

#ifndef _MEMBUFFER_H_
#define _MEMBUFFER_H_

#include <stdio.h>
//#include "platform.h"
#include <stdint.h>

#include "WinDefs.h"

#include <QString>

/**
Memory buffer class.
Used to ease the use of a memory buffer allocation and deallocation
Also provides convenient methods to wrap an existing buffer and read 
from it safely (avoid reaching past the buffer size).
*/
class MemBuffer
{
public:
	/**
	Construct memory buffer.
	If len is 0 no memory is allocated
	*/
	MemBuffer(uint32_t len = 0);

	/**
	Construct a memory buffer from an existing buffer
	Takes the buffer length and if own is true take ownership (delete on destruction)
	The buffer will be deleted with delete [];
	*/
	MemBuffer(uint8_t* data, uint32_t len, bool own = false);
	
	/**
	Destructor will free the reserved memory if the object is owner
	*/
	~MemBuffer();

	/**
	Create a new buffer with the given size
	*/
	bool Create(uint32_t size);

	/**
	Assign a new buffer and optionally take ownership
	*/
	void Assign(uint8_t* data, uint32_t len, bool own);

	/**
	Transfer the contents of one buffer to another
	src looses his contents.
	*/
	void Assign(MemBuffer& src);

	/**
	Do a deep copy
	*/
	void DeepCopy(const uint8_t* data, uint32_t len);

	/**
	Resize the buffer to the new size (expand or shrink). 
	Remaining contents are preserved as long as they fit in the new size.
	If they don't fit it copies as much as it can.
	NOTE: The copy is done from the current read position, not the start of the buffer
		  Do a Rewind() before calling this if you need to preserve the entire buffer.
	*/
	bool Resize(uint32_t size);

	/**
	Destroy current content, release memory
	*/
	void Destroy();

	/**
	Release the internal buffer. The object loses the buffer and goes to an empty state.
	It is the callers responsibility to free the buffer when done.
	*/
	uint8_t* Release();

	/**
	Access the buffer at beginning
	*/
	uint8_t* GetBufferPtr();
	const uint8_t* GetBufferPtr() const;
	
	/**
	Access the buffer at the current read location
	*/
	uint8_t* GetReadPtr();
	const uint8_t* GetReadPtr() const;

	/**
	Reposition read cursor.
	Returns final position from the beginning of the buffer
	*/
	bool Seek( long pos, uint32_t origin = SEEK_SET );

	/**
	Get the current position from the beginning of the buffer
	Returns 0 if empty
	*/
	uint32_t Tell() const;

	/** 
	Go back to the beginning 
	*/
	bool Rewind();
	
	/** 
	Test if we reached the end of the buffer 
	*/
	bool Eof() const;

	/** 
	Test for empty buffer (not created)
	*/
	bool IsEmpty() const;

	/**
	Get the buffer size
	*/
	uint32_t TotalSize() const;

	/**
	Get the available bytes to be red
	*/
	uint32_t Remaining() const;
	
	/**
	Read the required amount of data and place it in 'dst'
	If there is insufficient data to be red then nothing is done
	*/
	bool Read( void* dst, uint32_t size );

	/**
	Read a string. If len is -1 a zero terminated string is assumed.
	Returns an empty string in case Remaining() < len or 
	len == -1 but a null terminator cannot be found 
	*/
	QString ReadString( int len = - 1, uint32_t codepage = CP_ACP );

	/**
	Converts the buffer into a null terminated UTF-16 QString
	*/
	void AsQString(QString& s);

	/**
	Set a byte value
	*/
	bool SetAt(uint32_t pos, uint8_t value);


	enum ByteOrder { 
		BigEndian, 
		LittleEndian,
		DefaultByteOrder = BigEndian
	};

	/**
	Get the byte order configured for this stream
	*/
	ByteOrder GetByteOrder() const;
	bool IsLittleEndian() const;
	bool IsBigEndian() const;

	/**
	Set the byte order for this stream
	*/
	void SetByteOrder( ByteOrder bo );

	/**
	Commodities, all numbers are treated as little-endian
	There is no result reserved for error, caller must check Remaining() size
	*/
	uint8_t getUChar();
	uint16_t getUShort();
	uint32_t getULong();
        int64_t getInt40();
        int64_t getInt64();

private:
	/**
	This is not reference counted so we better avoid confusing operations.
	If ever required implement a Clone() method
	So disable copy constructor and assignment operator
	*/
	MemBuffer( const MemBuffer& ) {};

	MemBuffer& operator=(const MemBuffer&) { return *this; };

private:
	uint8_t* mData;
	uint32_t mSize;
	uint32_t mPos;
	bool mOwn;
	ByteOrder mByteOrder;
};

#endif
