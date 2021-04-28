///////////////////////////////////////////////////////////////////////////////
// TokenBuffer.h
// 
// Copyright (c) 2009 Electronic Arts Inc.
// Written and maintained by Paul Pedriana.
// Based on PPMalloc's StackAllocator and Talin's variation of it for UTFXml.
//
// Implements a buffer for efficiently constructing Json encoded or decoded strings.
///////////////////////////////////////////////////////////////////////////////


#ifndef EAJSON_INTERNAL_TOKENBUFFER_H
#define EAJSON_INTERNAL_TOKENBUFFER_H


#include <EAJson/internal/Config.h>
#include <coreallocator/icoreallocator_interface.h>
#include <string.h>
#if EAJSON_DEV_ASSERT_ENABLED
    #include EA_ASSERT_HEADER
#endif


namespace EA
{
    namespace Json
    {
        //////////////////////////////////////////////////////////////////////////
        /// TokenBuffer
        ///
        /// A buffer that stores a series of incrementally-built tokens with
        /// a minimum of memory allocations (i.e. less than one allocation
        /// per token, since many tokens can share the same memory block.)
        //////////////////////////////////////////////////////////////////////////

        class EAJSON_API TokenBuffer
        {
        public:
            static const size_t kBlockSizeDefault = 256;

            TokenBuffer(EA::Allocator::ICoreAllocator* pAllocator = NULL, size_t blockSize = 0);
           ~TokenBuffer();

            EA::Allocator::ICoreAllocator* GetAllocator() const;
            void SetAllocator(EA::Allocator::ICoreAllocator* pAllocator);

            // Clear the buffer (keeps 1 block)
            void Clear();

            // Specify the block size for future buffer allocations.
            void SetBlockSize(size_t blockSize);

            // Returns the block size used for buffer allocations.
            size_t GetBlockSize() const;

            // Returns a pointer to the beginning of the token buffer.
            char8_t* Begin() const;

            // Returns a pointer to the current position pointer, which will
            // be one-past the last written character.
            char8_t* Current() const;

            // Returns a pointer to the last written character.
            char8_t* Last() const;

            // Returns a pointer to the beginning of the current UTF8 sequence.
            // This will be the same as Current for single-byte UTF8 sequences.
            const char8_t* CurrentUTF8() const;

            // Simply sets the pointer, doesn't reallocate and copy.
            void SetCurrentUTF8(const char8_t* pCurrentUTF8);

            // Append a single character to the buffer, encoded as UTF-8
            bool AppendEncodedChar(uint32_t c);

            // Append a byte (unencoded) to the buffer
            bool AppendByte(uint8_t b);

            // Append two bytes (unencoded) to the buffer
            bool AppendBytes(uint8_t b0, uint8_t b1);

            // Append three bytes (unencoded) to the buffer
            bool AppendBytes(uint8_t b0, uint8_t b1, uint8_t b2);

            // Append four bytes (unencoded) to the buffer
            bool AppendBytes(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3);

            // Append bytes (unencoded) to the buffer
            bool AppendBytes(const uint8_t* bytes, ssize_t nLen);

            // Expands capacity by nLen bytes; returns pointer to first byte of the expanded space.
            char8_t* AppendBytes(ssize_t nLen);

            // Removes the last byte, verifies it equals value, asserts if there is no last byte.
            bool RemoveVerifiedByte(char8_t value, bool bAssertOnFailure);

            // Removes the last byte, verifies it equals oldValue, replaces with new value, asserts if there is no last byte.
            bool ReplaceVerifiedByte(char8_t oldValue, char8_t newValue, bool bAssertOnFailure);

            // Replaces last byte with new value, asserts if there is no last byte.
            void ReplaceByte(char8_t newValue);

            // Removes bytes from the end.
            void RemoveBytes(ssize_t nLen);

            // Add a terminating NULL to the token and return its address.
            const char8_t* FinishToken();

            // Return the current token length. Not valid after FinishToken() is called.
            ssize_t TokenLength() const;

        protected:
            typedef EA::Allocator::ICoreAllocator ICoreAllocator;

            bool IncreaseCapacity(ssize_t additionalSize);

            struct Block
            {
                Block*      mpPrev;     // Blocks are in a growing singly-linked list.
                char8_t*    mpBegin;    // points to &mpBlock + sizeof(Block). In other words: points to "block memory here".
                char8_t*    mpEnd;      // points to &mpBlock + sizeof(Block) + block memory size.
                // Block memory here.
            };

            ICoreAllocator* mpAllocator;            // 
            Block*          mpCurrentBlock;         // Blocks are in a growing singly-linked list. mpCurrentBlock refers to the last one in the list; the one most recently added.
            char8_t*        mpCurrentBlockEnd;      // Equal to mpCurrentBlock->mpBlockEnd. To consider: this variable is redundant and serves only to give a modest speed increase.
            char8_t*        mpCurrentTokenBegin;    // Where within mpCurrentBlock the currently written string begins.
            char8_t*        mpCurrentTokenPos;      // Where within mpCurrentBlock the next byte of the currently written string will go.
            const char8_t*  mpCurrentUTF8;          // Pointer to location in token buffer where the beginning of the current UTF8 sequence begins. Used for UTF8 encoding validation. It will always be within the current block and always be 0-5 characters prior to mpCurrentTokenPos.
            size_t          mBlockSize;             // Size of the memory pointed to by mpCurrentBlock.
        };
    }
}


