/******************************************************************************/
/*!
    ImageCompressor

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_IMAGE_COMPRESSOR_H
#define BUGSENTRY_IMAGE_COMPRESSOR_H

/*** Includes *****************************************************************/
#ifndef INCLUDED_eabase_H
    #include "eabase/eabase.h"
#endif
#include "BugSentry/imagesampler.h"
#include "eajpeg/jpeglib.h"

/*** Forward Declarations *****************************************************/
namespace EA
{
    namespace BugSentry
    {
        class IReportWriter;
    }

    namespace Allocator
    {
        class ICoreAllocator;
    }
}

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        class ImageCompressor
        {
        public:
            ImageCompressor(EA::Allocator::ICoreAllocator *allocator);
            ~ImageCompressor() {};

            bool Compress(ImageSampler* sampler, IReportWriter* writeInterface);

            static int sBytesRequested;

        private:
            static const int MAX_SCANLINE_WIDTH = 1920;
            static const int BYTES_PER_PIXEL = 3;
            unsigned char mUncompressedScanline[MAX_SCANLINE_WIDTH*BYTES_PER_PIXEL];

            static void *TempAlloc(size_t size);
            static void TempFree(void *ptr);
            static void InitDestination(EA::JPEG::j_compress_ptr compressionInfo);
            static unsigned char EmptyOutputBuffer(EA::JPEG::j_compress_ptr compressionInfo);
            static void TerminateDestination(EA::JPEG::j_compress_ptr compressionInfo);
        };

        class CompressionDestinationManager
        {
        public:
            static const int INTERMEDIATE_BUFFER_SIZE = 96;
            EA::JPEG::jpeg_destination_mgr mDefaultDestinationMgr;
            IReportWriter* mWriteInterface;
            EA::JPEG::JOCTET mTemporaryBuffer[INTERMEDIATE_BUFFER_SIZE];
        };
    }
}

#endif // BUGSENTRY_IMAGE_COMPRESSOR_H
