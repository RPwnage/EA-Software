#include <stdlib.h>
#include <stdio.h>
#include <QFile>
#include "zlib.h"
#include "zutil.h"

#ifndef ZLIB_INTERNAL
#define ZLIB_INTERNAL
#endif

#include "inftrees.h"
#include "inffixed.h"
#include "inflate.h"
#include "zlibDecompressor.h"
#include "../ZipFileInfo/MemBuffer.h"


namespace Origin
{
    namespace Downloader
    {
        // Borrowing the following data structure from ZipFileInfo.h which has dependencies I don't want...
        // i.e. ATL, MemoryBuffer, File, etc.
        class sPKFileHeader
        {
        public:
            sPKFileHeader()
            {
                memset(this, 0, sizeof(*this));
            }


            uint32_t signature;         //central file header signature   4 bytes  (0x04034b50)
            uint16_t versionNeeded;     //version needed to extract       2 bytes
            uint16_t bitFlags;          //general purpose bit flag        2 bytes
            uint16_t compressionMethod; //compression method              2 bytes
            uint16_t lastModTime;       //last mod file time              2 bytes
            uint16_t lastModDate;       //last mod file date              2 bytes
            uint32_t crc32;             //crc-32                          4 bytes
            uint32_t compressedSize;    //compressed size                 4 bytes
            uint32_t uncompressedSize;  //uncompressed size               4 bytes
            uint16_t filenameLength;    //file name length                2 bytes
            uint16_t extraFieldLength;  //extra field length              2 bytes

        };

        namespace ZLibDecompressor
        {

            // Stream version tags...... keep them all the same length for ease of reading
            const char* szStreamPersistTag_v1	= "stream_persist";  // version 1
            const char* szStreamPersistTag_v2	= "stream_0000002";  // version 2, adding bytes saved
            const char* szStreamPersistTag_v3	= "stream_0000003";  // version 3, fixing error with setting next pointer in state

            DeflateStreamDecompressor::DeflateStreamDecompressor()
            {
                mInitialized = false;
                mZStream = NULL;
                mDecompressionBuffer = NULL;
                mDecompressionBufferCapacity = 0;
                mBufferDecompressedBytes = 0;
                mTotalInputBytesProcessed = 0;
                mTotalOutputBytes = 0;
                mStatus = Z_OK;
                mFinalPass = false;
            }

            DeflateStreamDecompressor::~DeflateStreamDecompressor()
            {
                Shutdown();
            }

            const int32_t kDecompressBuffer = 256*1024; // 256KB Decompression Buffer

            int32_t DeflateStreamDecompressor::Init()
            {
                if(!mInitialized)
                {
                    mZStream = (z_stream*)(malloc(sizeof(z_stream)));

                    mZStream->zalloc = Z_NULL;// (alloc_func) malloc;
                    mZStream->zfree = Z_NULL; // (free_func) free;
                    mZStream->opaque = Z_NULL;
                    mZStream->next_in = Z_NULL;
                    mZStream->avail_in = 0;

                    //Since we are using an old version of ZLib, the library lacks support of transparent initialization of such stream.
                    //In newer versions, one can call inflateInit2(mZStream,-MAX_WBITS+32); to do this.
                    //We expect the stream header to be removed at this point.
                    mStatus = inflateInit2(mZStream,-MAX_WBITS);

                    if(mStatus == Z_OK)
                    {
                        mInitialized = true;
                        mCheckForStreamTypeError = true;
                        mBufferDecompressedBytes = 0;
                        mTotalInputBytesProcessed = 0;
                        mTotalOutputBytes = 0;
                        mFinalPass = false;
                    }

                    AllocateDecompressionBuffer(kDecompressBuffer);
                }

                return mStatus;
            }


            int32_t DeflateStreamDecompressor::Shutdown()
            {
                if (mDecompressionBuffer)
                {
                    ZFREE(mZStream, mDecompressionBuffer);
                    mDecompressionBuffer = NULL;
                }
                mDecompressionBufferCapacity = 0;
                mBufferDecompressedBytes = 0;
                mTotalInputBytesProcessed = 0;
                mTotalOutputBytes = 0;

                inflateEnd(mZStream);       // free stream memory
                free(mZStream);
                mZStream = NULL;

                mInitialized = false;
                mFinalPass = false;

                return Z_OK;
            }

