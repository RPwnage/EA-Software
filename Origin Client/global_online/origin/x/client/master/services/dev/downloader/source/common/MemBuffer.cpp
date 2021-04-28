// Memory buffer implementation

//#include "services/downloader/stdafx.h"
#include "services/debug/DebugService.h"
#include "MemBuffer.h"
#include <memory.h>
#include <QString>

//#define ORIGIN_ASSERT(expr) Q_ASSERT(expr)

namespace Origin
{
namespace Downloader
{

MemBuffer::MemBuffer(quint32 len) : mData(0),
								   mSize(0),
								   mPos(0),
								   mOwn(false),
								   mByteOrder(DefaultByteOrder)
{
	if ( len )
	{
		Create(len);
	}
}

MemBuffer::MemBuffer(quint8* data, quint32 len, bool own) : mData(0),
														   mSize(0),
														   mPos(0),
														   mOwn(false),
														   mByteOrder(DefaultByteOrder)
{
	if ( data && len )
	{
		mData = data;
		mSize = len;
		mOwn = own;
	}
}

MemBuffer::~MemBuffer()
{
	Destroy();
}

bool MemBuffer::Create(quint32 size)
{
	bool result = true;

	//Optim
	if ( mOwn && size == mSize )
	{
		mPos = 0;
		return true;
	}

	Destroy();

	if ( size )
	{
		mData = new quint8[size];

		if ( mData )
		{
			mSize = size;
			mPos = 0;
			mOwn = true;
		}
		else
			result = false;
	}

	return result;
}

void MemBuffer::Assign(quint8* data, quint32 len, bool own)
{
	//Self assignments, we could also check for pointers
	//in the range of our own allocation but thats just too much
	ORIGIN_ASSERT(!data || data != mData);

	Destroy();

	if ( data && len > 0 )
	{
		mData = data;
		mSize = len;
		mPos = 0;
		mOwn = own;
	}
}

void MemBuffer::Assign(MemBuffer& src)
{
	Assign(src.GetBufferPtr(), src.TotalSize(), src.mOwn);
	src.Release();
}

void MemBuffer::DeepCopy(const quint8* data, quint32 len)
{
	Destroy();
	Create(len);

	if ( len > 0 && data != NULL )
	{
        memcpy( GetBufferPtr(), data, qMin(len, TotalSize()) );
	}
}

bool MemBuffer::Resize(quint32 size)
{
	bool result = true;

	//Check no-op
	if ( size == mSize )
		return true;

	MemBuffer tmpBuffer;

	if ( Remaining() )
	{
		//Use temporary buffer to store current data
		tmpBuffer.Assign(mData, mSize, mOwn);
		tmpBuffer.Seek(mPos, SEEK_SET);
		Release();
	}
	else
	{
		Destroy();
	}

	if ( size )
	{
		result = Create(size);

		if ( result && (tmpBuffer.Remaining() > 0) )
		{
			//Copy contents
			memcpy( GetBufferPtr(), tmpBuffer.GetReadPtr(), qMin(tmpBuffer.Remaining(), TotalSize()) );
		}
	}

	return result;
}

void MemBuffer::Destroy()
{
	if ( mOwn && mData )
	{
		delete [] mData;
	}

	mData = NULL;
	mSize = mPos = 0;
	mOwn = false;
}

quint8* MemBuffer::Release()
{
	quint8* result = mData;

	if ( mData )
	{
		mData = NULL;
		mSize = mPos = 0;
		mOwn = false;
	}

	return result;
}

quint8* MemBuffer::GetBufferPtr()
{
	return mData;
}

const quint8* MemBuffer::GetBufferPtr() const
{
	return mData;
}

quint8* MemBuffer::GetReadPtr()
{
	return Remaining() > 0 ? &mData[mPos] : NULL;
}

const quint8* MemBuffer::GetReadPtr() const
{
	return Remaining() > 0 ? &mData[mPos] : NULL;
}

bool MemBuffer::Seek( qint32 pos, quint32 origin )
{
	bool result = false;

	if ( !IsEmpty() )
	{
		qint64 absolutePos = -1;

		if ( origin == SEEK_CUR )
		{
			absolutePos = qint64(mPos) + pos;
		}
		else if ( origin == SEEK_END )
		{
			absolutePos = qint64(mSize) + pos;
		}
		else if ( origin == SEEK_SET )
		{
			absolutePos = pos;
		}

		if ( absolutePos >= 0 && absolutePos <= mSize )
		{
			mPos = (quint32) absolutePos;
			result = true;
		}
	}

	return result;
}

quint32 MemBuffer::Tell() const
{
	return mPos;
}

bool MemBuffer::Rewind()
{
	mPos = 0;
	return mData != NULL;
}

bool MemBuffer::Eof() const
{
	return Remaining() == 0;
}

bool MemBuffer::IsEmpty() const
{
	return mData == NULL || mSize == 0;
}

quint32 MemBuffer::TotalSize() const
{
	return mSize;
}

quint32 MemBuffer::Remaining() const
{
	//mSize should always be greater or equal than mPos
	return mData ? mSize - mPos : 0;
}

bool MemBuffer::Read( void* dst, quint32 size )
{
	bool result = false;

	if ( Remaining() >= size )
	{
		if ( size )
		{
			memcpy(dst, GetReadPtr(), size);
			mPos += size;
		}

		result = true;
	}

	return result;
}

QString MemBuffer::ReadString( int len, CodePage codepage )
{
	QString dst;

	bool seekZero = len < 0;

	if ( seekZero )
	{
		//Search null terminator
		quint8* data = GetReadPtr();
		quint32 rem = Remaining();
		int count = 0;

		while ( rem-- )
		{
			if ( *data++ )
				count++;
			else
			{
				len = count;
				break;
			}

			//We have to limit this somehow...
			if ( count > 32 * 1024 )
				break;
		}
	}

	if ( len > 0 && len <= (int) Remaining() )
	{
		const char* str = (const char*) GetReadPtr();
		
		// codepage #def's: CP_ACP == 0, CP_OEMCP == 1, CP_UTF8 == 65001
		if ( codepage == kCodePageANSI || codepage == kCodePageOEM )
			dst = QString::fromLocal8Bit(str, len);
		else
			dst = QString::fromUtf8(str, len);

		//Advance pointer, skip null terminator if required
		if ( seekZero )
			Seek(len + 1, SEEK_CUR);
		else
			Seek(len, SEEK_CUR);
	}
	else
	{
		dst.clear();
	}
	return dst;
}

bool MemBuffer::SetAt(quint32 pos, quint8 value)
{
	if ( pos < mSize )
	{
		mData[pos] = value;
		return true;
	}

	return false;
}

MemBuffer::ByteOrder MemBuffer::GetByteOrder() const
{
	return mByteOrder;
}

bool MemBuffer::IsLittleEndian() const
{
	return mByteOrder == LittleEndian;
}

bool MemBuffer::IsBigEndian() const
{
	return mByteOrder == BigEndian;
}

void MemBuffer::SetByteOrder( ByteOrder bo )
{
	ORIGIN_ASSERT(bo == BigEndian || bo == LittleEndian);
	mByteOrder = bo;
}

quint8 MemBuffer::getUChar()
{
	quint8 res = 0;

	if ( Remaining() >= 1 )
	{
		quint8* data = GetReadPtr();
		res = *data;
		mPos++;
	}

	return res;
}

quint16 MemBuffer::getUShort()
{
	quint16 res = 0;

	if ( Remaining() >= 2 )
	{
		quint8* data = GetReadPtr();

		if ( IsLittleEndian() )
		{
			res = quint16(data[0]) + (quint16(data[1]) << 8);
		}
		else
		{
			res = quint16(data[1]) + (quint16(data[0]) << 8);
		}

		mPos += 2;
	}

	return res;
}

quint32 MemBuffer::getULong()
{
	quint32 res = 0;

	if ( Remaining() >= 4 )
	{
		quint8* data = GetReadPtr();

		if ( IsLittleEndian() )
		{
			res = quint32(data[0]) + (quint32(data[1]) << 8) + (quint32(data[2]) << 16) + (quint32(data[3]) << 24);
		}
		else
		{
			res = quint32(data[3]) + (quint32(data[2]) << 8) + (quint32(data[1]) << 16) + (quint32(data[0]) << 24);
		}

		mPos += 4;
	}

	return res;
}

qint64 MemBuffer::getInt64()
{
	qint64 res = 0;

	if ( Remaining() >= 8 )
	{
		quint8* data = GetReadPtr();

		if ( IsLittleEndian() )
		{
			res = qint64(data[0]) + (qint64(data[1]) << 8) +
					(qint64(data[2]) << 16) + (qint64(data[3]) << 24) +
					(qint64(data[4]) << 32) + (qint64(data[5]) << 40) +
					(qint64(data[6]) << 48) + (qint64(data[7]) << 56);
		}
		else
		{
			res = qint64(data[7]) + (qint64(data[6]) << 8) +
					(qint64(data[5]) << 16) + (qint64(data[4]) << 24) +
					(qint64(data[3]) << 32) + (qint64(data[2]) << 40) +
					(qint64(data[1]) << 48) + (qint64(data[0]) << 56);
		}

		mPos += 8;
	}

	return res;
}

} // namespace Downloader
} // namespace Origin

