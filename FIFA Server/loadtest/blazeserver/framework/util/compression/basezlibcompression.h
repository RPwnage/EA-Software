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

#ifndef BASEZLIBCOMPRESSIONCODEC_H
#define BASEZLIBCOMPRESSIONCODEC_H

/*** Include files *******************************************************************************/

// Zlib Includes
#include "zlib.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class BaseZlibCompression
{
public:    
    static const int32_t DEFAULT_COMPRESSION_LEVEL;
    static const int32_t MAX_COMPRESSION_LEVEL;
    
    BaseZlibCompression();    
    virtual ~BaseZlibCompression();  

    // Input/Output Buffers
    virtual void setInputBuffer(void *inputBuffer, size_t inputBufferSize);
    virtual void setOutputBuffer(void *outputBuffer, size_t outputBufferSize);       

    // Get compression status
    virtual size_t getInputBytesAvailable() const { return (mStreamReady == true) ? mZStream->avail_in : 0;  }
    virtual size_t getOutputBytesAvailable() const {  return (mStreamReady == true) ? mZStream->avail_out : 0;  }
    virtual size_t getTotalOut() const { return (mStreamReady == true) ? mZStream->total_out : 0; }       
    
    static const char8_t* getZlibInterfaceVersion();
    static const char8_t* getZlibVersion();

protected:                
    // ZLib Constants for compression
    static const int32_t ZLIB_MEMORY_LEVEL;
    static const int32_t ZLIB_COMPRESSION_STRATEGY;
    static const int32_t ZLIB_WINDOW_SIZE;
    static const int32_t ZLIB_DEFLATE_ZLIB_COMPRESSION_WINDOW_BITS;
    static const int32_t ZLIB_DEFLATE_GZIP_COMPRESSION_WINDOW_BITS;
    static const int32_t ZLIB_INFLATE_WINDOW_BITS;

    z_stream* mZStream;
    bool mStreamReady;
    void* mInputBuffer;
    size_t mInputBufferSize;
    void* mOutputBuffer;
    size_t mOutputBufferSize;       
    
    // Stream Helpers
    void createZlibStream();
    void destroyZlibStream(); 
    void clearBufferValues();
    void updateStreamBuffers();

    // Zlib allocators
    // typedef voidpf (*alloc_func) OF((voidpf opaque, uInt items, uInt size));
    // typedef void   (*free_func)  OF((voidpf opaque, voidpf address));
    static voidpf zAlloc(voidpf opaque, uInt items, uInt size);
    static void   zFree(voidpf opaque, voidpf address);       
};
} // Blaze
#endif // BASEZLIBCOMPRESSIONCODEC_H
