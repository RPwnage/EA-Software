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
#include "framework/util/compression/zlibinflate.h"

namespace Blaze
{

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief ZlibInflate
     
     ZlibInflate constructor    
*/
/*************************************************************************************************/
ZlibInflate::ZlibInflate()   
{   
    initialize();
}

/*************************************************************************************************/
/*!    
    \brief ZlibInflate
     
     ZlibInflate constructor

     \param[in]  inputBuffer - Pointer for input buffer
     \param[in]  inputBufferSize - Size for buffer     
     \param[in]  outputBuffer - Pointer for output buffer
     \param[in]  outputBufferSize - Size for buffer
*/
/*************************************************************************************************/
ZlibInflate::ZlibInflate(void *inputBuffer, size_t inputBufferSize, void *outputBuffer, size_t outputBufferSize)
{
    initialize();
    setInputBuffer(inputBuffer, inputBufferSize);
    setOutputBuffer(outputBuffer, outputBufferSize);
}

/*************************************************************************************************/
/*!
    \brief ~ZlibInflate

    ZlibInflate destructor
*/
/*************************************************************************************************/
ZlibInflate::~ZlibInflate()
{    
    end();
}

/*************************************************************************************************/
/*!
    \brief initialize

     Wrapper around inflateInit2
     - Initializes the internal stream state for decompression.
 
    \return - Zlib error code     

    ZLIB Notes
    ----------
    This is another version of inflateInit with an extra parameter.  The
    fields next_in, avail_in, zalloc, zfree and opaque must be initialized
    before by the caller.
   
    inflateInit2 returns Z_OK if success, Z_MEM_ERROR if there was not enough
    memory, Z_VERSION_ERROR if the zlib library version is incompatible with the
    version assumed by the caller, or Z_STREAM_ERROR if the parameters are
    invalid, such as a null pointer to the structure.  msg is set to null if
    there is no error message.  inflateInit2 does not perform any decompression
    apart from possibly reading the zlib header if present: actual decompression
    will be done by inflate().  (So next_in and avail_in may be modified, but
    next_out and avail_out are unused and unchanged.) The current implementation
    of inflateInit2() does not process any header information -- that is
    deferred until inflate() is called.
*/
/*************************************************************************************************/
int32_t ZlibInflate::initialize()
{   
    clearBufferValues();

    createZlibStream();       

    int32_t zlibErr = inflateInit2(mZStream, ZLIB_INFLATE_WINDOW_BITS);

    // If an error occurred creating the stream
    if (zlibErr != Z_OK)
    {
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
    inflate decompresses as much data as possible, and stops when the input
    buffer becomes empty or the output buffer becomes full.  It may introduce
    some output latency (reading input without producing any output) except when
    forced to flush.

    The detailed semantics are as follows.  inflate performs one or both of the
    following actions:

    - Decompress more input starting at next_in and update next_in and avail_in
    accordingly.  If not all input can be processed (because there is not
    enough room in the output buffer), next_in is updated and processing will
    resume at this point for the next call of inflate().

    - Provide more output starting at next_out and update next_out and avail_out
    accordingly.  inflate() provides as much output as possible, until there is
    no more input data or no more space in the output buffer (see below about
    the flush parameter).

    Before the call of inflate(), the application should ensure that at least
    one of the actions is possible, by providing more input and/or consuming more
    output, and updating the next_* and avail_* values accordingly.  The
    application can consume the uncompressed output when it wants, for example
    when the output buffer is full (avail_out == 0), or after each call of
    inflate().  If inflate returns Z_OK and with zero avail_out, it must be
    called again after making room in the output buffer because there might be
    more output pending.

    The flush parameter of inflate() can be Z_NO_FLUSH, Z_SYNC_FLUSH, Z_FINISH,
    Z_BLOCK, or Z_TREES.  Z_SYNC_FLUSH requests that inflate() flush as much
    output as possible to the output buffer.  Z_BLOCK requests that inflate()
    stop if and when it gets to the next deflate block boundary.  When decoding
    the zlib or gzip format, this will cause inflate() to return immediately
    after the header and before the first block.  When doing a raw inflate,
    inflate() will go ahead and process the first block, and will return when it
    gets to the end of that block, or when it runs out of data.

    The Z_BLOCK option assists in appending to or combining deflate streams.
    Also to assist in this, on return inflate() will set strm->data_type to the
    number of unused bits in the last byte taken from strm->next_in, plus 64 if
    inflate() is currently decoding the last block in the deflate stream, plus
    128 if inflate() returned immediately after decoding an end-of-block code or
    decoding the complete header up to just before the first byte of the deflate
    stream.  The end-of-block will not be indicated until all of the uncompressed
    data from that block has been written to strm->next_out.  The number of
    unused bits may in general be greater than seven, except when bit 7 of
    data_type is set, in which case the number of unused bits will be less than
    eight.  data_type is set as noted here every time inflate() returns for all
    flush options, and so can be used to determine the amount of currently
    consumed input in bits.

    The Z_TREES option behaves as Z_BLOCK does, but it also returns when the
    end of each deflate block header is reached, before any actual data in that
    block is decoded.  This allows the caller to determine the length of the
    deflate block header for later use in random access within a deflate block.
    256 is added to the value of strm->data_type when inflate() returns
    immediately after reaching the end of the deflate block header.

    inflate() should normally be called until it returns Z_STREAM_END or an
    error.  However if all decompression is to be performed in a single step (a
    single call of inflate), the parameter flush should be set to Z_FINISH.  In
    this case all pending input is processed and all pending output is flushed;
    avail_out must be large enough to hold all the uncompressed data.  (The size
    of the uncompressed data may have been saved by the compressor for this
    purpose.) The next operation on this stream must be inflateEnd to deallocate
    the decompression state.  The use of Z_FINISH is never required, but can be
    used to inform inflate that a faster approach may be used for the single
    inflate() call.

    In this implementation, inflate() always flushes as much output as
    possible to the output buffer, and always uses the faster approach on the
    first call.  So the only effect of the flush parameter in this implementation
    is on the return value of inflate(), as noted below, or when it returns early
    because Z_BLOCK or Z_TREES is used.

    If a preset dictionary is needed after this call (see inflateSetDictionary
    below), inflate sets strm->adler to the adler32 checksum of the dictionary
    chosen by the compressor and returns Z_NEED_DICT; otherwise it sets
    strm->adler to the adler32 checksum of all output produced so far (that is,
    total_out bytes) and returns Z_OK, Z_STREAM_END or an error code as described
    below.  At the end of the stream, inflate() checks that its computed adler32
    checksum is equal to that saved by the compressor and returns Z_STREAM_END
    only if the checksum is correct.

    inflate() can decompress and check either zlib-wrapped or gzip-wrapped
    deflate data.  The header type is detected automatically, if requested when
    initializing with inflateInit2().  Any information contained in the gzip
    header is not retained, so applications that need that information should
    instead use raw inflate, see inflateInit2() below, or inflateBack() and
    perform their own processing of the gzip header and trailer.

    inflate() returns Z_OK if some progress has been made (more input processed
    or more output produced), Z_STREAM_END if the end of the compressed data has
    been reached and all uncompressed output has been produced, Z_NEED_DICT if a
    preset dictionary is needed at this point, Z_DATA_ERROR if the input data was
    corrupted (input stream not conforming to the zlib format or incorrect check
    value), Z_STREAM_ERROR if the stream structure was inconsistent (for example
    next_in or next_out was Z_NULL), Z_MEM_ERROR if there was not enough memory,
    Z_BUF_ERROR if no progress is possible or if there was not enough room in the
    output buffer when Z_FINISH is used.  Note that Z_BUF_ERROR is not fatal, and
    inflate() can be called again with more input and more output space to
    continue decompressing.  If Z_DATA_ERROR is returned, the application may
    then call inflateSync() to look for a good compression block if a partial
    recovery of the data is desired.
*/
/*************************************************************************************************/
int32_t ZlibInflate::execute(int32_t flushType)
{
    if (mStreamReady == false)
    {
        EA_ASSERT(false);
        BLAZE_ERR(Log::SYSTEM, "[ZlibInflate].execute: Stream is not ready for compression!!");  

        return Z_STREAM_ERROR;
    }    
    else if (mInputBuffer == nullptr || mOutputBuffer == nullptr || mOutputBufferSize <= 0)
    {
        EA_ASSERT(false);
        BLAZE_ERR(Log::SYSTEM, "[ZlibInflate].execute: Invalid input/output buffer!!");  

        return Z_BUF_ERROR;
    }

    // Init Source Data
    mZStream->next_in  = static_cast<Bytef *>(mInputBuffer);
    mZStream->avail_in = mInputBufferSize;

    // Init Output Data
    mZStream->next_out  = static_cast<Bytef *>(mOutputBuffer);
    mZStream->avail_out = mOutputBufferSize;    

    // Do decompression
    int32_t zlibErr = inflate(mZStream, flushType);  

    updateStreamBuffers();

    return zlibErr;
}

/*************************************************************************************************/
/*!
    \brief reset

     Wrapper around inflateReset
     - Reset stream data

    \return - Zlib error code     

    ZLIB Notes
    ----------
    This function is equivalent to inflateEnd followed by inflateInit,
    but does not free and reallocate all the internal decompression state.  The
    stream will keep attributes that may have been set by inflateInit2.

    inflateReset returns Z_OK if success, or Z_STREAM_ERROR if the source
    stream state was inconsistent (such as zalloc or state being Z_NULL).
*/
/*************************************************************************************************/
int32_t ZlibInflate::reset()
{        
    int32_t zlibErr = Z_STREAM_ERROR;

    EA_ASSERT(mStreamReady == true);
    if (mStreamReady == true)
    {
        zlibErr = inflateReset(mZStream);
    }

    return zlibErr;    
}

/*************************************************************************************************/
/*!
    \brief end

     Wrapper around inflateEnd
     - Destroy stream data
     
    \return - Zlib error code     

    ZLIB Notes
    ----------
    All dynamically allocated data structures for this stream are freed.
    This function discards any unprocessed input and does not flush any pending
    output.

    inflateEnd returns Z_OK if success, Z_STREAM_ERROR if the stream state
    was inconsistent.  In the error case, msg may be set but then points to a
    static string (which must not be deallocated).
*/
/*************************************************************************************************/
int32_t ZlibInflate::end()
{
    int zLibErr = Z_DATA_ERROR;

    EA_ASSERT(mStreamReady == true);
    if (mStreamReady == true)
    {
        zLibErr = inflateEnd(mZStream);
        destroyZlibStream();   
    }    

    return zLibErr;
}

size_t ZlibInflate::getDecompressedDataSize(size_t originalDataSize)
{
    if (mStreamReady == false)
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[ZlibInflate].getDecompressedDataSize: Stream not ready.");
        return 0;
    }

    return originalDataSize * MAX_COMPRESSION_LEVEL;
}

/*** Protected Methods ******************************************************************************/

/*** Private Methods ******************************************************************************/

} // Blaze

