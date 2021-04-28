/*************************************************************************************************/
/*!
    \file   zlibdeflate.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ZlibDeflate

    Compresses data using Zlib

*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/

// Framework Includes
#include "framework/blaze.h"
#include "framework/util/compression/zlibdeflate.h"

namespace Blaze
{

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief ZlibDeflate

    ZlibDeflate constructor

    \param[in]  compressionType - Type of compression to use (e.g. zlib/gzip)
*/
/*************************************************************************************************/
ZlibDeflate::ZlibDeflate(CompressionType compressionType)    
{       
    initialize(compressionType, DEFAULT_COMPRESSION_LEVEL);
}

/*************************************************************************************************/
/*!
    \brief ZlibDeflate

    ZlibDeflate constructor

    \param[in]  compressionType - Type of compression to use (e.g. zlib/gzip)
    \param[in]  compressionLevel - Level of compression to use The compression level must be 
                                   DEFAULT_COMPRESSION_LEVEL, or between 0 and 9: 1 gives best speed, 
                                   9 gives best compression, 0 gives no compression at all.
*/
/*************************************************************************************************/
ZlibDeflate::ZlibDeflate(CompressionType compressionType, int32_t compressionLevel)  
{   
    initialize(compressionType, compressionLevel);
}

/*************************************************************************************************/
/*!
    \brief ~ZlibDeflate

    ZlibDeflate destructor
*/
/*************************************************************************************************/
ZlibDeflate::~ZlibDeflate()
{    
    end();
}

/*************************************************************************************************/
/*!
    \brief initialize

     Wrapper around deflateInit2
     - Initializes the internal stream state for compression.

    \param[in]  compressionType - Private data object
    \param[in]  compressionLevel - Number of items to allocate  
    \param[in]  memoryLevel - How much memory should be allocated for the internal compression state.
    \param[in]  compressionStrategy - Used to tune the compression algorithm
    \return - Zlib error code     

    ZLIB Notes
    ----------
    The compression level must be Z_DEFAULT_COMPRESSION, or between 0 and 9:
    1 gives best speed, 9 gives best compression, 0 gives no compression at all
    (the input data is simply copied a block at a time).  Z_DEFAULT_COMPRESSION
    requests a default compromise between speed and compression (currently
    equivalent to level 6).

    The memLevel parameter specifies how much memory should be allocated
    for the internal compression state.  memLevel=1 uses minimum memory but is
    slow and reduces compression ratio; memLevel=9 uses maximum memory for
    optimal speed.  The default value is 8.  See zconf.h for total memory usage
    as a function of windowBits and memLevel.

    The strategy parameter is used to tune the compression algorithm.  Use the
    value Z_DEFAULT_STRATEGY for normal data, Z_FILTERED for data produced by a
    filter (or predictor), Z_HUFFMAN_ONLY to force Huffman encoding only (no
    string match), or Z_RLE to limit match distances to one (run-length
    encoding).  Filtered data consists mostly of small values with a somewhat
    random distribution.  In this case, the compression algorithm is tuned to
    compress them better.  The effect of Z_FILTERED is to force more Huffman
    coding and less string matching; it is somewhat intermediate between
    Z_DEFAULT_STRATEGY and Z_HUFFMAN_ONLY.  Z_RLE is designed to be almost as
    fast as Z_HUFFMAN_ONLY, but give better compression for PNG image data.  The
    strategy parameter only affects the compression ratio but not the
    correctness of the compressed output even if it is not set appropriately.
    Z_FIXED prevents the use of dynamic Huffman codes, allowing for a simpler
    decoder for special applications.

    Returns Z_OK if success, Z_MEM_ERROR if there was not enough
    memory, Z_STREAM_ERROR if any parameter is invalid (such as an invalid
    method), or Z_VERSION_ERROR if the zlib library version (zlib_version) is
    incompatible with the version assumed by the caller (ZLIB_VERSION). 
*/
/*************************************************************************************************/
int32_t ZlibDeflate::initialize(CompressionType compressionType, int32_t compressionLevel, int32_t memoryLevel /*=ZLIB_MEMORY_LEVEL*/, int32_t compressionStrategy/*= ZLIB_COMPRESSION_STRATEGY*/)
{   
    int32_t windowBits = ZLIB_DEFLATE_ZLIB_COMPRESSION_WINDOW_BITS;

    clearBufferValues();

    createZlibStream();        

    switch (compressionType)
    {
    case COMPRESSION_TYPE_ZLIB:
        windowBits = ZLIB_DEFLATE_ZLIB_COMPRESSION_WINDOW_BITS;
        break;

    case COMPRESSION_TYPE_GZIP:
        windowBits = ZLIB_DEFLATE_GZIP_COMPRESSION_WINDOW_BITS;
        break;

    default:
        EA_ASSERT(false);
        windowBits = ZLIB_DEFLATE_ZLIB_COMPRESSION_WINDOW_BITS;
        BLAZE_ERR(Log::SYSTEM, "[ZlibDeflate].initialize: Invalid compression type, Zlib compression");
        break;
    }

    // Initialize compression
    int32_t zlibErr = deflateInit2(mZStream, compressionLevel, Z_DEFLATED, windowBits, memoryLevel, compressionStrategy);

    // If an error occurred creating the stream
    if (zlibErr != Z_OK)
    {        
        EA_ASSERT(false);
        BLAZE_ERR(Log::SYSTEM, "[ZlibDeflate].initialize: Error initializing the zlib stream (ZlibErr: %d)!!", zlibErr);  
        destroyZlibStream();
    }

    return zlibErr;
}

