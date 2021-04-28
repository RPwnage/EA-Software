///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/Hash.h
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHCLIENT_HASH_H
#define EAPATCHCLIENT_HASH_H


#include <EAPatchClient/Base.h>
#include <EAStdC/EAMemory.h>
#include <EASTL/vector.h>
#include <EASTL/hash_set.h>


namespace EA
{
    namespace Patch
    {
        /// kDiffBlockSizeMin
        /// kDiffBlockSizeMax
        ///
        /// The maximum size of diff blocks we support. 
        /// We have this max size because we need to create runtime buffers that are each at 
        /// least this size. If this size were arbitrarily high then our code would have to be
        /// a lot more complicated to handle it. The values below are configurable, and we 
        /// should be able to tweak them during development of this software.
        /// 
        const size_t kDiffBlockSizeMin       =    1024; // 1 KiB.
        const size_t kDiffBlockShiftMin      =      10; // 2 ^ kDiffBlockShiftMax == kDiffBlockSizeMax. 1 << kDiffBlockShiftMax == kDiffBlockSizeMax.

        const size_t kDiffBlockSizeMax       = 1048576; // 1 KiB.
        const size_t kDiffBlockShiftMax      =      20; // 2 ^ kDiffBlockShiftMax == kDiffBlockSizeMax. 1 << kDiffBlockShiftMax == kDiffBlockSizeMax.


        /// kDiffBlockSizeDefault
        /// 
        /// In un-optimized patches we default to a single block size (and associated block shift).
        /// Such patches are useful for initial testing, as they are much faster to generate.
        ///
        const size_t kDiffBlockSizeDefault   =    8192; // 8 KiB.
        const size_t kDiffBlockShiftDefault  =      13; // 2 ^ kDiffBlockShiftMax == kDiffBlockSizeMax. 1 << kDiffBlockShiftMax == kDiffBlockSizeMax.




        /// HashValueMD5
        ///
        /// The data needed for a hash. 
        /// MD5 hashes are good at generating unique signatures for blocks of data, but they are no
        /// longer considered cryptographically secure. It's within practical limits to generate a 
        /// bogus block of data that has an equal MD5 value. That probably doesn't make MD5 unsuitable
        /// for our patching purposes, though this may have to be reconsidered if we can't guarantee
        /// that the secure transmission of patches can be guaranteed. MD6 is probably too heavy for 
        /// our needs but can be considered if necessary.
        /// http://en.wikipedia.org/wiki/MD5
        ///
        struct HashValueMD5
        {
            uint8_t mData[16];

            HashValueMD5();
            void Clear();
        };

        bool operator==(const HashValueMD5& a, const HashValueMD5& b);
        inline
        bool operator!=(const HashValueMD5& a, const HashValueMD5& b) { return !operator==(a, b); }


        /// HashValueMD5Brief
        ///
        /// This is a shortened version of HashValueMD5, for the purposes of using less space in 
        /// the patch diff data (which can be quite large) at the cost of losing some uniqueness.
        ///
        struct HashValueMD5Brief
        {
            uint8_t mData[8];

            HashValueMD5Brief();
            void Clear();
        };
      
        bool operator==(const HashValueMD5Brief& a, const HashValueMD5Brief& b);
        inline
        bool operator!=(const HashValueMD5Brief& a, const HashValueMD5Brief& b) { return !operator==(a, b); }


        /// Converts full hash (HashValueMD5) into a brief hash (HashValueMD5Brief).
        /// This works by combining the bits of hashMD5 down to a smaller size.
        /// It's impossible to convert hashMD5Brief back to hashMD5.
        void GenerateBriefHash(const HashValueMD5& hashMD5, HashValueMD5Brief& hashMD5Brief);



        /// BlockHash
        ///
        /// Describes the hash for a Block, in both checksum form and MD5 form.
        /// This is useful for doing optimized differential patching. The remote patch file is broken
        /// down into blocks of size N (e.g. 8192), and each of these blocks has one of these BlockHashes
        /// associated with it. The BlockHashes for the file and the size (e.g. 8192) is built ahead
        /// of time by the EAPatchMaker utility. The local machine can use these hashes to quickly and 
        /// efficiently tell what parts of the local file match the remote patch file and thus know the 
        /// parts of the remote file that are different from the local file and need to be downloaded.
        ///
        /// What is a rolling checksum?:
        ///     https://www.google.com/search?q=rolling+checksum
        ///     http://en.wikipedia.org/wiki/Rolling_hash
        ///     https://klubkev.org/rsync/
        ///     http://en.wikipedia.org/wiki/Rsync#Algorithm
        ///
        struct BlockHash
        {
            uint32_t          mChecksum;     // Rolling checksum. This is fast to calculate and can be "rolling", but not as accurate. Used as a first-pass quick check.
            HashValueMD5Brief mHashValue;    // This is slower to calculate and can't be "rolling", but is very accurate. Used as a final check.

        public:
            BlockHash() : mChecksum(), mHashValue() {}
            BlockHash(uint32_t checksum, const HashValueMD5Brief& hashValue) : mChecksum(checksum), mHashValue(hashValue) {}
        };

        /// Defines an array of BlockHashes. There is one such array for each remote file from a patch set.
        typedef eastl::vector<BlockHash, CoreAllocator> BlockHashArray;



        /// BlockHashWithPosition
        ///
        /// This is simply a BlockHash with a position field.
        struct BlockHashWithPosition : public BlockHash
        {
            uint64_t mPosition;

        public:
            BlockHashWithPosition() : BlockHash(), mPosition() {}
            BlockHashWithPosition(uint32_t checksum, const HashValueMD5Brief& hashValue, uint64_t position) : BlockHash(checksum, hashValue), mPosition(position) {}

