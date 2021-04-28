/*************************************************************************************************/
/*!
    \file   basezlibcompression.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class basezlibcompression

    Base class for Zlib compressor/decompressor

*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/

// Framework Includes
#include "framework/blaze.h"
#include "framework/util/compression/basezlibcompression.h"

namespace Blaze
{

// Default compression level
// Z_DEFAULT_COMPRESSION requests a default compromise between speed and compression (currently
// equivalent to level 6).
const int32_t BaseZlibCompression::DEFAULT_COMPRESSION_LEVEL = Z_DEFAULT_COMPRESSION;

// The compression level must be Z_DEFAULT_COMPRESSION, or between 0 and 9: 1 gives best speed, 
// 9 gives best compression, 0 gives no compression at all (the input data is simply copied a block at a time)
const int32_t BaseZlibCompression::MAX_COMPRESSION_LEVEL = Z_BEST_COMPRESSION;

// memLevel=1 uses minimum memory but is slow and reduces compression ratio; memLevel=9 uses maximum 
// memory for optimal speed.  The default value is 8.  See zconf.h for total memory usage as a 
// function of windowBits and memLevel.
const int32_t BaseZlibCompression::ZLIB_MEMORY_LEVEL = 8;

// Used to tune the compression algorithm. Use the value X_DEFAULT_STRATEGY for normal data, Z_FILTERED 
// for data produced by a filter (or predictor), or Z_HUFFMAN_ONLY to force Huffman encoding only 
// (no string match)
const int32_t BaseZlibCompression::ZLIB_COMPRESSION_STRATEGY = Z_DEFAULT_STRATEGY;

// The windowBits parameter is the base two logarithm of the window size (the size of the history buffer). 
// It should be in the range 8..15 for this version of the library. Larger values of this parameter result 
// in better compression at the expense of memory usage. The default value is 15 if deflateInit is used instead.
const int32_t BaseZlibCompression::ZLIB_WINDOW_SIZE = 15;
const int32_t BaseZlibCompression::ZLIB_DEFLATE_ZLIB_COMPRESSION_WINDOW_BITS = BaseZlibCompression::ZLIB_WINDOW_SIZE;

// Add 16 to windowBits to write a simple gzip header and trailer around the compressed data instead of a zlib wrapper.
const int32_t BaseZlibCompression::ZLIB_DEFLATE_GZIP_COMPRESSION_WINDOW_BITS = (ZLIB_WINDOW_SIZE + 16);

// Add 32 to windowBits to enable zlib and gzip decoding with automatic header detection.
const int32_t BaseZlibCompression::ZLIB_INFLATE_WINDOW_BITS = (ZLIB_WINDOW_SIZE + 32);

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief BaseZlibCompression

    BaseZlibCompression constructor
*/
/*************************************************************************************************/
BaseZlibCompression::BaseZlibCompression() :
    mZStream(nullptr),
    mStreamReady(false),
    mInputBuffer(nullptr),
    mOutputBuffer(nullptr)
{            
}

/*************************************************************************************************/
/*!
    \brief ~BaseZlibCompression

    BaseZlibCompression destructor
*/
/*************************************************************************************************/
BaseZlibCompression::~BaseZlibCompression()
{           
}

const char8_t* BaseZlibCompression::getZlibInterfaceVersion()
{
    return ZLIB_VERSION;
}

const char8_t* BaseZlibCompression::getZlibVersion()
{
    return zlibVersion();
}

/*************************************************************************************************/
/*!
    \brief setInputBuffer

    Set input buffer for compression/decompression

    \param[in]  inputBuffer - Pointer for input buffer
    \param[in]  inputBufferSize - Size for buffer
*/
/*************************************************************************************************/
void BaseZlibCompression::setInputBuffer(void *inputBuffer, size_t inputBufferSize)
{
    mInputBuffer = inputBuffer;
    mInputBufferSize = inputBufferSize;
}

/*************************************************************************************************/
/*!
    \brief setOutputBuffer

    Set input buffer for compression/decompression

    \param[in]  outputBuffer - Pointer for output buffer
    \param[in]  outputBufferSize - Size for buffer
*/
/*************************************************************************************************/
void BaseZlibCompression::setOutputBuffer(void *outputBuffer, size_t outputBufferSize)
{    
    mOutputBuffer = outputBuffer;
    mOutputBufferSize = outputBufferSize;
}

/*************************************************************************************************/
/*!
    \brief clearBufferValues

    Clear input/output buffer values
*/
/*************************************************************************************************/
void BaseZlibCompression::clearBufferValues()
{
    mInputBuffer = nullptr;
    mInputBufferSize = 0;
    mOutputBuffer = nullptr;
    mOutputBufferSize = 0;    
}

/*** Protected Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief createZlibStream

    Create new stream object for use with ZLib
*/
/*************************************************************************************************/
void BaseZlibCompression::createZlibStream()
{
    EA_ASSERT(mZStream == nullptr);

    if (mZStream == nullptr)
    {
        mZStream = BLAZE_NEW_NAMED("BazeZlibCompressionCodec::ZlibStream") z_stream;   
        memset(mZStream, 0, sizeof(*mZStream));

        mZStream->opaque = this;
        mZStream->zalloc = zAlloc;
        mZStream->zfree  = zFree; 

        mStreamReady = true;
    }     
}

/*************************************************************************************************/
/*!
    \brief destroyZlibStream

    Destroy stream object
*/
/*************************************************************************************************/
void BaseZlibCompression::destroyZlibStream()
{
    EA_ASSERT(mZStream != nullptr);
    if (mZStream != nullptr)
    {
        delete mZStream;
        mZStream = nullptr;

        mStreamReady = false;
    }
}   

/*************************************************************************************************/
/*!
    \brief updateStreamBuffers

    Update input/out buffer pointers
*/
/*************************************************************************************************/
void BaseZlibCompression::updateStreamBuffers()
{
    if (mStreamReady)
    {
        // Update buffer pointers
        mInputBuffer = mZStream->next_in;
        mOutputBuffer = mZStream->next_out;
    }
}


/*************************************************************************************************/
/*!
    \brief zAlloc

    Used to allocate data for Zlib

    \param[in]  opaque - Private data object
    \param[in]  items - Number of items to allocate
    \param[in]  size - Size of items to allocate  
    \return - Pointer to newly allocated memory

    typedef voidpf (*alloc_func) OF((voidpf opaque, uInt items, uInt size));
*/
/*************************************************************************************************/
voidpf BaseZlibCompression::zAlloc(voidpf opaque, uInt items, uInt size)
{
    voidpf newAllocation = BLAZE_ALLOC_MGID(items * size, MEM_GROUP_FRAMEWORK_CATEGORY, "BaseZlibCompression::zAlloc");

    if (newAllocation == nullptr)
    {
        EA_ASSERT(false);
        BLAZE_ERR(Log::SYSTEM, "[BaseZlibCompression:%p].zAlloc: Allocation failed!!", opaque);
    }

    return newAllocation;    
}

/*************************************************************************************************/
/*!
    \brief pushDependency

    Used to free data allocated by Zlib

    \param[in]  opaque - Private data object
    \param[in]  address - Address of memory to deallocate

    typedef void   (*free_func)  OF((voidpf opaque, voidpf address));
*/
/*************************************************************************************************/
void BaseZlibCompression::zFree(voidpf opaque, voidpf address)
{
    BLAZE_FREE(address);    
}

} // Blaze

