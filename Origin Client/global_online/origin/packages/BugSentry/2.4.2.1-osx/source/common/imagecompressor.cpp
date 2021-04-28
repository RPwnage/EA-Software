/******************************************************************************/
/*!
    ImageCompressor

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Includes *****************************************************************/
#include <stdio.h> // needed for jmemsys.h
#include "BugSentry/imagecompressor.h"
#include "BugSentry/imagesampler.h"
#include "EAStdC/EAMemory.h"
#include "BugSentry/ireportwriter.h"
#include "coreallocator/icoreallocator_interface.h"
#include "eajpeg/jmemsys.h"

/*** Variables ****************************************************************/
int EA::BugSentry::ImageCompressor::sBytesRequested = 0;

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        /******************************************************************************/
        /*! ImageCompressor::ImageCompressor

            \brief      Constructor

            \param      allocator - The ICoreAllocator to use for compressing the screenshot.
        */
        /******************************************************************************/
        ImageCompressor::ImageCompressor(EA::Allocator::ICoreAllocator *allocator)
        {
            EA::JPEG::jpeg_set_allocator(allocator);
            EA::StdC::Memset8(mUncompressedScanline, 0, sizeof(mUncompressedScanline));
        }

        /******************************************************************************/
        /*! ImageCompressor::Compress

            \brief      Compresses the specified Image data and writes the result using the writer interface.

            \param       - sampler - The object to access the image data.
            \param       - writeInterface - The interface to write the compressed image to.
            \return      - true if the compression was successful, false if there was an error.
        */
        /******************************************************************************/
        bool ImageCompressor::Compress(ImageSampler* sampler, IReportWriter* writeInterface)
        {
            EA::JPEG::jpeg_compress_struct compressionInfo;
            EA::JPEG::jpeg_error_mgr errorManager;
            CompressionDestinationManager destinationManager;

            // hook it up to our own thing
            destinationManager.mWriteInterface = writeInterface;
            destinationManager.mDefaultDestinationMgr.init_destination = &InitDestination;
            destinationManager.mDefaultDestinationMgr.empty_output_buffer = &EmptyOutputBuffer;
            destinationManager.mDefaultDestinationMgr.term_destination = &TerminateDestination;

            compressionInfo.err = EA::JPEG::jpeg_std_error(&errorManager);

            //1. allocate compression object
            EA::JPEG::jpeg_create_compress(&compressionInfo);

            //2. specify destination, reinterpret_cast is the suggested thing to do here
            compressionInfo.dest = reinterpret_cast<EA::JPEG::jpeg_destination_mgr*>(&destinationManager);

            //3. set parameters for compression
            compressionInfo.image_width = static_cast<EA::JPEG::JDIMENSION>(sampler->GetWidth());
            compressionInfo.image_height = static_cast<EA::JPEG::JDIMENSION>(sampler->GetHeight());
            compressionInfo.input_components = 3; // RGB
            compressionInfo.in_color_space = EA::JPEG::JCS_RGB;
            EA::JPEG::jpeg_set_defaults(&compressionInfo);

            //4. start compressor
            EA::JPEG::jpeg_start_compress(&compressionInfo, TRUE);

            //5. deal with each scanline
            // make sure we guard against larger images that could exist for some weird reason
            if (compressionInfo.image_width <= static_cast<unsigned int>(MAX_SCANLINE_WIDTH))
            {
                EA::JPEG::JSAMPROW rowPointer = &mUncompressedScanline[0];

                while (compressionInfo.next_scanline < compressionInfo.image_height)
                {
                    ImagePixel *outputPixel = reinterpret_cast<ImagePixel*>(&mUncompressedScanline[0]);

                    // build compression input buffer or easy consumer interface
                    for (int x= 0; x < static_cast<int>(compressionInfo.image_width); ++x)
                    {
                        *outputPixel++ = sampler->GetPixel(x, static_cast<int>(compressionInfo.next_scanline));
                    }

                    // write the scan line
                    EA::JPEG::jpeg_write_scanlines(&compressionInfo, &rowPointer, 1);
                }
            }

            //6. finish compression
            EA::JPEG::jpeg_finish_compress(&compressionInfo);

            //7. release compression object
            EA::JPEG::jpeg_destroy_compress(&compressionInfo);

            return true;
        }

        /******************************************************************************/
        /*! ImageCompressor::InitDestination

            \brief      Required libjpeg function - InitDestination.

            \param       - compressionInfo - The compression information for libjpeg.
            \return      - none.
        */
        /******************************************************************************/
        void ImageCompressor::InitDestination(EA::JPEG::j_compress_ptr compressionInfo)
        {
            CompressionDestinationManager* destinationManager = reinterpret_cast<CompressionDestinationManager*>(compressionInfo->dest);

            // clear the memory
            EA::StdC::Memset8(&destinationManager->mTemporaryBuffer, 0, sizeof(destinationManager->mTemporaryBuffer));

            destinationManager->mDefaultDestinationMgr.free_in_buffer = sizeof(destinationManager->mTemporaryBuffer);
            destinationManager->mDefaultDestinationMgr.next_output_byte = &destinationManager->mTemporaryBuffer[0];
        }

        /******************************************************************************/
        /*! ImageCompressor::EmptyOutputBuffer

            \brief      Required libjpeg function - EmptyOutputBuffer.

            \param       - compressionInfo - The compression information for libjpeg.
            \return      - true if successful, false if there was an error.
        */
        /******************************************************************************/
        uint8_t ImageCompressor::EmptyOutputBuffer(EA::JPEG::j_compress_ptr compressionInfo)
        {
            bool success = false;

            CompressionDestinationManager* destinationManager = reinterpret_cast<CompressionDestinationManager*>(compressionInfo->dest);

            // should empty everything
            success = destinationManager->mWriteInterface->Write(destinationManager->mTemporaryBuffer, sizeof(destinationManager->mTemporaryBuffer));

            // reset our write buffer
            destinationManager->mDefaultDestinationMgr.free_in_buffer = sizeof(destinationManager->mTemporaryBuffer);
            destinationManager->mDefaultDestinationMgr.next_output_byte = &destinationManager->mTemporaryBuffer[0];
            
            return static_cast<uint8_t>(success);
        }

        /******************************************************************************/
        /*! ImageCompressor::TerminateDestination

            \brief      Required libjpeg function - TerminateDestination.

            \param       - compressionInfo - The compression information for libjpeg.
            \return      - none.
        */
        /******************************************************************************/
        void ImageCompressor::TerminateDestination(EA::JPEG::j_compress_ptr compressionInfo)
        {
            CompressionDestinationManager* destinationManager = reinterpret_cast<CompressionDestinationManager*>(compressionInfo->dest);

            int bytesLeft = static_cast<int>(sizeof(destinationManager->mTemporaryBuffer) - destinationManager->mDefaultDestinationMgr.free_in_buffer);

            // Write any data remaining in the buffer
            if (bytesLeft > 0)
            {
                destinationManager->mWriteInterface->Write(destinationManager->mTemporaryBuffer, bytesLeft);
            }
        }
    }
}