//
// Copyright 2005, Electronic Arts Inc
//
#include "services/log/LogService.h"
#include "services/debug/DebugService.h"

#include <SingleFileChunkMap.h>
#include <math.h>

#include <QFile>

namespace Origin
{
namespace Downloader
{

const QString SingleFileChunkMap::CHUNK_MAP_FILE_EXTENSION = ".map";
const int SingleFileChunkMap::MAX_UNSAVED_CHUNKS = 5;

/**
bitcount iterates over each bit. 
The while condition helps terminates the loop earlier.
*/
inline int bitcount(uint n)
{
	int count = 0;    
	while (n)
	{
		count += n & 0x1u ;    
		n >>= 1 ;
	}
	return count ;
}

/* Precomputed array that stores the number of ones in each byte. */
const int bits_in_byte[] = {
	0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
}; /* 256 bytes */

SingleFileChunkMap::SingleFileChunkMap(const QString& SingleFileChunkMapName, qint64 Filesize, int ChunkSize, const QString& logId) :
	mbInitialized(false),
	mpSingleFileChunkMap(NULL),
	mSingleFileChunkMapLen(0),
	mChunksMarked(0),
	mLogId(logId)
{
	mSingleFileChunkMapFilename = SingleFileChunkMapName;
	mCMH.Version = 1;
	mCMH.Chunksize = ChunkSize;
	mCMH.Filesize  = Filesize;
	mCMH.TotalChunks = 0;
}

SingleFileChunkMap::~SingleFileChunkMap()
{
	if(mpSingleFileChunkMap)
		delete [] mpSingleFileChunkMap;

	mbInitialized = false;
}

bool SingleFileChunkMap::Init()
{
	if ( mbInitialized ) 
		return true;

	bool result = true;
	mChunksMarked = 0;
	mChunksSinceLastSave = 0;

	// Try to resume, if chunk map file doesn't exist or is corrupt, create a new chunk map
	if (!QFile::exists(mSingleFileChunkMapFilename) || ((result = ReadSingleFileChunkMap_Header()) == false))
	{
		DetermineChunkSize();

		ORIGIN_ASSERT((1+((mCMH.Filesize-1)/mCMH.Chunksize)) < INT_MAX);
		mCMH.TotalChunks = (mCMH.Filesize / mCMH.Chunksize) + ( (mCMH.Filesize % mCMH.Chunksize) ? 1 : 0 );
		
		result = AllocChunkMap();
	}

	mbInitialized = result;
	return result;
}

void SingleFileChunkMap::ResetMarkedChunkMap()
{
	mChunksMarked = 0;
	mChunksSinceLastSave = 0;
	memset(mpSingleFileChunkMap, 0, mSingleFileChunkMapLen); 
}

bool SingleFileChunkMap::ReadSingleFileChunkMap_Header()
{
	qint64 read_bytes = 0;
	
	QFile hFile(mSingleFileChunkMapFilename);

	if(hFile.open(QIODevice::ReadOnly) && hFile.isReadable())
	{
		int oldChunkSize = mCMH.Chunksize;

		read_bytes = hFile.read( (char*)&mCMH, sizeof(chunkmap_header));

		if ( (oldChunkSize != mCMH.Chunksize) 
			|| (sizeof(chunkmap_header) != read_bytes) )
		{
			ORIGIN_LOG_WARNING << "[" << mLogId << "] Existing chunk map header is corrupt, cannot resume. Download will restart.";
			return false;
		}

		ORIGIN_ASSERT(mCMH.TotalChunks > 0);
		
		AllocChunkMap();

		// Read the existing map
		read_bytes = hFile.read((char*)mpSingleFileChunkMap, mSingleFileChunkMapLen);

		if ( mSingleFileChunkMapLen != read_bytes )
		{
			ORIGIN_LOG_WARNING << "[" << mLogId << "] Existing chunk map is corrupt, cannot resume. Download will restart.";
			return false;
		}

		// Count the total number of chunks marked
		mChunksMarked = 0;
		for ( int i = 0; i < mSingleFileChunkMapLen; i++ )
			mChunksMarked += bits_in_byte[ mpSingleFileChunkMap[i] ];

		if(GetUnmarkedSize() + GetMarkedSize() != GetFilesize())
		{
			ORIGIN_LOG_WARNING << "[" << mLogId << "] Existing chunk map is corrupt, cannot resume. Download will restart.";
			return false;
		}

		return true;
	}
	else
	{
		ORIGIN_LOG_WARNING << "[" << mLogId << "] Existing chunk map is could not be opened for reading, cannot resume. Download will restart.";
		return false;
	}
}

qint64	SingleFileChunkMap::ChunkToByte(int ChunkNo) const
{
	return qint64(ChunkNo) * GetChunkSize();
}

int	SingleFileChunkMap::ByteToChunk(qint64 ByteNo) const
{
	if ( !mbInitialized )
		return -1;
   
	ORIGIN_ASSERT(ByteNo/GetChunkSize() < INT_MAX);

	return int(ByteNo/GetChunkSize());
}

void SingleFileChunkMap::DetermineChunkSize()
{
	// The constructor has already set the size
	if (mCMH.Chunksize != 0) return;

	ORIGIN_ASSERT(mCMH.Filesize/(1024 * 8) < INT_MAX);

	// Make the chunks so that there are no more than 1k of map to write
	mCMH.Chunksize = int(mCMH.Filesize/(1024 * 8));

	// Make chunksizes on 1K boundaries
	mCMH.Chunksize = 1024 * (1 + mCMH.Chunksize/1024);
}

bool SingleFileChunkMap::AllocChunkMap()
{
	if(mpSingleFileChunkMap)
		delete [] mpSingleFileChunkMap;

	// Determine the size of the chunk map
	mSingleFileChunkMapLen = 1 + ((mCMH.TotalChunks-1)/8);

	// Allocate space
	mpSingleFileChunkMap = new uchar[mSingleFileChunkMapLen];
	memset(mpSingleFileChunkMap, 0, mSingleFileChunkMapLen); 


	//Just in case
	ORIGIN_ASSERT(sizeof(uchar) == 1);

	//Checkins
	ORIGIN_ASSERT(mCMH.TotalChunks == (mCMH.Filesize / mCMH.Chunksize) + ( (mCMH.Filesize % mCMH.Chunksize) ? 1 : 0 ));

	return true;
}

int	SingleFileChunkMap::GetChunkSize() const
{ 
	if ( mbInitialized )
		return mCMH.Chunksize;
	
	return 0;
}

qint64 SingleFileChunkMap::GetFilesize() const
{	
	if ( mbInitialized )
		return mCMH.Filesize;

	return 0;
}

qint64	SingleFileChunkMap::GetUnmarkedSize() const
{
	if ( !mbInitialized )
		return 0;

	qint64 total = 0;
	qint64 chunks = CountUnmarkedChunks(); 

	if ( !IsChunkMarked(mCMH.TotalChunks - 1) )
	{
		//The last chunk size may differ so get its actual size
		qint64 iRemainder = mCMH.Filesize % GetChunkSize();

		if ( iRemainder > 0 )
		{
			total += iRemainder;
			chunks--;
		}
	}

	return total + chunks * GetChunkSize();
}

qint64	SingleFileChunkMap::GetMarkedSize() const
{
	if ( !mbInitialized )
		return 0;

	qint64 total = 0;
	qint64 chunks = mChunksMarked; 

	if ( IsChunkMarked(mCMH.TotalChunks - 1) )
	{
		//The last chunk size may differ so get its actual size
		qint64 iRemainder = mCMH.Filesize % GetChunkSize();

		if ( iRemainder > 0 )
		{
			total += iRemainder;
			chunks--;
		}
	}

	return total + chunks * GetChunkSize();
}

bool SingleFileChunkMap::SaveSingleFileChunkMap()
{
	QFile	hFile(mSingleFileChunkMapFilename);
	qint64 write_bytes = 0;

	ORIGIN_ASSERT(!mSingleFileChunkMapFilename.isEmpty());

	if (!mbInitialized) return false;

	// rewrite the existing file so that if the app shuts down while the filesystem is deleting the file (as with CREATE_ALWAYS),
	// we don't lose any progress.
	if(!hFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return false;

	if ( !hFile.isWritable() )
		return false;

	// Write the header
	write_bytes = hFile.write((char*)&mCMH, sizeof(chunkmap_header));

	if (sizeof(chunkmap_header) != write_bytes)
		return false;

	// Write the chunkmap
	write_bytes = hFile.write((char*)mpSingleFileChunkMap, mSingleFileChunkMapLen);

	if (mSingleFileChunkMapLen != write_bytes)
		return false;
	
	hFile.close();

	mChunksSinceLastSave = 0;

	return true;
}

bool SingleFileChunkMap::MarkChunk(int ChunkIndex)
{
//	CoreLogWarning(L"Marking chunk #%d", ChunkIndex);

	if ( !mbInitialized )
		return false;

	if (ChunkIndex < 0 || ChunkIndex > (mSingleFileChunkMapLen * 8)) 
		return false;

	// Figure out where this chunk is inside the bitmap
	int Offset = ChunkIndex / 8;

	// determine which bit to check inside the map
	int BitOffset = ChunkIndex % 8;

	// Create a mask
	int	bitMask = 0x01 << BitOffset;

	// If it's not already marked
	if ( (mpSingleFileChunkMap[Offset] & bitMask) == 0x00 )
	{
		mChunksMarked++;
		mChunksSinceLastSave++;

		// turn on the bit that indicate this chunk is processed
		mpSingleFileChunkMap[Offset] |= bitMask;
	}

#ifdef _DEBUG	
	int checkcount = 0;

	for ( int i = 0; i < mSingleFileChunkMapLen; i++ )
		checkcount += bits_in_byte[ mpSingleFileChunkMap[i] ];

	ORIGIN_ASSERT(checkcount == mChunksMarked);
#endif

	if(mChunksSinceLastSave >= MAX_UNSAVED_CHUNKS)
	{
		SaveSingleFileChunkMap();
	}
	return true;
}

bool SingleFileChunkMap::MarkRange(qint64 StartByteIndex, qint64 EndByteIndex)
{
//	CoreLogWarning(L"Marking range #%I64d to %I64d", StartByteIndex, EndByteIndex);

	qint64	byteToMark  = StartByteIndex;
	int internalChunkSize = GetChunkSize();
	bool LastChunk = (EndByteIndex+1 >= mCMH.Filesize);

	//Clamp values
	if ( EndByteIndex > mCMH.Filesize )
		EndByteIndex = mCMH.Filesize;

	if ( StartByteIndex < 0 )
		StartByteIndex = 0;

	// No chunk can possibly be marked
	qint64 BlockSize = (EndByteIndex - StartByteIndex);
	if ( BlockSize < internalChunkSize && !LastChunk )
	{
		return false;
	}

	// Align byteToMark to a chunk boundary
	if (byteToMark % internalChunkSize != 0)
	{
		// Make the starting byte the next chunk boundary
		byteToMark = ((byteToMark/internalChunkSize)+1) * internalChunkSize;
	}

	// Align EndByteIndex to a chunk boundary
	if (BlockSize % internalChunkSize != 0)
	{
		if ( !LastChunk )
		{
			// Make the ending byte the previous chunk boundary
			EndByteIndex = StartByteIndex + (((BlockSize/internalChunkSize)-1) * internalChunkSize) - 1;
		}
	}

	// Check if the end of the chunk is within the number of bytes that are to be marked (unless it's the last chunk)
	if ( byteToMark < EndByteIndex )
	{
		// Mark all the chunks that encompass the CountBytes
		do 
		{
			MarkByte(byteToMark);
			byteToMark += internalChunkSize;

		} while ( (byteToMark + internalChunkSize - 1 <= EndByteIndex) ||
				  (byteToMark < EndByteIndex && LastChunk) );

		//Checking
		ORIGIN_ASSERT( GetUnmarkedSize() + GetMarkedSize() == mCMH.Filesize );

		return true;
	}

	return false;
}

bool SingleFileChunkMap::IsChunkMarked(int ChunkIndex) const
{
	if (!mbInitialized) return false;
	
	ORIGIN_ASSERT(ChunkIndex >= 0 && ChunkIndex <= (8 * mSingleFileChunkMapLen));

	// Figure out where this chunk is inside our bitmap
	int Offset = ChunkIndex / 8;

	// determine which bit to check inside the map
	int BitOffset = ChunkIndex % 8;

	// Create a mask
	int	bitMask = 0x01 << BitOffset;

	return (mpSingleFileChunkMap[Offset] & bitMask) != 0x00;
}

qint64 SingleFileChunkMap::FindNextUnmarkedChunk(int MarkedChunkIndex) const
{
	qint64	rval = -1;

	if (!mbInitialized) 
		return -1;
	
	if (MarkedChunkIndex > (8 * mSingleFileChunkMapLen))
		return -1;

	// Figure out where this chunk is inside out bitmap
	int StartChunk = MarkedChunkIndex / 8;

	// Search for the first cluster that contains an unmarked chunk
	int		map_index;
	qint64 unmarked_chunk = 0;
	int		Offset = StartChunk;

	do
	{
		// Find a cluster with unmarked chunks
		for (map_index = Offset; map_index < mSingleFileChunkMapLen; map_index++)
			if (mpSingleFileChunkMap[map_index] != 0xff) 
				break;

		// If there are unmarked chunks
		if ( map_index < mSingleFileChunkMapLen )
		{
			unmarked_chunk = map_index * 8;
			int mask;
			int startbit = 0;

			// MarkedChunkIndex is being checked
			if (map_index == StartChunk)
				startbit = MarkedChunkIndex % 8;

			// Find the bit inside this map byte
			for (int j=startbit; j < 8; j++)
			{
				mask = 0x01 << j;
				if ((mpSingleFileChunkMap[map_index] & mask) !=  mask)
				{
					rval = unmarked_chunk + j;
					break;
				}
			}

			Offset++;
		}

	} while ((rval < 0) && (map_index < mSingleFileChunkMapLen));

	//Next byte is fully checked and may return an invalid chunk
	if ( rval >= mCMH.TotalChunks )
		rval = -1;

	return rval;
}

qint64	SingleFileChunkMap::FindNextUnmarkedByte (qint64 MarkedByteNumber) const
{
	qint64 rval = -1;

	if (MarkedByteNumber < mCMH.Filesize) 
	{
		ORIGIN_ASSERT(mCMH.Filesize/(1024*8) < INT_MAX);
		ORIGIN_ASSERT(FindNextUnmarkedChunk(ByteToChunk(MarkedByteNumber)) < INT_MAX);

		rval = ChunkToByte(int(FindNextUnmarkedChunk(ByteToChunk(MarkedByteNumber))));

		// Any value outside of a real filesize offset range means there is no unmarked byte
		if (rval < 0 || mCMH.Filesize <= rval)
			rval = -1;
	}

	return rval;
}

int	SingleFileChunkMap::CountUnmarkedChunks() const
{ 
	return qMax((mCMH.TotalChunks - mChunksMarked), 0);
}

bool SingleFileChunkMap::IsByteMarked(qint64 ByteNumber) const
{
	if ( !mbInitialized ) 
		return false;

	return IsChunkMarked( ByteToChunk(ByteNumber) );
}

bool SingleFileChunkMap::MarkByte(qint64 ByteNumber)
{
	if ( !mbInitialized ) 
		return false;

	return MarkChunk( ByteToChunk(ByteNumber) );
}

QString SingleFileChunkMap::GetChunkMapFilename(const QString& localTargetFile)
{
	if(localTargetFile.isEmpty())
	{
		return "";
	}
	else
	{
		return localTargetFile + CHUNK_MAP_FILE_EXTENSION;
	}
}

}
}
