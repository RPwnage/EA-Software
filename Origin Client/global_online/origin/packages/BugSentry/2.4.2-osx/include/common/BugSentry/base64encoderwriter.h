/******************************************************************************/
/*!
    Base64EncoderWriter

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRY_BASE64_ENCODER_WRITER_H
#define BUGSENTRY_BASE64_ENCODER_WRITER_H

/*** Includes *****************************************************************/
#include "BugSentry/ireportwriter.h"

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        enum Base64Constants
        {
            BASE64_INPUT_SIZE= 3,
            BASE64_OUTPUT_SIZE= 4
        };

        class Base64EncoderWriter : public IReportWriter
        {
        public:
            Base64EncoderWriter(IReportWriter* writeInterface);
            virtual ~Base64EncoderWriter();

            virtual bool Write(const void* data, int size);
            virtual char* GetCurrentPosition() const { return NULL; }
            void EncodeBase64(const unsigned char* inputData /*BASE64_INPUT_SIZE*/, unsigned char* outputData /*BASE64_OUTPUT_SIZE*/);

        private:
            static const unsigned int BUFFER_CAPACITY = BASE64_INPUT_SIZE; // left over size
            static const char* BASE64_ALPHABET;

            bool WriteEncode(const unsigned char* rawBuffer /*BASE64_INPUT_SIZE*/);

            IReportWriter* mWriteInterface;

            // Base64 encoding requires writing triplets, if we are given less data we need to keep it somewhere.
            unsigned char mBufferOverflow[BUFFER_CAPACITY];
            unsigned int mBufferOverflowCount;
        };
    }
}

#endif //BUGSENTRY_BASE64_ENCODER_WRITER_H
