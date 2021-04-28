// zlibDecompressor
// Adapted by Alex Zvenigorodsky from a wrapper written for HTTP decompression in EAWebKit by Arpit Baldeva

// Limitations:
// zLib supports max stream length of signed 32bits.  So each individual stream processed can't be larger than 2GB.  (Needs verification... may be possible to do longer streams.)

#ifndef ZLIBDECOMPRESSOR_H
#define ZLIBDECOMPRESSOR_H

//#include "platform.h"
//#include <windows.h>
#include <stdint.h>
#include "zlib.h"
#include <QString>
#include <QFile>

namespace Origin
{
namespace Downloader
{
    namespace ZLibDecompressor
    {
        //This stream decompressor is able to decompress a stream compressed using "DEFLATE" algorithm. 
        class DeflateStreamDecompressor
        {
        public:
            DeflateStreamDecompressor();
            ~DeflateStreamDecompressor();

            int32_t	    Init();
            int32_t     Shutdown();

            int32_t     ResumeStream(const QString& pStreamPersistFilename);       // restore stream state from the specified file
            int32_t     SaveStream(const QString& pStreamPersistFilename);

            int32_t     StartBuffer(uint8_t* sourceStream, int32_t sourceLength);
            int32_t     Decompress();

            bool        HasMoreOutput();    // true if there is more output pending that didn't fit into the output buffer
            bool        NeedsMoreInput();   // true if the decompressor hasn't reached the end of the stream (Z_STREAM_END)

            void        FreeDecompressionBuffer();

            uint8_t*    GetDecompressedBuffer() { return mDecompressionBuffer; }
            uint64_t    GetDecompressedBytes() { return mBufferDecompressedBytes; }

            uint64_t    GetTotalInputBytesProcessed() { return mTotalInputBytesProcessed; }
			uint64_t	GetTotalOutputBytes() { return mTotalOutputBytes; }

            int32_t     ProcessPKZipHeader(uint8_t* sourceStream, int32_t sourceLength, int32_t& inputBytesProcessed);


        private:
            void        AllocateDecompressionBuffer( int32_t sourceLength );

			// Helpers to load in the stream data
			int32_t		ReadStreamV1(QFile& file);
			int32_t		ReadStreamV2(QFile& file);
			int32_t		ReadStreamV3(QFile& file);


            z_stream*	mZStream;                       // Stream object including state
            uint8_t*	mDecompressionBuffer;           // Space to decompress
            size_t		mDecompressionBufferCapacity;   // Size of decompression buffer  
            uint64_t    mBufferDecompressedBytes;             // Number of valid bytes in the decompression buffer

            int32_t     mStatus;

            uint64_t    mTotalInputBytesProcessed;      // all the compressed data passed in
			uint64_t    mTotalOutputBytes;				// all of the decompressed data returned
            bool		mInitialized;
            bool        mCheckForStreamTypeError;       // Flag for responding to Z_DATA_ERROR which may mean that a PKZip Header needs to be parsed.
            bool        mFinalPass;
        };

    }

} // namespace Downloader
} // namespace Origin

#endif 
