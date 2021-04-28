///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/Serialization.h
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHCLIENT_SERIALIZATION_H
#define EAPATCHCLIENT_SERIALIZATION_H


#include <EAPatchClient/Base.h>
#include <EAPatchClient/Error.h>


#ifdef _MSC_VER
    #pragma warning(push)           // We have to be careful about disabling this warning. Sometimes the warning is meaningful; sometimes it isn't.
    #pragma warning(disable: 4251)  // class (some template) needs to have dll-interface to be used by clients.
    #pragma warning(disable: 4275)  // non dll-interface class used as base for dll-interface class.
#endif


namespace EA
{
    namespace IO
    {
        class IStream;
    }

    namespace Patch
    {
        /// SerializationBase
        ///
        /// Base class for serialization.
        ///
        class EAPATCHCLIENT_API SerializationBase : public ErrorBase
        {
        public:
            SerializationBase();

            // Binary serialization helpers.
            // We provide some helper read /write functions, for the purpose of making the 
            // serialization functions simpler and easier to read. The functions handle
            // the maintaining of error state.
            // Don't use these for writing XML, as that's a text format. 
            // To consider: Move these binary functions to a BinarySerializationBase subclass.
            bool ReadUint8Array(uint8_t* pArray, size_t arrayCount);
            bool WriteUint8Array(const uint8_t* pArray, size_t arrayCount);

            bool ReadUint32(uint32_t& value);
            bool WriteUint32(const uint32_t& value);

            bool ReadUint64(uint64_t& value);
            bool WriteUint64(const uint64_t& value);

            bool ReadString(String& value);
            bool WriteString(const String& value);

        protected:
            EA::IO::IStream* mpStream;  // The current stream being worked on.
        };

    } // namespace Patch

} // namespace EA


#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // Header include guard



 