            /// Used for hash_multiset. 
            struct BlockHashHash{ size_t operator()(const BlockHashWithPosition& bhwp) const { return static_cast<size_t>(bhwp.mChecksum); } } ;

            /// Used for hash_multiset::find_as(uint32_t). 
            struct BlockHashHashEqualTo : public eastl::binary_function<uint32_t, BlockHashWithPosition, bool>
            {
                bool operator()(const uint32_t& a, const BlockHashWithPosition& b) const { return a == b.mChecksum; }
                bool operator()(const BlockHashWithPosition& b, const uint32_t& a) const { return b.mChecksum == a; }
            };
        };
      
        bool operator==(const BlockHashWithPosition& a, const BlockHashWithPosition& b);
        inline
        bool operator!=(const BlockHashWithPosition& a, const BlockHashWithPosition& b) { return !operator==(a, b); }


        /// Defines a hash_multiset of BlockHashWithPosition. The hash key is the BlockHash::mChecksum value. 
        /// This is used for runtime BlockHash lookups during file diff patching. This is an alternative 
        /// to BlockHashArray as a representation for a collection of BlockHashes which is useful for 
        /// runtime lookups.
        typedef eastl::hash_multiset<BlockHashWithPosition, BlockHashWithPosition::BlockHashHash, eastl::equal_to<BlockHashWithPosition>, CoreAllocator> BlockHashWithPositionSet; 


        /// Rolling Checksums
        ///
        /// A checksum is like a poor man's hash. It assigns a value (e.g. uint32_t) to a span
        /// of memory for the purpose of being able to quickly identify it. Checksums are fast
        /// to calculate but typically don't have enough bits to be highly reliable, nor are they
        /// typically cyptographically secure (as they can be reverse-engineered).
        ///
        /// A rolling checksum is useful for where we have a checksum for a block of (e.g.) 1024 bytes, 
        /// and we want to efficiently know the checksum for the same sized block moved by one byte forward.
        /// If you have 3096 bytes in a file, and use a block size of 1024, the rolling checksums are the 
        /// checksums for bytes 0-1024, 1-1025, 2-1026, etc.
    
        /// This is an arbitrary value which is what a checksum is set to in case there was an error reading
        /// the checksum. This is for debugging purposes only. It's possible that a valid checksum for a block
        /// might be this same value. So you can't sue this value to tell that a checksum calculation was in error.
        const uint32_t kChecksumError = 0xffffffff;

        /// UpdateRollingChecksum
        ///
        /// Update a rolling checksum for the next byte in a buffer.
        /// blockShift is log2(blockSize), where block size is the fixed size block that you 
        /// are calculating incremental checksums for.
        /// prevBlockFirstByte is the first byte of the block of data used to calculate the
        /// prevChecksum. nextBlockLastByte is one byte after the block of data used to calculate
        /// the prevChecksum.
        ///
        /// Example usage:
        ///     char   buffer[16384];
        ///     size_t kBlockSize      = 1024;
        ///     size_t kBlockSizeShift = log2(kBlockSize); // log2(1024) == 10.
        ///
        ///     <read into buffer>
        ///     uint32_t checksum = GetRollingChecksum(buffer, kBlockSize);
        ///     <do something with checksum>
        ///     for(int i = 0; i < 16384-kBlockSize; i++) {
        ///         checksum = UpdateRollingChecksum(checksum, buffer[i], buffer[kBlockSize + i], kBlockSizeShift);
        ///         <do something with checksum>
        ///     }
        ///
        uint32_t UpdateRollingChecksum(uint32_t prevChecksum, uint8_t prevBlockFirstByte, uint8_t nextBlockLastByte, size_t blockShift);

        /// GetRollingChecksum
        ///
        /// Gets the checksum for an entire block of data. This is intended to be used in 
        /// conjunction with UpdateRollingChecksum, as this function generates a checksum that
        /// can be moved forward in the data with UpdateRollingChecksum.
        ///
        uint32_t GetRollingChecksum(const uint8_t* pData, size_t dataLength);



        /// MD5
        ///
        /// Implements an MD5 hash of the data fed to it through AddData.
        /// An MD5 hash is a way to condense a span of memory into a single 16 byte value.
        /// It's not a compression scheme, but rather the point is that the 16 bytes of data
        /// tend to be unique and so it can be used to fairly reliably validate that the span
        /// of memory is what it is. Instead of comparing two 10MB files byte-by-byte, you can
        /// be 99% sure they are the same if their MD5 hashes are equal.
        /// http://en.wikipedia.org/wiki/MD5
        ///
        /// Example usage:
        ///     MD5     md5;
        ///     uint8_t result[16];
        ///
        ///     md5.Begin();
        ///     md5.AddData("sdfuiwoernfgjksdfg", 18);
        ///     md5.AddData("ssfsssssssssd", 13);
        ///     md5.End(result);
        ///
        class MD5
        {
        public:  
            // Incremental calculation  
            void Begin();
            void AddData(const void* data, size_t dataLength);
            void End(HashValueMD5& hashValue);

            /// One-shot calculation that works on a single contiguous block of memory.
            static void GetHashValue(const void* data, size_t dataLength, HashValueMD5& hashValue);

            /// One-shot calculation that works on two blocks of memory (presumably disjoint).
            /// The order of the memory is significant.
            static void GetHashValue(const void* data1, size_t dataLength1, 
                                     const void* data2, size_t dataLength2, HashValueMD5& hashValue);

        protected:
            uint64_t mWorkData[16];
        };

    } // namespace Patch

} // namespace EA



#endif // Header include guard



 




