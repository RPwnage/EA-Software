///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/Serialization.cpp
//
// Copyright (c) Electronic Arts. All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EAPatchClient/Base.h>
#include <EAPatchClient/Serialization.h>
#include <EAIO/EAFileStream.h>
#include <EAIO/EAStreamBuffer.h>
#include <EAIO/EAStreamChild.h>
#include <EAIO/EAStreamAdapter.h>
//

namespace EA
{
namespace Patch
{


SerializationBase::SerializationBase()
  : mpStream(NULL)
{
}


bool SerializationBase::ReadUint8Array(uint8_t* pArray, size_t arrayCount)
{
    if(mbSuccess)
        mbSuccess = EA::IO::ReadUint8(mpStream, pArray, arrayCount);
    return mbSuccess;
}

bool SerializationBase::WriteUint8Array(const uint8_t* pArray, size_t arrayCount)
{
    if(mbSuccess)
        mbSuccess = EA::IO::WriteUint8(mpStream, pArray, arrayCount);
    return mbSuccess;
}


bool SerializationBase::ReadUint32(uint32_t& value)
{
    if(mbSuccess)
        mbSuccess = EA::IO::ReadUint32(mpStream, value, EAPATCH_ENDIAN);
    return mbSuccess;
}

bool SerializationBase::WriteUint32(const uint32_t& value)
{
    if(mbSuccess)
        mbSuccess = EA::IO::WriteUint32(mpStream, value, EAPATCH_ENDIAN);
    return mbSuccess;
}


bool SerializationBase::ReadUint64(uint64_t& value)
{
    if(mbSuccess)
        mbSuccess = EA::IO::ReadUint64(mpStream, value, EAPATCH_ENDIAN);
    return mbSuccess;
}

bool SerializationBase::WriteUint64(const uint64_t& value)
{
    if(mbSuccess)
        mbSuccess = EA::IO::WriteUint64(mpStream, value, EAPATCH_ENDIAN);
    return mbSuccess;
}


namespace 
{
    // This is a copy of the version in EAIO but with a compiler warning fixed. The warning has been
    // fixed but we need this here for a year or so till the fix propogates to users.
    template <typename String8>
    inline bool ReadString8Local(EA::IO::IStream* pIS, String8& string, EA::IO::Endian endianDestination)
    {
        uint32_t length;

        if(EA::IO::ReadUint32(pIS, length, endianDestination))
        {
            string.resize(length);
            return EA::IO::ReadUint8(pIS, (uint8_t*)string.data(), length);
        }

        return false;
    }
}

bool SerializationBase::ReadString(String& value)
{
    if(mbSuccess)
        mbSuccess = ReadString8Local(mpStream, value, EAPATCH_ENDIAN); // Note we are using our version and not the EAIO version for the time being. See above for why.
    return mbSuccess;
}

bool SerializationBase::WriteString(const String& value)
{
    if(mbSuccess)
        mbSuccess = EA::IO::WriteString8(mpStream, value, EAPATCH_ENDIAN);
    return mbSuccess;
}


} // namespace Patch
} // namespace EA







