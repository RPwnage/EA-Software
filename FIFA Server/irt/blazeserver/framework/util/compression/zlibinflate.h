/*************************************************************************************************/
/*!
    \file   zlibinflate.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ZlibInflate

    Decompress data using Zlib

*/
/*************************************************************************************************/

#ifndef ZLIBINFLATE_H
#define ZLIBINFLATE_H

/*** Include files *******************************************************************************/

// Framework Includes
#include "framework/util/compression/basezlibcompression.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class ZlibInflate : public BaseZlibCompression
{
public:
    ZlibInflate();      
    ZlibInflate(void *inputBuffer, size_t inputBufferSize, void *outputBuffer, size_t outputBufferSize); 
    ~ZlibInflate() override;   
      
    int32_t initialize();    
    int32_t execute(int32_t flushType);  
    int32_t reset();
    int32_t end();
    size_t getDecompressedDataSize(size_t originalDataSize);
};
} // Blaze
#endif // ZLIBINFLATE_H