/*************************************************************************************************/
/*!
    \brief execute

    Wrapper around deflate
    - Compresses data

    \param[in]  flushType - Flush type
    \return - Zlib error code    

    ZLIB Notes
    ----------
    deflate compresses as much data as possible, and stops when the input
    buffer becomes empty or the output buffer becomes full.  It may introduce
    some output latency (reading input without producing any output) except when
    forced to flush.

    The detailed semantics are as follows.  deflate performs one or both of the
    following actions:

    - Compress more input starting at next_in and update next_in and avail_in
    accordingly.  If not all input can be processed (because there is not
    enough room in the output buffer), next_in and avail_in are updated and
    processing will resume at this point for the next call of deflate().

    - Provide more output starting at next_out and update next_out and avail_out
    accordingly.  This action is forced if the parameter flush is non zero.
    Forcing flush frequently degrades the compression ratio, so this parameter
    should be set only when necessary (in interactive applications).  Some
    output may be provided even if flush is not set.

    Before the call of deflate(), the application should ensure that at least
    one of the actions is possible, by providing more input and/or consuming more
    output, and updating avail_in or avail_out accordingly; avail_out should
    never be zero before the call.  The application can consume the compressed
    output when it wants, for example when the output buffer is full (avail_out
    == 0), or after each call of deflate().  If deflate returns Z_OK and with
    zero avail_out, it must be called again after making room in the output
    buffer because there might be more output pending.

    Normally the parameter flush is set to Z_NO_FLUSH, which allows deflate to
    decide how much data to accumulate before producing output, in order to
    maximize compression.

    If the parameter flush is set to Z_SYNC_FLUSH, all pending output is
    flushed to the output buffer and the output is aligned on a byte boundary, so
    that the decompressor can get all input data available so far.  (In
    particular avail_in is zero after the call if enough output space has been
    provided before the call.) Flushing may degrade compression for some
    compression algorithms and so it should be used only when necessary.  This
    completes the current deflate block and follows it with an empty stored block
    that is three bits plus filler bits to the next byte, followed by four bytes
    (00 00 ff ff).

    If flush is set to Z_PARTIAL_FLUSH, all pending output is flushed to the
    output buffer, but the output is not aligned to a byte boundary.  All of the
    input data so far will be available to the decompressor, as for Z_SYNC_FLUSH.
    This completes the current deflate block and follows it with an empty fixed
    codes block that is 10 bits long.  This assures that enough bytes are output
    in order for the decompressor to finish the block before the empty fixed code
    block.

    If flush is set to Z_BLOCK, a deflate block is completed and emitted, as
    for Z_SYNC_FLUSH, but the output is not aligned on a byte boundary, and up to
    seven bits of the current block are held to be written as the next byte after
    the next deflate block is completed.  In this case, the decompressor may not
    be provided enough bits at this point in order to complete decompression of
    the data provided so far to the compressor.  It may need to wait for the next
    block to be emitted.  This is for advanced applications that need to control
    the emission of deflate blocks.

    If flush is set to Z_FULL_FLUSH, all output is flushed as with
    Z_SYNC_FLUSH, and the compression state is reset so that decompression can
    restart from this point if previous compressed data has been damaged or if
    random access is desired.  Using Z_FULL_FLUSH too often can seriously degrade
    compression.

    If deflate returns with avail_out == 0, this function must be called again
    with the same value of the flush parameter and more output space (updated
    avail_out), until the flush is complete (deflate returns with non-zero
    avail_out).  In the case of a Z_FULL_FLUSH or Z_SYNC_FLUSH, make sure that
    avail_out is greater than six to avoid repeated flush markers due to
    avail_out == 0 on return.

    If the parameter flush is set to Z_FINISH, pending input is processed,
    pending output is flushed and deflate returns with Z_STREAM_END if there was
    enough output space; if deflate returns with Z_OK, this function must be
    called again with Z_FINISH and more output space (updated avail_out) but no
    more input data, until it returns with Z_STREAM_END or an error.  After
    deflate has returned Z_STREAM_END, the only possible operations on the stream
    are deflateReset or deflateEnd.

    Z_FINISH can be used immediately after deflateInit if all the compression
    is to be done in a single step.  In this case, avail_out must be at least the
    value returned by deflateBound (see below).  If deflate does not return
    Z_STREAM_END, then it must be called again as described above.

    deflate() sets strm->adler to the adler32 checksum of all input read
    so far (that is, total_in bytes).

    deflate() may update strm->data_type if it can make a good guess about
    the input data type (Z_BINARY or Z_TEXT).  In doubt, the data is considered
    binary.  This field is only for information purposes and does not affect the
    compression algorithm in any manner.

    deflate() returns Z_OK if some progress has been made (more input
    processed or more output produced), Z_STREAM_END if all input has been
    consumed and all output has been produced (only when flush is set to
    Z_FINISH), Z_STREAM_ERROR if the stream state was inconsistent (for example
    if next_in or next_out was Z_NULL), Z_BUF_ERROR if no progress is possible
    (for example avail_in or avail_out was zero).  Note that Z_BUF_ERROR is not
    fatal, and deflate() can be called again with more input and more output
    space to continue compressing.
*/
/*************************************************************************************************/
int32_t ZlibDeflate::execute(int32_t flushType)
{
    if (mStreamReady == false)
    {
        EA_ASSERT(false);
        BLAZE_ERR(Log::SYSTEM, "[ZlibDeflate].execute: Stream is not ready for compression!!");  

        return Z_STREAM_ERROR;
    }    
    else if (mInputBuffer == nullptr || mOutputBuffer == nullptr || mOutputBufferSize <= 0)
    {
        EA_ASSERT(false);
        BLAZE_ERR(Log::SYSTEM, "[ZlibDeflate].execute: Invlaid input/output buffer!!");  

        return Z_BUF_ERROR;
    }

    // Init Source Data
    mZStream->next_in  = static_cast<Bytef *>(mInputBuffer);
    mZStream->avail_in = mInputBufferSize;

    // Init Output Data
    mZStream->next_out  = static_cast<Bytef *>(mOutputBuffer);
    mZStream->avail_out = mOutputBufferSize;    

    // Do compression
    int32_t zlibErr = deflate(mZStream, flushType);      

    updateStreamBuffers();

    return zlibErr;
}