///////////////////////////////////////////////////////////////////////////////
// inlines
///////////////////////////////////////////////////////////////////////////////

namespace EA
{
    namespace Json
    {

        inline EA::Allocator::ICoreAllocator* TokenBuffer::GetAllocator() const
        {
            return mpAllocator;
        }

        inline void TokenBuffer::SetAllocator(EA::Allocator::ICoreAllocator* pAllocator)
        {
            mpAllocator = pAllocator;
        }

        inline void TokenBuffer::SetBlockSize(size_t blockSize)
        {
            mBlockSize = blockSize;
        }

        inline size_t TokenBuffer::GetBlockSize() const
        {
            return mBlockSize;
        }

        inline char8_t* TokenBuffer::Begin() const
        {
            return mpCurrentTokenBegin;
        }

        inline char8_t* TokenBuffer::Current() const
        {
            return mpCurrentTokenPos;
        }

        inline char8_t* TokenBuffer::Last() const
        {
            EAJSON_DEV_ASSERT((mpCurrentTokenPos - mpCurrentTokenBegin > 0)); // TokenLength() > 0

            return mpCurrentTokenPos - 1;
        }

        inline const char8_t* TokenBuffer::CurrentUTF8() const
        {
            return mpCurrentUTF8;
        }

        inline void TokenBuffer::SetCurrentUTF8(const char8_t* pCurrentUTF8)
        {
            mpCurrentUTF8 = pCurrentUTF8;
        }

        inline bool TokenBuffer::AppendByte(uint8_t b)
        {
            if(EA_UNLIKELY((mpCurrentTokenPos + 1) > mpCurrentBlockEnd))
            {
                if(!IncreaseCapacity(1))
                    return false;
            }

            *mpCurrentTokenPos++ = (char8_t)b;
            return true;
        }

        inline bool TokenBuffer::AppendBytes(uint8_t b0, uint8_t b1)
        {
            if(EA_UNLIKELY((mpCurrentTokenPos + 2) > mpCurrentBlockEnd))
            {
                if(!IncreaseCapacity(2))
                    return false;
            }

            *mpCurrentTokenPos++ = (char8_t)b0;
            *mpCurrentTokenPos++ = (char8_t)b1;
            return true;
        }

        inline bool TokenBuffer::AppendBytes(uint8_t b0, uint8_t b1, uint8_t b2)
        {
            if(EA_UNLIKELY((mpCurrentTokenPos + 3) > mpCurrentBlockEnd))
            {
                if(!IncreaseCapacity(3))
                    return false;
            }

            *mpCurrentTokenPos++ = (char8_t)b0;
            *mpCurrentTokenPos++ = (char8_t)b1;
            *mpCurrentTokenPos++ = (char8_t)b2;
            return true;
        }

        inline bool TokenBuffer::AppendBytes(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3)
        {
            if(EA_UNLIKELY((mpCurrentTokenPos + 4) > mpCurrentBlockEnd))
            {
                if(!IncreaseCapacity(4))
                    return false;
            }

            *mpCurrentTokenPos++ = (char8_t)b0;
            *mpCurrentTokenPos++ = (char8_t)b1;
            *mpCurrentTokenPos++ = (char8_t)b2;
            *mpCurrentTokenPos++ = (char8_t)b3;
            return true;
        }

        inline bool TokenBuffer::AppendBytes(const uint8_t* bytes, ssize_t nLen)
        {
            if(EA_UNLIKELY((mpCurrentTokenPos + nLen) > mpCurrentBlockEnd))
            {
                if(!IncreaseCapacity(nLen))
                    return false;
            }

            memcpy(mpCurrentTokenPos, bytes, (size_t)nLen);
            mpCurrentTokenPos += nLen;
            return true;
        }

        inline char8_t* TokenBuffer::AppendBytes(ssize_t nLen)
        {
            if(EA_UNLIKELY((mpCurrentTokenPos + nLen) > mpCurrentBlockEnd))
            {
                if(!IncreaseCapacity(nLen))
                    return NULL;
            }

            char8_t* pInitialPos = mpCurrentTokenPos;
            mpCurrentTokenPos += nLen;

            return pInitialPos;
        }

        inline void TokenBuffer::ReplaceByte(char8_t newValue)
        {
            EAJSON_DEV_ASSERT(((ssize_t)(mpCurrentTokenPos - mpCurrentTokenBegin) > 0)); // (TokenLength() > 0)

            mpCurrentTokenPos[-1] = newValue;
        }

        inline void TokenBuffer::RemoveBytes(ssize_t nLen)
        {
            EAJSON_DEV_ASSERT((mpCurrentTokenPos - nLen) >= mpCurrentTokenBegin);

            mpCurrentTokenPos -= nLen;
        }

        inline const char8_t* TokenBuffer::FinishToken()
        {
            AppendByte(0);

            const char8_t* pResult = mpCurrentTokenBegin;
            mpCurrentTokenBegin = mpCurrentTokenPos;

            return pResult;
        }

        inline ssize_t TokenBuffer::TokenLength() const
        {
            return (mpCurrentTokenPos - mpCurrentTokenBegin);
        }
    }
}

#endif // Header include guard

