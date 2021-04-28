/////////////////////////////////////////////////////////////////////////////
// EAStreamNull.h
//
// Copyright (c) 2007, Electronic Arts Inc. All rights reserved.
// Written by Paul Pedriana
/////////////////////////////////////////////////////////////////////////////


#if !defined(EAIO_EASTREAMNULL_H) && !defined(FOUNDATION_EASTREAMNULL_H)
#define EAIO_EASTREAMNULL_H
#define FOUNDATION_EASTREAMNULL_H // For backward compatability. Eventually, we'll want to get rid of this.


#include <EAIO/internal/Config.h>
#ifndef EAIO_EASTREAM_H
    #include <EAIO/EAStream.h>
#endif


namespace EA
{
    namespace IO
    {

        /// class StreamNull
        ///
        /// Implements a 'bit bucket' stream, whereby all writes to the stream
        /// succeed but do nothing and all reads from the stream succeed but write
        /// nothing to the user-supplied buffer.
        ///
        class EAIO_API StreamNull : public IStream
        {
        public:
            static const uint32_t kTypeStreamNull = 0x025c9bb3;

        public:
            StreamNull();
            StreamNull(const StreamNull& x);
            StreamNull& operator=(const StreamNull& x);

            // IStream functionality
            virtual int       AddRef();
            virtual int       Release();
            virtual uint32_t  GetType() const;
            virtual int       GetAccessFlags() const;
            virtual int       GetState() const;
            virtual bool      Close();
            virtual size_type GetSize() const;
            virtual bool      SetSize(size_type size);
            virtual off_type  GetPosition(PositionType positionType = kPositionTypeBegin) const;
            virtual bool      SetPosition(off_type position, PositionType positionType = kPositionTypeBegin);
            virtual size_type GetAvailable() const;
            virtual size_type Read(void* pData, size_type nSize);
            virtual bool      Flush();
            virtual bool      Write(const void* pData, size_type nSize);
 
        protected:
            int mnRefCount;

        }; // class StreamNull

    } // namespace IO

} // namespace EA



#endif // Header include guard













