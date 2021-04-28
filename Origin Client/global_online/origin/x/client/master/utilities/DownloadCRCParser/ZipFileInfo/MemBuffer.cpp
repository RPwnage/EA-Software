/**
Memory buffer implementation
*/

#include "MemBuffer.h"
#include <assert.h>
#include <memory.h>
//#include <alg.h>

MemBuffer::MemBuffer(uint32_t len) : mData(0), 
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

MemBuffer::MemBuffer(uint8_t* data, uint32_t len, bool own) : mData(0), 
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

bool MemBuffer::Create(uint32_t size)
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
		mData = new uint8_t[size];

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

void MemBuffer::Assign(uint8_t* data, uint32_t len, bool own)
{
	//Self assignments, we could also check for pointers 
	//in the range of our own allocation but thats just too much
	assert(!data || data != mData);

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

void MemBuffer::DeepCopy(const uint8_t* data, uint32_t len)
{
	Destroy();
	Create(len);

	if ( len > 0 && data != NULL )
	{
        memcpy( GetBufferPtr(), data, MIN(len, TotalSize()) );
	}
}

bool MemBuffer::Resize(uint32_t size)
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
                        memcpy( GetBufferPtr(), tmpBuffer.GetReadPtr(), MIN(tmpBuffer.Remaining(), TotalSize()) );
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

uint8_t* MemBuffer::Release()
{
	uint8_t* result = mData;
	
	if ( mData )
	{
		mData = NULL;
		mSize = mPos = 0;
		mOwn = false;
	}

	return result;
}

uint8_t* MemBuffer::GetBufferPtr()
{
	return mData;
}

const uint8_t* MemBuffer::GetBufferPtr() const
{
	return mData;
}

uint8_t* MemBuffer::GetReadPtr()
{
	return Remaining() > 0 ? &mData[mPos] : NULL;
}

const uint8_t* MemBuffer::GetReadPtr() const
{
	return Remaining() > 0 ? &mData[mPos] : NULL;
}

bool MemBuffer::Seek( long pos, u_int origin )
{
	bool result = false;

	if ( !IsEmpty() )
	{
                int64_t absolutePos = -1;

		if ( origin == SEEK_CUR )
		{
                        absolutePos = int64_t(mPos) + pos;
		}
		else if ( origin == SEEK_END )
		{
                        absolutePos = int64_t(mSize) + pos;
		}
		else if ( origin == SEEK_SET )
		{
			absolutePos = pos;
		}

		if ( absolutePos >= 0 && absolutePos <= mSize )
		{
			mPos = (uint32_t) absolutePos;
			result = true;
		}
	}

	return result;
}

uint32_t MemBuffer::Tell() const
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

uint32_t MemBuffer::TotalSize() const
{
	return mSize;
}

uint32_t MemBuffer::Remaining() const
{
	//mSize should always be greater or equal than mPos
	return mData ? mSize - mPos : 0;
}

bool MemBuffer::Read( void* dst, uint32_t size )
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

QString MemBuffer::ReadString( int len, u_int codepage )
{
	QString dst;
	
	bool seekZero = len < 0;

	if ( seekZero )
	{
		//Search null terminator
		uint8_t* data = GetReadPtr();
		uint32_t rem = Remaining();
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
		wchar_t* wideBuffer = new wchar_t[len+1];
		const char* str = (const char*) GetReadPtr();

		//Quirk: If we use -1 for the input string length then MultiByteToWideChar() will also process the terminating null character
		//and include the null terminator in the output string, but will also return the length INCLUDING the null terminator
		int nStringLength = MultiByteToWideChar(codepage, 0, str, len, wideBuffer, len + 1);

		if ( nStringLength >= 0 )
		{
			wideBuffer[nStringLength] = L'\0';
			dst = QString::fromWCharArray(wideBuffer, nStringLength);

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
	}

	return dst;
}

void MemBuffer::AsQString(QString& s)
{
	if ((mData == NULL) || (mSize <= 0))
	{
		s.clear();
		return;
	}

	u_short* pBOM = reinterpret_cast<u_short*>(mData);
	if (mSize >= 2 && (*pBOM == 0xFFFE || *pBOM == 0xFEFF))
	{
		s = QString::fromUtf16((const ushort*)mData, mSize / sizeof(wchar_t));
	}
	else
	{
		s = QString::fromUtf8((const char*)mData, mSize);
	}
}

bool MemBuffer::SetAt(uint32_t pos, uint8_t value)
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
	assert(bo == BigEndian || bo == LittleEndian);
	mByteOrder = bo;
}

uint8_t MemBuffer::getUChar()
{
	uint8_t res = 0;

	if ( Remaining() >= 1 )
	{
		uint8_t* data = GetReadPtr();
		res = *data;
		mPos++;
	}

	return res;
}

u_short MemBuffer::getUShort()
{
	u_short res = 0;

	if ( Remaining() >= 2 )
	{
		uint8_t* data = GetReadPtr();

		if ( IsLittleEndian() )
		{
			res = u_short(data[0]) + (u_short(data[1]) << 8);
		}
		else
		{
			res = u_short(data[1]) + (u_short(data[0]) << 8);
		}
		
		mPos += 2;
	}

	return res;
}

uint32_t MemBuffer::getULong()
{
	uint32_t res = 0;

	if ( Remaining() >= 4 )
	{
		uint8_t* data = GetReadPtr();

		if ( IsLittleEndian() )
		{
			res = uint32_t(data[0]) + (uint32_t(data[1]) << 8) + (uint32_t(data[2]) << 16) + (uint32_t(data[3]) << 24);
		}
		else
		{
			res = uint32_t(data[3]) + (uint32_t(data[2]) << 8) + (uint32_t(data[1]) << 16) + (uint32_t(data[0]) << 24);
		}

		mPos += 4;
	}

	return res;
}

//	40-bit integers are used in the BIG5 file
int64_t MemBuffer::getInt40()
{
	__int64 res = 0;

	if ( Remaining() >= 5 )
	{
		uint8_t* data = GetReadPtr();

		if ( IsLittleEndian() )
		{
                        res = int64_t(data[0]) + (int64_t(data[1]) << 8) +
                                        (int64_t(data[2]) << 16) + (int64_t(data[3]) << 24) +
                                        (int64_t(data[4]) << 32);
		}
		else
		{
                        res = int64_t(data[4]) + (int64_t(data[3]) << 8) +
                                        (int64_t(data[2]) << 16) + (int64_t(data[1]) << 24) +
                                        (int64_t(data[0]) << 32);
		}

		mPos += 5;
	}

	return res;
}

int64_t MemBuffer::getInt64()
{
	__int64 res = 0;

	if ( Remaining() >= 8 )
	{
		uint8_t* data = GetReadPtr();

		if ( IsLittleEndian() )
		{
                        res = int64_t(data[0]) + (int64_t(data[1]) << 8) +
                                        (int64_t(data[2]) << 16) + (int64_t(data[3]) << 24) +
                                        (int64_t(data[4]) << 32) + (int64_t(data[5]) << 40) +
                                        (int64_t(data[6]) << 48) + (int64_t(data[7]) << 56);
		}
		else
		{
                        res = int64_t(data[7]) + (int64_t(data[6]) << 8) +
                                        (int64_t(data[5]) << 16) + (int64_t(data[4]) << 24) +
                                        (int64_t(data[3]) << 32) + (int64_t(data[2]) << 40) +
                                        (int64_t(data[1]) << 48) + (int64_t(data[0]) << 56);
		}

		mPos += 8;
	}

	return res;
}