            int32_t DeflateStreamDecompressor::SaveStream(const QString& pStreamPersistFilename)
            {
                int32_t status = Z_OK;

                struct inflate_state FAR *state;

                // Create file
                QFile file(pStreamPersistFilename);
                if (!file.open(QIODevice::WriteOnly))
                    return Z_ERRNO;


                file.write(szStreamPersistTag_v3, strlen(szStreamPersistTag_v3));		// store a tag for the file

                file.write((char*)&mTotalInputBytesProcessed, sizeof(mTotalInputBytesProcessed));      // store the total number of input bytes processed
                file.write((char*)&mTotalOutputBytes, sizeof(mTotalOutputBytes));						// store the total number of output bytes

                if (mZStream && mZStream->state && mZStream->zalloc != (alloc_func) 0 && mZStream->zfree != (free_func) 0)
                {
                    //state = (struct inflate_state FAR *)source->state;
                    state = (struct inflate_state FAR *) mZStream->state;

                    file.write((char*)state, sizeof(struct inflate_state));

                    uint32_t lencodeOffset = (uint32_t) (state->lencode - state->codes);
                    uint32_t distcodeOffset = (uint32_t) (state->distcode - state->codes);
                    uint32_t nextOffset = (uint32_t) (state->next - state->codes);

                    uint8_t usingFixedTable = 0;
                    // Check whether lencode is indexed into the fixed table or not
                    if (lencodeOffset > ENOUGH)
                    {
                        usingFixedTable = 1;
                        lencodeOffset = (uint32_t) (state->lencode - lenfix);
                        distcodeOffset = (uint32_t) (state->distcode - distfix);
                    }

                    file.write((char*)&lencodeOffset, sizeof(uint32_t));
                    file.write((char*)&distcodeOffset, sizeof(uint32_t));
                    file.write((char*)&nextOffset, sizeof(uint32_t));
                    file.write((char*)&usingFixedTable, sizeof(uint8_t));
                    file.write((char*)&state->lenbits, sizeof(state->lenbits));
                    file.write((char*)&state->distbits, sizeof(state->distbits));


                    if (state->window != Z_NULL)
                    {
                        uint32_t nWSize = 1U << state->wbits;
                        file.write((char*)(state->window), nWSize);
                    }


                    file.write((char*)&mZStream->total_in, sizeof(uint32_t));     // AZ: Trying to persist this specifically
                    file.write((char*)&mZStream->total_out, sizeof(uint32_t));     // AZ: Trying to persist this specifically
                    file.write((char*)&mZStream->data_type, sizeof(uint32_t));     // AZ: Trying to persist this specifically
                    file.write((char*)&mZStream->opaque, sizeof(uint32_t));     // AZ: Trying to persist this specifically
                    file.write((char*)&mZStream->adler, sizeof(uint32_t));     // AZ: Trying to persist this specifically

                    status = Z_OK;
                }
                else
                {
                    status = Z_STREAM_ERROR;
                }

                file.close();
                return status;
            }

            int32_t DeflateStreamDecompressor::ResumeStream(const QString& pStreamPersistFilename)
            {
                // Create file
                qint64 nNumRead = 0;
                int32_t nReturnCode = Z_OK;

                QFile file(pStreamPersistFilename);
                if (!file.open(QIODevice::ReadOnly))
                {
                    nReturnCode = Z_ERRNO;
                    goto resume_stream_exit;
                }

                Shutdown();
                Init();

                if (mZStream == NULL)
                {
                    nReturnCode = Z_STREAM_ERROR;
                    goto resume_stream_exit;
                }

                //state = (struct inflate_state FAR *)source->state;


                char streamTagBuffer[32];
                memset(streamTagBuffer, 0, sizeof(streamTagBuffer));
                nNumRead = file.read((char*)&streamTagBuffer[0], strlen(szStreamPersistTag_v1));
                if (nNumRead != strlen(szStreamPersistTag_v1))
                {
                    // Can't read from file or string tag is missing
                    nReturnCode = Z_ERRNO;
                    goto resume_stream_exit;
                }

                // Compare the stream tag to determine which read function to call
                if (strcmp(streamTagBuffer, szStreamPersistTag_v1) == 0)
                {
                    nReturnCode = ReadStreamV1(file);
                }
                else if (strcmp(streamTagBuffer, szStreamPersistTag_v2) == 0)
                {
                    nReturnCode = ReadStreamV2(file);
                }
                else if (strcmp(streamTagBuffer, szStreamPersistTag_v3) == 0)
                {
                    nReturnCode = ReadStreamV3(file);
                }
                else
                {
                    nReturnCode = Z_ERRNO;
                }

resume_stream_exit:
                return nReturnCode;
            }


