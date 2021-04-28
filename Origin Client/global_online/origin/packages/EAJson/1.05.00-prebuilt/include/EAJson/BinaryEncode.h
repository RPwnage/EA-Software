///////////////////////////////////////////////////////////////////////////////
// BinaryEncode.h
//
// Copyright (c) 2013 Electronic Arts Inc.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////


#ifndef EAJSON_BINARYENCODE_H
#define EAJSON_BINARYENCODE_H


#include <EAJson/internal/Config.h>

 
namespace EA
{
    namespace Json
    {

        /// BinaryEncode
        ///
        /// Useful for storing binary data in Json fields. Note that with Bson you don't need
        /// this and can directly read/write binary data.
        /// The pOutput buffer must refer to different memory than the pInput buffer.
        /// The output will typically be bigger than the input, but never more than 2x the input size.
        /// Returns the required capacity of the output string. The function succeeds if the return
        /// value is <= outputCapacity.
        ///
        /// Example usage:
        ///     char8_t buffer[1024];
        ///     size_t  result = BinaryEncode(myDataVector.data(), myDataVector.size(), buffer, EAArrayCount(buffer));
        ///     if(result <= EAArrayCount(buffer))
        ///         printf("success");
        ///
        EAJSON_API size_t BinaryEncode(const uint8_t* pInputData, size_t inputSize, char8_t* pOutputString, size_t outputCapacity);


        /// BinaryDecode
        ///
        /// Useful for retrieving binary data from Json fields encoded via BinaryEncode. 
        /// The pOutput buffer must refer to different memory than the pInput buffer.
        /// The output will typically be smaller than the input, but never more than the input size.
        /// Returns the required capacity of the output data. The function succeeds if the return
        /// value is <= outputCapacity. Returns -1 if the input string encoding is broken.
        ///
        /// Example usage:
        ///     uint8_t buffer[1024];
        ///     ssize_t result = BinaryEncode(myEncodedString.data(), myEncodedString.size(), buffer, EAArrayCount(buffer));
        ///     if((result >= 0) && (result <= EAArrayCount(buffer)))
        ///         printf("success");
        ///
        EAJSON_API ssize_t BinaryDecode(const char8_t* pInputString, size_t inputSize, uint8_t* pOutputData, size_t outputCapacity);


        /// BinaryEncode
        /// 
        /// Example usage:
        ///     eastl::string8 s("Aladdin:open sesame"), d;
        ///     BinaryEncode(s, d);
        ///     BinaryDecode(d, s);
        ///
        template <typename Vector, typename String>
        bool BinaryEncode(const Vector& inputData, String& outputString)
        {
            const size_t inputSize  = (size_t)inputData.size();
            size_t       outputSize = ((inputSize + 2) / 3 * 4) + 64; // outputSize is the base64 size + 64 for a little extra due to our headers.

            outputString.resize(outputSize);
            size_t result = BinaryEncode(inputData.data(), inputSize, &outputString[0], outputSize);

            if(result <= outputSize)
            {
                outputString.resize(result);
                return true;
            }

            return false;
        }


        /// BinaryDecode
        ///
        /// Example usage:
        ///     eastl::string8 s("Aladdin:open sesame"), d;
        ///     BinaryEncode(s, d);
        ///     BinaryDecode(d, s);
        ///
        template <typename String, typename Vector>
        bool BinaryDecode(const String& inputString, Vector& outputData)
        {
            const size_t   inputSize  = (size_t)inputString.length();
            const char8_t* pInput     = inputString.data();
            size_t         outputSize = 16 + ((inputSize + 3) / 4 * 3); // +16 to handle identifier prefixes BinaryEncode uses.

            outputData.resize(outputSize);
            ssize_t result = BinaryDecode(pInput, inputSize, &outputData[0], outputSize);
            if((result >= 0) && ((size_t)result <= outputSize))
            {
                outputData.resize(result);
                return true;
            }

            return false;
        }

    } // namespace Json

} // namespace EA


#endif // include guard














