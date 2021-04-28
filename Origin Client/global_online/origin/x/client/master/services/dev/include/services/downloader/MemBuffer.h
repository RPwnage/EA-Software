// Memory buffer

#ifndef _MEMBUFFER_H_
#define _MEMBUFFER_H_

#include <stdio.h>
#include <QString>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Downloader
{

/**
\brief Used to ease the use of memory buffer allocation and deallocation.

Also provides convenient methods to wrap an existing buffer and read 
from it safely (by avoiding reaching past the buffer size).
*/
class ORIGIN_PLUGIN_API MemBuffer
{
public:
    /// \brief Code page enum.
	enum CodePage
	{
		kCodePageANSI = 0,      ///< ANSI code page.
		kCodePageOEM = 1,       ///< OEM code page.
		kCodePageUTF8 = 65001   ///< UTF8 code page. Values from WinNls.h, used for familiarity only.
	};

	/**
	\brief Constructs a memory buffer.

	\param len If len is 0, no memory is allocated.
	*/
	MemBuffer(quint32 len = 0);

	/**
	\brief Constructs a memory buffer from an existing buffer.

	Takes the buffer length and if own is true takes ownership (deletes on destruction).
	The buffer will be deleted with delete [];

    \param data TBD.
    \param len TBD.
    \param own TBD.
	*/
	MemBuffer(quint8* data, quint32 len, bool own = false);
	
	/**
	\brief Destructor will free the reserved memory if the object is owner.
	*/
	~MemBuffer();

	/**
	\brief Creates a new buffer with the given size.

    \param size TBD.
	*/
	bool Create(quint32 size);

	/**
	\brief Assigns a new buffer and optionally takes ownership of it.

    \param data TBD.
    \param len TBD.
    \param own TBD.
	*/
	void Assign(quint8* data, quint32 len, bool own);

	/**
	\brief Transfers the contents of one buffer to another.

	\param src Looses its contents.
	*/
	void Assign(MemBuffer& src);

	/**
	\brief Does a deep copy.

    \param data TBD.
    \param len TBD.
	*/
	void DeepCopy(const quint8* data, quint32 len);

	/**
	\brief Resizes the buffer to the new size (expand or shrink).
     
	The remaining contents are preserved as long as they fit in the new size.
	If they don't fit, it copies as much as it can.

	NOTE: The copy is done from the current read position, not the start of the buffer.

	Do a Rewind() before calling this if you need to preserve the entire buffer.
    \param size TBD.
	*/
	bool Resize(quint32 size);

	/**
	\brief Destroys the current content, releases memory.
	*/
	void Destroy();

	/**
	Release the internal buffer. The object loses the buffer and goes to an empty state.
	It is the callers responsibility to free the buffer when done.
	*/
	quint8* Release();

	/**
	\brief Accesses the buffer at the beginning.
	*/
	quint8* GetBufferPtr();
	const quint8* GetBufferPtr() const;
	
	/**
	\brief Accesses the buffer at the current read location.
	*/
	quint8* GetReadPtr();
	const quint8* GetReadPtr() const;

	/**
	\brief Repositions the read cursor.

	Returns final position from the beginning of the buffer.
    \param pos TBD.
    \param origin TBD.
	*/
	bool Seek( qint32 pos, quint32 origin = SEEK_SET );

	/**
	\brief Gets the current position from the beginning of the buffer.
	Returns 0 if empty
	*/
	quint32 Tell() const;

	/** 
	\brief Goes back to the beginning.
	*/
	bool Rewind();
	
	/** 
	\brief Tests if we reached the end of the buffer.
	*/
	bool Eof() const;

	/** 
	\brief Tests for an empty buffer (not created).
	*/
	bool IsEmpty() const;

	/**
	\brief Gets the buffer size.
	*/
	quint32 TotalSize() const;

	/**
	\brief Gets the available bytes to be read.
	*/
	quint32 Remaining() const;
	
	/**
	\brief Reads the required amount of data and places it in 'dst'

	If there is insufficient data to be read then nothing is done.
    \param dst TBD.
    \param size TBD.
	*/
	bool Read( void* dst, quint32 size );

	/**
	\brief Reads a string.
    
    If len is -1 a zero terminated string is assumed.
	Returns an empty string in case Remaining() < len or 
	len == -1 but a null terminator cannot be found 
	Default value for codepage is CP_ACP, which is 0.
    \param len TBD.
    \param codepage TBD.
	*/
	QString ReadString( int len = - 1, CodePage codepage = kCodePageANSI );

	/**
	\brief Sets a byte value.

    \param pos TBD.
    \param value TBD.
	*/
	bool SetAt(quint32 pos, quint8 value);

    
	/// \brief Byte order enum.
	enum ByteOrder { 
		BigEndian,                      //!< The byte order is bigendian.
		LittleEndian,                   //!< The byte order is littleendian.
		DefaultByteOrder = BigEndian    //!< The default order is bigendian.
	};

	/**
	\brief Gets the byte order configured for this stream. See #ByteOrder.
	*/
	ByteOrder GetByteOrder() const;
	bool IsLittleEndian() const;
	bool IsBigEndian() const;

	/**
	\brief Sets the byte order for this stream.

    \param bo The byte order, which is one of #ByteOrder.
	*/
	void SetByteOrder( ByteOrder bo );

	/**
	Commodities, all numbers are treated as little-endian.
	There is no result reserved for error; the caller must check Remaining() size.
	*/
	quint8 getUChar();
	quint16 getUShort();
	quint32 getULong();
	qint64 getInt64();

    bool IsOwner() { return mOwn; }

private:
	/**
	This is not reference counted so we better avoid confusing operations.
	If ever required implement a Clone() method
	So disable copy constructor and assignment operator
	*/
	MemBuffer( const MemBuffer& ) {};

	MemBuffer& operator=(const MemBuffer&) { return *this; };

private:
	quint8* mData;
	quint32 mSize;
	quint32 mPos;
	bool mOwn;
	ByteOrder mByteOrder;
};

} // namespace Downloader
} // namespace Origin

#endif