            int32_t DeflateStreamDecompressor::ReadStreamV1(QFile& file)
            {
                ulong nNumRead = 0;
                nNumRead = file.read((char*)&mTotalInputBytesProcessed, sizeof(mTotalInputBytesProcessed));      // the total number of input bytes processed

                /* allocate space */
                struct inflate_state FAR *copy;
                unsigned char FAR *window;
                copy = (struct inflate_state FAR *) ZALLOC(mZStream, 1, sizeof(struct inflate_state));
                if (copy == Z_NULL)
                    return Z_MEM_ERROR;


                file.read((char*)copy, sizeof(struct inflate_state));


                uint32_t lencodeOffset = 0;
                uint32_t distcodeOffset = 0;

                file.read((char*)&lencodeOffset, sizeof(uint32_t));
                file.read((char*)&distcodeOffset, sizeof(uint32_t));

                window = Z_NULL;

                if (copy->window != Z_NULL)
                {
                    uint32_t nWSize = 1U << copy->wbits;
                    window = (unsigned char FAR *) ZALLOC(mZStream, 1, nWSize);
                    file.read((char*)window, nWSize);
                }


                copy->lencode = copy->codes + lencodeOffset;
                copy->distcode = copy->codes + distcodeOffset;


                copy->next = copy->codes + (copy->next - copy->codes);

                copy->window = window;

                mZStream->state = (struct internal_state FAR *) copy;

                file.read((char*)&mZStream->total_in, sizeof(uint32_t));     // AZ: Trying to persist this specifically
                file.read((char*)&mZStream->total_out, sizeof(uint32_t));     // AZ: Trying to persist this specifically
                file.read((char*)&mZStream->data_type, sizeof(uint32_t));     // AZ: Trying to persist this specifically
                file.read((char*)&mZStream->opaque, sizeof(uint32_t));     // AZ: Trying to persist this specifically
                file.read((char*)&mZStream->adler, sizeof(uint32_t));     // AZ: Trying to persist this specifically

                return Z_OK;
            }

            int32_t DeflateStreamDecompressor::ReadStreamV2(QFile& file)
            {
                file.read((char*)&mTotalInputBytesProcessed, sizeof(mTotalInputBytesProcessed));      // the total number of input bytes processed
                file.read((char*)&mTotalOutputBytes, sizeof(mTotalOutputBytes));      // the total number of input bytes processed

                /* allocate space */
                struct inflate_state FAR *copy;
                unsigned char FAR *window;
                copy = (struct inflate_state FAR *) ZALLOC(mZStream, 1, sizeof(struct inflate_state));
                if (copy == Z_NULL)
                    return Z_MEM_ERROR;


                file.read((char*)copy, sizeof(struct inflate_state));


                uint32_t lencodeOffset = 0;
                uint32_t distcodeOffset = 0;

                file.read((char*)&lencodeOffset, sizeof(uint32_t));
                file.read((char*)&distcodeOffset, sizeof(uint32_t));

                window = Z_NULL;

                if (copy->window != Z_NULL)
                {
                    uint32_t nWSize = 1U << copy->wbits;
                    window = (unsigned char FAR *) ZALLOC(mZStream, 1, nWSize);
                    file.read((char*)window, nWSize);
                }


                copy->lencode = copy->codes + lencodeOffset;
                copy->distcode = copy->codes + distcodeOffset;


                copy->next = copy->codes + (copy->next - copy->codes);

                copy->window = window;

                mZStream->state = (struct internal_state FAR *) copy;

                file.read((char*)&mZStream->total_in, sizeof(uint32_t));     // AZ: Trying to persist this specifically
                file.read((char*)&mZStream->total_out, sizeof(uint32_t));     // AZ: Trying to persist this specifically
                file.read((char*)&mZStream->data_type, sizeof(uint32_t));     // AZ: Trying to persist this specifically
                file.read((char*)&mZStream->opaque, sizeof(uint32_t));     // AZ: Trying to persist this specifically
                file.read((char*)&mZStream->adler, sizeof(uint32_t));     // AZ: Trying to persist this specifically

                return Z_OK;
            }

