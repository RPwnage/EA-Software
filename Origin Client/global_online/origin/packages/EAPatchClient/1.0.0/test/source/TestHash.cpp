/////////////////////////////////////////////////////////////////////////////
// EAPatchClientTest/TestHash.cpp
//
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include <EAPatchClientTest/Test.h>
#include <EAPatchClient/Hash.h>
#include <EAStdC/EARandom.h>
#include <EATest/EATest.h>
#include <stdlib.h>



///////////////////////////////////////////////////////////////////////////////
// TestHash
//
int TestHash()
{
    int nErrorCount(0);

    // Test RollingChecksum
    {
        // To do: Make this a much bigger and more generic test.
        // To do: Test the handling of file data that's not of a full block increment. Should act like 0-padded.

        const size_t kBufferSize = 16384;
        const size_t kBlockSize  =  2048;    // Must be power of 2.
        const size_t kBlockShift =    11;    // log2(2048)
     
        EA::StdC::Random rand;
        uint8_t* buffer = new uint8_t[kBufferSize];
        for(size_t i = 0; i < kBufferSize; i++)
            buffer[i] = (uint8_t)rand.RandomUint32Uniform();
     
        // Get the checksum via entire block calculations.
        uint32_t rsumBlock_0_2048 = EA::Patch::GetRollingChecksum(buffer + 0, kBlockSize);
        uint32_t rsumBlock_1_2049 = EA::Patch::GetRollingChecksum(buffer + 1, kBlockSize);
        uint32_t rsumBlock_2_2050 = EA::Patch::GetRollingChecksum(buffer + 2, kBlockSize);
        uint32_t rsumBlock_3_2051 = EA::Patch::GetRollingChecksum(buffer + 3, kBlockSize);

        // Now do byte-by-byte rolling checksum calculations. These should match the 
        // results given by the entire block calculations above. 
        uint32_t rsum_1_2049 = rsumBlock_0_2048;
        rsum_1_2049 = EA::Patch::UpdateRollingChecksum(rsum_1_2049, buffer[0], buffer[2048], kBlockShift);
        EATEST_VERIFY(rsum_1_2049 == rsumBlock_1_2049);

        uint32_t rsum_2_2050 = rsum_1_2049;
        rsum_2_2050 = EA::Patch::UpdateRollingChecksum(rsum_2_2050, buffer[1], buffer[2049], kBlockShift);
        EATEST_VERIFY(rsum_2_2050 == rsumBlock_2_2050);

        uint32_t rsum_3_2051 = rsum_2_2050;
        rsum_3_2051 = EA::Patch::UpdateRollingChecksum(rsum_3_2051, buffer[2], buffer[2050], kBlockShift);
        EATEST_VERIFY(rsum_3_2051 == rsumBlock_3_2051);

        delete[] buffer;
    }

    // Test HashValueMD5,  HashValueMD5Brief, etc.
    {
        // To do.
        // class MD5
        //    void Begin();
        //    void AddData(const void* data, size_t dataLength);
        //    void End(HashValueMD5& hashValue);
        //
        // GenerateBriefHash()
    }

    return nErrorCount;
}





