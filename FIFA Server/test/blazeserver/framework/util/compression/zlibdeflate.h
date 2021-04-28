/*************************************************************************************************/
/*!
    \file   zlibdeflate.h


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

#ifndef ZLIBDEFLATE_H
#define ZLIBDEFLATE_H

/*** Include files *******************************************************************************/

// Framework Includes
#include "framework/util/compression/basezlibcompression.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class ZlibDeflate : public BaseZlibCompression
{
public:
    // Supported compression types
    enum CompressionType
    {
        COMPRESSION_TYPE_ZLIB = 0,
        COMPRESSION_TYPE_GZIP,
        COMPRESSION_TYPE_COUNT
    };
      
    explicit ZlibDeflate(CompressionType compressionType);    
    ZlibDeflate(CompressionType compressionType, int32_t compressionLevel);    
    ~ZlibDeflate() override;   

    size_t getWorstCaseCompressedDataSize(size_t originalDataSize);         

    int32_t initialize(CompressionType compressionType, int32_t compressionLevel, int32_t memoryLevel = ZLIB_MEMORY_LEVEL, int32_t compressionStrategy = ZLIB_COMPRESSION_STRATEGY);   
    int32_t execute(int32_t flushType);
    int32_t end();
    int32_t reset();  
};
} // Blaze
#endif // ZLIBDEFLATE_H