            int32_t DeflateStreamDecompressor::ReadStreamV3(QFile& file)
            {
                file.read((char*)&mTotalInputBytesProcessed, sizeof(mTotalInputBytesProcessed));      // the total number of input bytes processed
                file.read((char*)&mTotalOutputBytes, sizeof(mTotalOutputBytes));      // the total number of input bytes processed

                /* allocate space */
                struct inflate_state FAR *copy;
                unsigned char FAR *window;
                copy = (struct inflate_state FAR *) ZALLOC(mZStream, 1, sizeof(struct inflate_state));
                if (copy == Z_NULL)
                    return Z_MEM_ERROR;


                file.read((char*)copy, sizeof(struct inflate_state));


                uint32_t lencodeOffset = 0;
                uint32_t distcodeOffset = 0;
                uint32_t nextOffset = 0;
                uint8_t usingFixedTable = 0;

                file.read((char*)&lencodeOffset, sizeof(uint32_t));
                file.read((char*)&distcodeOffset, sizeof(uint32_t));
                file.read((char*)&nextOffset, sizeof(uint32_t));
                file.read((char*)&usingFixedTable, sizeof(uint8_t));
                file.read((char*)&copy->lenbits, sizeof(copy->lenbits));
                file.read((char*)&copy->distbits, sizeof(copy->distbits));

                // These values should be in range to set up the state properly
                if (usingFixedTable > 0)
                {
                    copy->lencode = lenfix + lencodeOffset;
                    copy->distcode = distfix + distcodeOffset;
                }
                else
                {
                    copy->lencode = copy->codes + lencodeOffset;
                    copy->distcode = copy->codes + distcodeOffset;
                }

                copy->next = copy->codes + nextOffset;


                window = Z_NULL;

                if (copy->window != Z_NULL)
                {
                    uint32_t nWSize = 1U << copy->wbits;
                    window = (unsigned char FAR *) ZALLOC(mZStream, 1, nWSize);
                    file.read((char*)window, nWSize);
                }

                copy->window = window;

                ZFREE(mZStream, mZStream->state);
                mZStream->state = (struct internal_state FAR *) copy;

                file.read((char*)&mZStream->total_in, sizeof(uint32_t));     // AZ: Trying to persist this specifically
                file.read((char*)&mZStream->total_out, sizeof(uint32_t));     // AZ: Trying to persist this specifically
                file.read((char*)&mZStream->data_type, sizeof(uint32_t));     // AZ: Trying to persist this specifically
                file.read((char*)&mZStream->opaque, sizeof(uint32_t));     // AZ: Trying to persist this specifically
                file.read((char*)&mZStream->adler, sizeof(uint32_t));     // AZ: Trying to persist this specifically

                return Z_OK;
            }

            int32_t DeflateStreamDecompressor::StartBuffer(uint8_t* sourceStream, int32_t sourceLength)
            {
                if (sourceStream == NULL || sourceLength < 0)
                {
                    return Z_ERRNO;
                }

                //If not already initialized, initialize at the first attempt to decompress.
                if(!mInitialized)
                {
                    mStatus = Init();
                    if(mStatus != Z_OK)
                        return mStatus;
                }

                mFinalPass = false;
                mZStream->next_in = (Bytef*)sourceStream;
                mZStream->avail_in = sourceLength;

                return Z_OK;
            }

            int32_t DeflateStreamDecompressor::Decompress()
            {

                //If not already initialized, initialize at the first attempt to decompress.
                if(!mInitialized)
                {
                    mStatus = Z_ERRNO;
                    return mStatus;
                }

                mBufferDecompressedBytes = 0;

                //int32_t totalBytesDecompressed = 0;

                if (mZStream->avail_in > 0 || mFinalPass)
                {
                    mZStream->next_out = (Bytef*)(mDecompressionBuffer);
                    mZStream->avail_out = (uInt) mDecompressionBufferCapacity; // the bytes remaining in the buffer

                    struct inflate_state FAR *state = (inflate_state FAR *) mZStream->state;        // for debugging
                    Q_UNUSED(state);

                    Bytef* pNextOutBeforeInflate = mZStream->next_out;
                    Bytef* pNextInBeforeInflate = mZStream->next_in;

                    mStatus = inflate(mZStream, Z_SYNC_FLUSH);

                    // If we're passed stream data with a header we need to process it, re-init and try inflate again
                    if (mStatus == Z_DATA_ERROR)
                    {
                        // Could be a header error..... 
                        int32_t headerBytesProcessed = 0;
                        mStatus = ProcessPKZipHeader(mZStream->next_in, mZStream->avail_in, headerBytesProcessed);
                        if(mStatus != Z_OK)
                        {
                            // Error processing PKZip Header
                            return mStatus;
                        }

                        mStatus = inflateInit2(mZStream,-MAX_WBITS);

                        // Now that the header is processed, try decompression again
                        mZStream->next_in   += headerBytesProcessed;
                        mZStream->avail_in  -= headerBytesProcessed;

                        mStatus = inflate(mZStream, Z_SYNC_FLUSH);
                    }

                    Bytef* pNextOutAfterInflate = mZStream->next_out;
                    Bytef* pNextInAfterInflate = mZStream->next_in;

                    int32_t bytesProcessed =    (int32_t) (pNextInAfterInflate  - pNextInBeforeInflate);
                    int32_t bytesDecompressed = (int32_t) (pNextOutAfterInflate - pNextOutBeforeInflate);

                    // Advance the counters
                    mBufferDecompressedBytes = bytesDecompressed;
                    mTotalInputBytesProcessed += bytesProcessed;
                    mTotalOutputBytes += bytesDecompressed;

                    // Make sure we grab all the data if we've exhausted our input and output buffers (there may be more)
                    if (mStatus == Z_OK && mZStream->avail_in == 0 && mZStream->avail_out == 0)
                    {
                        mFinalPass = true;
                    }

                    if (mStatus == Z_STREAM_END)
                    {
                        // End of file
                        inflateEnd(mZStream);
                        return mStatus;
                    }

                    if(mStatus < 0)
                    {
                        // Error status
                        if (mStatus != Z_BUF_ERROR)
                            inflateEnd(mZStream);
                        //else
                        //    mStatus = Z_OK; // Explicitly allow Z_BUF_ERROR because we either have more data coming, or we will catch this error via other means
                    }
                }

                return mStatus;
            }

