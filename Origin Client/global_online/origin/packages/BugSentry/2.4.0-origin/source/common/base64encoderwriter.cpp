/******************************************************************************/
/*!
    Base64EncoderWriter

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     10/17/2008 (lvillegas/mbaylis)
*/
/******************************************************************************/

/*** Includes *****************************************************************/
#include "BugSentry/base64encoderwriter.h"
#include "EAStdC/EAMemory.h"

/*** Variables ****************************************************************/
const char *EA::BugSentry::Base64EncoderWriter::BASE64_ALPHABET =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "+/";

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        /******************************************************************************/
        /*! Base64EncoderWriter::Base64EncoderWriter

            \brief      Constructor

            \param       - writeInterface: The writer interface to use for writing the report.
        */
        /******************************************************************************/
        Base64EncoderWriter::Base64EncoderWriter(IReportWriter* writeInterface)
        {
            mWriteInterface = writeInterface;
            mBufferOverflowCount = 0;
            EA::StdC::Memset8(mBufferOverflow, 0, sizeof(mBufferOverflow));
        }

        /******************************************************************************/
        /*! Base64EncoderWriter::~Base64EncoderWriter

            \brief      Destructor

            \param       - none.
        */
        /******************************************************************************/
        Base64EncoderWriter::~Base64EncoderWriter()
        {
            if ((mBufferOverflowCount > 0) && (mBufferOverflowCount <= BUFFER_CAPACITY))
            {
                unsigned char rawBuffer[BASE64_INPUT_SIZE];
                EA::StdC::Memset8(rawBuffer, 0, sizeof(rawBuffer));

                EA::StdC::Memcpy(rawBuffer, mBufferOverflow, mBufferOverflowCount);

                WriteEncode(rawBuffer);
            }
        }

        /******************************************************************************/
        /*! Base64EncoderWriter::Write

            \brief      Base 64 encode and write the specified data to the writer interface.

            \param       - data: The data to be encoded and written.
            \param       - size: The size of the data.
            \return      - none.
        */
        /******************************************************************************/
        bool Base64EncoderWriter::Write(const void* data, int size)
        {
            bool success = false;

            // some complicated logic here, here is the rational
            // - Base64 encoding needs sets of 3 chars to be written
            //      at the time, which we might or might not get
            //      at time of writing it was guaranteed since we can set the compression
            //      buffer to be a multiple of 3, but that could change at any point
            // - We keep a leftover buffer to keep the overflow chars that could not be grouped and
            //      we would write those with incoming data when we have enough to write one group

            if (data && (size > 0))
            {
                const unsigned char* charData = static_cast<const unsigned char*>(data);
                unsigned int bytesToConsume = static_cast<unsigned int>(size);

                success = true; // optimistic

                // deal with bytes leftover from before, deal first with the case in which
                // we might need to deal with granular bytes being passed in
                if ((mBufferOverflowCount > 0) && (mBufferOverflowCount < BUFFER_CAPACITY))
                {
                    unsigned int bytesToWriteOnOverflow = (BUFFER_CAPACITY - mBufferOverflowCount);
                    bytesToWriteOnOverflow = (bytesToConsume < bytesToWriteOnOverflow) ? bytesToConsume :  bytesToWriteOnOverflow;

                    // if we have anything to fill in the leftover buffer then do it with
                    // input data
                    if (bytesToWriteOnOverflow > 0)
                    {
                        EA::StdC::Memcpy(&mBufferOverflow[mBufferOverflowCount], charData, bytesToWriteOnOverflow);
                        mBufferOverflowCount += bytesToWriteOnOverflow;
                        bytesToConsume -= bytesToWriteOnOverflow;
                        charData += bytesToWriteOnOverflow;
                    }
                }

                // do the encoding of the leftover if it is filled
                if (mBufferOverflowCount >= BUFFER_CAPACITY)
                {
                    success = WriteEncode(mBufferOverflow);
                    mBufferOverflowCount = 0;
                }

                // deal with bytes on the input buffer
                while (success && (bytesToConsume >= BASE64_INPUT_SIZE))
                {
                    success = WriteEncode(charData);
                    charData += BASE64_INPUT_SIZE;
                    bytesToConsume -= BASE64_INPUT_SIZE;
                }

                // at this point we are guaranteed to have an empty leftover buffer
                if (success &&
                    (bytesToConsume > 0) &&
                    (bytesToConsume < BASE64_INPUT_SIZE))
                {
                    // deal with any leftovers
                    EA::StdC::Memcpy(mBufferOverflow, charData, bytesToConsume);
                    mBufferOverflowCount = bytesToConsume;
                }
            }

            return success;
        }

        /******************************************************************************/
        /*! Base64EncoderWriter::WriteEncode

            \brief      Base 64 encode and write the specified data to the writer interface.

            \param       - rawBuffer: The data to be encoded and written.
            \return      - none.
        */
        /******************************************************************************/
        bool Base64EncoderWriter::WriteEncode(const unsigned char* rawBuffer /*BASE64_INPUT_SIZE*/)
        {
            unsigned char encodedBuffer[BASE64_OUTPUT_SIZE];
            EA::StdC::Memset8(encodedBuffer, 0, sizeof(encodedBuffer));

            EncodeBase64(rawBuffer, encodedBuffer);

            return mWriteInterface->Write(encodedBuffer, sizeof(encodedBuffer));
        }

        #if defined(EA_PLATFORM_REVOLUTION)
            #pragma warning off (10317) // implicit arithmetic conversion 
        #endif

        #if defined(EA_PLATFORM_WINDOWS)
            #pragma warning(push,1)
            #pragma warning(disable: 4365)
        #endif

        /******************************************************************************/
        /*! Base64EncoderWriter::EncodeBase64

            \brief      Base 64 encode the specified input buffer to the specified output buffer.

            \param       - inputData: The buffer containing the data to be encoded.
            \param       - outputData: The buffer to write the encoded data to.
            \return      - none.
        */
        /******************************************************************************/
        void Base64EncoderWriter::EncodeBase64(const unsigned char* inputData /*BASE64_INPUT_SIZE*/, unsigned char* outputData /*BASE64_OUTPUT_SIZE*/)
        {
            // base64 encoding is simple, we remap 3 bytes into 4 bytes, with the 
            // ceveat that the 4 bytes are all ASCII representable
            // we do this by explitting the input into sets of 6 bits, all which are translated
            // via the base64 alphabet table above

            // 0x3f is a mask for the rightmost 6 bits
            // inputData[0]&0x3 means the last 2 bits, we shit them left to build the second 
            //    set of 6 bits
            // 0xf is a mask for the rightmost 4 bits

            outputData[0] = inputData[0] >> 2; // use the leftmost 6 bits for the first one
            outputData[1] = ((inputData[0] & 0x3) << 4) | (inputData[1] >> 4);
            outputData[2] = ((inputData[1] & 0xf) << 2) | (inputData[2] >> 6);
            outputData[3] = inputData[2] & 0x3f;

            // encode
            outputData[0] = BASE64_ALPHABET[outputData[0]];
            outputData[1] = BASE64_ALPHABET[outputData[1]];
            outputData[2] = BASE64_ALPHABET[outputData[2]];
            outputData[3] = BASE64_ALPHABET[outputData[3]];
        }

        #if defined(EA_PLATFORM_WINDOWS)
            #pragma warning(pop)
        #endif

        #if defined(EA_PLATFORM_REVOLUTION)
            #pragma warning on (10317)
        #endif
    }
}