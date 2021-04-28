/*! *********************************************************************************************/
/*!
    \file 


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#include <test.h>
#include <test/tdf/alltypes.h>
#include <EATDF/tdfblob.h>
#include <EAStdC/EAString.h>

TEST(tdfblobtest, blobTest)
{
    /*The stream 'ABCD' is 32 bits long.  It is mapped as
    follows:
    A (65)     B (66)     C (67)     D (68)   (None) (None)
    01000001   01000010   01000011   01000100

    16 (Q)  20 (U)  9 (J)   3 (D)    17 (R) 0 (A)  NA (=) NA (=)
    010000  010100  001001  000011   010001 000000 000000 000000
    
    QUJDRA==
    */
    {
        const char8_t data[] = "ABCD";
        const size_t dataSize = strlen(data);
        const char8_t encData[] = "QUJDRA==";
        const size_t encDataSize = strlen(encData);

        EA::TDF::TdfBlob blob;
        blob.setData((uint8_t*)data, static_cast<EA::TDF::TdfSizeVal>(dataSize));

        // Encoding
        size_t bufSize = blob.calcBase64EncodedSize() + 1;
        char8_t* encodeBuf = new char8_t[bufSize];
        int32_t numWritten = blob.encodeBase64(encodeBuf, bufSize);
        EXPECT_TRUE(EA::StdC::Strcmp(encodeBuf, encData) == 0);
        EXPECT_TRUE(numWritten == static_cast<int32_t>(encDataSize));

        // Decoding
        int32_t numDecoded = blob.decodeBase64(encodeBuf, static_cast<EA::TDF::TdfSizeVal>(encDataSize));
        EXPECT_TRUE(memcmp(blob.getData(), data, blob.getCount()) == 0);
        EXPECT_TRUE(numDecoded == static_cast<int32_t>(dataSize));

        delete[] encodeBuf;
    }   

    {
        const int32_t numbers[] = {65};
        const size_t numSize = strlen((char8_t*)numbers);
        const char8_t encData[] = "QQ==";
        const size_t encDataSize = strlen(encData);

        EA::TDF::TdfBlob blob;
        blob.setData((uint8_t*)(numbers), static_cast<EA::TDF::TdfSizeVal>(numSize));

        // Encoding
        size_t encodeBuf = blob.calcBase64EncodedSize() + 1;
        char8_t* outputBuf = new char8_t[encodeBuf];
        int32_t numWritten = blob.encodeBase64(outputBuf, encodeBuf);
        EXPECT_TRUE(EA::StdC::Strcmp(outputBuf, encData) == 0);
        EXPECT_TRUE(numWritten == static_cast<int32_t>(encDataSize));

        // Decoding
        int32_t numDecoded = blob.decodeBase64(outputBuf, static_cast<EA::TDF::TdfSizeVal>(encDataSize));
        EXPECT_TRUE(memcmp(blob.getData(), numbers, blob.getCount()) == 0);
        EXPECT_TRUE((size_t)numDecoded == numSize);

        delete[] outputBuf;
    }

    // Error case: encode buffer size does not consider null terminator
    {
        const char8_t data[] = "ABCD";
        const size_t dataSize = strlen(data);

        EA::TDF::TdfBlob blob;
        blob.setData((uint8_t*)data, static_cast<EA::TDF::TdfSizeVal>(dataSize));

        // Encoding
        size_t bufSize = blob.calcBase64EncodedSize();
        char8_t* encodeBuf = new char8_t[bufSize];
        int32_t numWritten = blob.encodeBase64(encodeBuf, bufSize);
        EXPECT_TRUE(numWritten == -1);

        delete[] encodeBuf;
    }

    // Error case: insufficient buffer size case
    {
        const char8_t data[] = "ABCD";
        const size_t dataSize = strlen(data);

        char8_t* errBuf = new char8_t[1];
        errBuf[0] = 0xF;
        EA::TDF::TdfBlob blob;
        blob.setData((uint8_t*)data, static_cast<EA::TDF::TdfSizeVal>(dataSize));
        int32_t numWritten = blob.encodeBase64(errBuf, 1);
        EXPECT_TRUE(errBuf[0] == '\0');
        EXPECT_TRUE(numWritten == -1);
        delete[] errBuf;
    }
    
    // Error case: Empty Blob case
    {
        EA::TDF::TdfBlob blob;
        size_t bufSize = blob.calcBase64EncodedSize() + 1;
        char8_t* outputBuf = new char8_t[bufSize];
        int32_t numWritten = blob.encodeBase64(outputBuf, bufSize);

        EXPECT_TRUE(EA::StdC::Strcmp(outputBuf, "") == 0);
        EXPECT_TRUE(numWritten == 0);
        delete[] outputBuf;
    }    
}