            bool DeflateStreamDecompressor::HasMoreOutput()
            {
                if (!mZStream)
                    return false;

                // If there is more input data but no more output buffer
                if (mZStream->avail_in > 0)
                    return true;

                // if there is no more data coming in, the
                return mStatus == Z_OK && mBufferDecompressedBytes > 0;
            }

            bool DeflateStreamDecompressor::NeedsMoreInput()
            {
                if (!mZStream)
                    return false;

                return mStatus == Z_OK;
            }


            void DeflateStreamDecompressor::FreeDecompressionBuffer()
            {
                // Free the decompression buffer (we will re-allocate a new one if needed)
                if (mDecompressionBuffer)
                {
                    ZFREE(mZStream, mDecompressionBuffer);
                    mDecompressionBuffer = NULL;
                }
                mDecompressionBufferCapacity = 0;
            }

            int32_t DeflateStreamDecompressor::ProcessPKZipHeader(uint8_t* sourceStream, int32_t sourceLength, int32_t& inputBytesProcessed)
            {
                if (static_cast<size_t>(sourceLength) < sizeof(sPKFileHeader))
                    return Z_ERRNO;

                // If the pkzip header doesn't start with "PK" this isn't the correct header
                if (memcmp((void*) sourceStream, "PK", 2) != 0)
                    return Z_ERRNO;

                sPKFileHeader fileHeader;
                MemBuffer memBuf(sourceStream, sourceLength);
                memBuf.SetByteOrder( MemBuffer::LittleEndian );

                fileHeader.signature = memBuf.getULong();			//central file header signature   4 bytes  (0x04034b50)
                fileHeader.versionNeeded = memBuf.getUShort();		//version needed to extract       2 bytes
                fileHeader.bitFlags = memBuf.getUShort();			//general purpose bit flag        2 bytes
                fileHeader.compressionMethod = memBuf.getUShort();	//compression method              2 bytes
                fileHeader.lastModTime = memBuf.getUShort();		//last mod file time              2 bytes
                fileHeader.lastModDate = memBuf.getUShort();		//last mod file date              2 bytes
                fileHeader.crc32 = memBuf.getULong();				//crc-32                          4 bytes
                fileHeader.compressedSize = memBuf.getULong();		//compressed size                 4 bytes
                fileHeader.uncompressedSize = memBuf.getULong();    //uncompressed size               4 bytes
                fileHeader.filenameLength = memBuf.getUShort();		//file name length                2 bytes
                fileHeader.extraFieldLength = memBuf.getUShort();	//extra field length              2 bytes

                inputBytesProcessed = 30 + fileHeader.filenameLength + fileHeader.extraFieldLength;
                return Z_OK;
            }

            void DeflateStreamDecompressor::AllocateDecompressionBuffer( int32_t amountToAllocate )
            {
                uint8_t* pNew = (uint8_t*) ZALLOC(mZStream, 1, mDecompressionBufferCapacity + amountToAllocate);
                memcpy((void*) pNew, mDecompressionBuffer, mDecompressionBufferCapacity);
                ZFREE(mZStream, mDecompressionBuffer);
                mDecompressionBuffer = pNew;
                mDecompressionBufferCapacity += amountToAllocate;
            }
        } // Namespace ZLibDecompressor

    } // namespace Downloader
} // namespace Origin