/*************************************************************************************************/
/*!
    \brief end

    Wrapper around defalateEnd
    - Destroy stream data

    \return - Zlib error code    

    ZLIB Notes
    ----------
    All dynamically allocated data structures for this stream are freed.
    This function discards any unprocessed input and does not flush any pending
    output.

    deflateEnd returns Z_OK if success, Z_STREAM_ERROR if the
    stream state was inconsistent, Z_DATA_ERROR if the stream was freed
    prematurely (some input or output was discarded).  In the error case, msg
    may be set but then points to a static string (which must not be
    deallocated).
*/
/*************************************************************************************************/
int32_t ZlibDeflate::end()
{
    int zLibErr = Z_DATA_ERROR;

    if (mStreamReady == true)
    {
        zLibErr = deflateEnd(mZStream);
        destroyZlibStream();   
    }    

    return zLibErr;
}

/*************************************************************************************************/
/*!
    \brief reset

    Wrapper around deflateReset 
    - Reset stream data

    \return - Zlib error code    

    ZLIB Notes
    ----------
    This function is equivalent to deflateEnd followed by deflateInit,
    but does not free and reallocate all the internal compression state.  The
    stream will keep the same compression level and any other attributes that
    may have been set by deflateInit2.

    deflateReset returns Z_OK if success, or Z_STREAM_ERROR if the source
    stream state was inconsistent (such as zalloc or state being Z_NULL).
*/
/*************************************************************************************************/
int32_t ZlibDeflate::reset()
{
    int32_t zlibErr = Z_STREAM_ERROR;

    if (mStreamReady)
    {
        zlibErr = deflateReset(mZStream);
    }

    return zlibErr;    
}

/*************************************************************************************************/
/*!
    \brief getWorstCaseCompressedDataSize

    Get worst case size of compressed data given the current compression settings

    \param[in]  originalDataSize - Size of buffer to compress  
    \return - Worst case size of buffer
*/
/*************************************************************************************************/
size_t ZlibDeflate::getWorstCaseCompressedDataSize(size_t originalDataSize)
{
    if (mStreamReady == false)
    {
        BLAZE_ERR(Log::SYSTEM, "[ZlibDeflate].getWorstCaseCompressedDataSize: Stream not ready!!");
        return 0;
    }

    return deflateBound(mZStream, originalDataSize);
}

/*** Protected Methods ******************************************************************************/

/*** Private Methods ******************************************************************************/

} // Blaze

