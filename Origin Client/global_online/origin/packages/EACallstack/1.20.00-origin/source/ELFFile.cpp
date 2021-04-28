///////////////////////////////////////////////////////////////////////////////
// ELFFile.cpp
//
// Copyright (c) 2006 Electronic Arts Inc.
// Created by Paul Schlegel, developed and maintained by Paul Pedriana.
//
// This is a minimal implementation of an ELF32/64 reader with just enough
// functionality for obtaining section information, the minimum functionality
// required by a DWARF2 reader.
//
// A good source of information about the ELF format can be found in the
// FreeBSD file formats manual:
//
//     http://www.freebsd.org/cgi/man.cgi?query=elf
//
///////////////////////////////////////////////////////////////////////////////


#include <EACallstack/ELFFile.h>
#include <EAIO/EAStreamAdapter.h>


namespace EA
{
namespace Callstack
{


///////////////////////////////////////////////////////////////////////////////
// ELFFile
//
ELFFile::ELFFile()
  : mbInitialized(false),
    mnClass(0),
    mEndian(EA::IO::kEndianBig),
    mpStream(NULL),
    mELFOffset(0),
    mHeader()
{
}


///////////////////////////////////////////////////////////////////////////////
// Init
//
bool ELFFile::Init(IO::IStream* pStream)
{
    if(!mbInitialized)
    {
        mpStream = pStream;

        if(pStream)
            mbInitialized = ReadFileHeader(&mHeader);
    }

    return mbInitialized;
}


///////////////////////////////////////////////////////////////////////////////
// GetSectionCount
//
int ELFFile::GetSectionCount()
{
    if(mbInitialized && mHeader.e_shoff && mHeader.e_shentsize)
        return mHeader.e_shnum;

    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// FindSectionHeaderByIndex
//
bool ELFFile::FindSectionHeaderByIndex(int nSectionIndex, ELF::ELF_Shdr* pSectionHeaderOut)
{
    const int nSectionCount = GetSectionCount(); // Will return zero if !mbInitialized

    if(nSectionIndex < nSectionCount)
    {
        const IO::off_type pos = (IO::off_type)(mELFOffset + mHeader.e_shoff + (mHeader.e_shentsize * nSectionIndex));

        if(mpStream->SetPosition(pos))
            return ReadSectionHeader(pSectionHeaderOut);
    }

    return false;
}


///////////////////////////////////////////////////////////////////////////////
// FindSectionHeaderByName
//
bool ELFFile::FindSectionHeaderByName(const char8_t* pSectionName, ELF::ELF_Shdr* pSectionHeaderOut)
{
    using namespace ELF;

    if(mbInitialized)
    {
        int      nNameSectionIndex;
        ELF_Shdr nameSectionHeader;

        // Look up the index of the section containing all section names.
        if(mHeader.e_shstrndx < SHN_LORESERVE)
        {
            nNameSectionIndex = mHeader.e_shstrndx;
        }
        else if(mHeader.e_shstrndx == SHN_XINDEX)
        {
            if(!FindSectionHeaderByIndex(0, pSectionHeaderOut))
                return false;

            nNameSectionIndex = (int)(int32_t)pSectionHeaderOut->sh_link;
        }
        else
            return false;

        // Get the name section header.
        if(FindSectionHeaderByIndex(nNameSectionIndex, &nameSectionHeader))
        {
            if(nameSectionHeader.sh_type != SHT_NOBITS)
            {
                // Find the requested section.
                const int nSectionCount = GetSectionCount();

                for(int i = 0; i < nSectionCount; ++i)
                {
                    if(FindSectionHeaderByIndex(i, pSectionHeaderOut))
                    {
                        const IO::off_type pos = (IO::off_type)(mELFOffset + nameSectionHeader.sh_offset + pSectionHeaderOut->sh_name);

                        if(mpStream->SetPosition(pos))
                        {
                            if(SectionNameMatches(pSectionName))
                                return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}


///////////////////////////////////////////////////////////////////////////////
// ReadFileHeader
//
bool ELFFile::ReadFileHeader(ELF::ELF_Ehdr* pFileHeaderOut)
{
    bool bReturnValue = false;

    using namespace EA::IO;
    using namespace ELF;

    if(mpStream && mpStream->SetPosition(0))
    {
        if(mpStream->Read(pFileHeaderOut->e_ident, sizeof(pFileHeaderOut->e_ident)) == sizeof(pFileHeaderOut->e_ident))
        {
            // Sony .self files (starting with the 190 SDK?) begin with a section that begins with 'SCE\0'.
            // Note that a .self file is a "signed ELF" file; it's used for security/DRM on the Playstation 3.
            if((pFileHeaderOut->e_ident[EI_MAG0] == 'S') &&
               (pFileHeaderOut->e_ident[EI_MAG1] == 'C') &&
               (pFileHeaderOut->e_ident[EI_MAG2] == 'E') &&
               (pFileHeaderOut->e_ident[EI_MAG3] ==  0))
            {
                // See the SCE_Hdr struct.
                uint64_t nSCEHeaderSize = 0;
                ReadUint64(mpStream, nSCEHeaderSize, mEndian);
                mELFOffset = (uint32_t)nSCEHeaderSize; // nSCEHeaderSize is a uint64_t but it will always be a small value.
                mpStream->SetPosition((IO::off_type)mELFOffset);
                mpStream->Read(pFileHeaderOut->e_ident, sizeof(pFileHeaderOut->e_ident));
            }

            // Some basic sanity checking of the file header.
            if((pFileHeaderOut->e_ident[EI_MAG0] == 0x7F) &&
               (pFileHeaderOut->e_ident[EI_MAG1] ==  'E') &&
               (pFileHeaderOut->e_ident[EI_MAG2] ==  'L') &&
               (pFileHeaderOut->e_ident[EI_MAG3] ==  'F'))
            {
                // Set the ELF class, 32- or 64-bit.
                mnClass = pFileHeaderOut->e_ident[EI_CLASS];
                mEndian = (pFileHeaderOut->e_ident[EI_DATA] == ELFDATA2MSB) ? kEndianBig : kEndianLittle;

                // EA_ASSERT((mnClass == ELFCLASS32) || (mnClass == ELFCLASS64));
                if(mnClass == ELFCLASS32)
                {
                    // Coerce the 32-bit file header into a 64-bit one.
                    uint32_t temp;

                    ReadUint16(mpStream, pFileHeaderOut->e_type,      mEndian);
                    ReadUint16(mpStream, pFileHeaderOut->e_machine,   mEndian);
                    ReadUint32(mpStream, pFileHeaderOut->e_version,   mEndian);
                    ReadUint32(mpStream, temp,                        mEndian);
                        pFileHeaderOut->e_entry = temp;
                    ReadUint32(mpStream, temp,                        mEndian);
                        pFileHeaderOut->e_phoff = temp;
                    ReadUint32(mpStream, temp,                        mEndian);
                        pFileHeaderOut->e_shoff = temp;
                    ReadUint32(mpStream, pFileHeaderOut->e_flags,     mEndian);
                    ReadUint16(mpStream, pFileHeaderOut->e_ehsize,    mEndian);
                    ReadUint16(mpStream, pFileHeaderOut->e_phentsize, mEndian);
                    ReadUint16(mpStream, pFileHeaderOut->e_phnum,     mEndian);
                    ReadUint16(mpStream, pFileHeaderOut->e_shentsize, mEndian);
                    ReadUint16(mpStream, pFileHeaderOut->e_shnum,     mEndian);
                    ReadUint16(mpStream, pFileHeaderOut->e_shstrndx,  mEndian);

                    bReturnValue = (mpStream->GetState() == kStateSuccess);
                }
                else if(mnClass == ELFCLASS64)
                {
                    ReadUint16(mpStream, pFileHeaderOut->e_type,      mEndian);
                    ReadUint16(mpStream, pFileHeaderOut->e_machine,   mEndian);
                    ReadUint32(mpStream, pFileHeaderOut->e_version,   mEndian);
                    ReadUint64(mpStream, pFileHeaderOut->e_entry,     mEndian);
                    ReadUint64(mpStream, pFileHeaderOut->e_phoff,     mEndian);
                    ReadUint64(mpStream, pFileHeaderOut->e_shoff,     mEndian);
                    ReadUint32(mpStream, pFileHeaderOut->e_flags,     mEndian);
                    ReadUint16(mpStream, pFileHeaderOut->e_ehsize,    mEndian);
                    ReadUint16(mpStream, pFileHeaderOut->e_phentsize, mEndian);
                    ReadUint16(mpStream, pFileHeaderOut->e_phnum,     mEndian);
                    ReadUint16(mpStream, pFileHeaderOut->e_shentsize, mEndian);
                    ReadUint16(mpStream, pFileHeaderOut->e_shnum,     mEndian);
                    ReadUint16(mpStream, pFileHeaderOut->e_shstrndx,  mEndian);

                    bReturnValue = (mpStream->GetState() == kStateSuccess);
                }
            }
        }
    }

    return bReturnValue;
}


///////////////////////////////////////////////////////////////////////////////
// ReadSectionHeader
//
bool ELFFile::ReadSectionHeader(ELF::ELF_Shdr* pSectionHeaderOut)
{
    using namespace ELF;

    bool bReturnValue = false;

    if(mbInitialized)
    {
        if(mnClass == ELFCLASS32)
        {
            ELF32_Shdr elf32Shdr;

            ReadUint32(mpStream, elf32Shdr.sh_name,      mEndian);
            ReadUint32(mpStream, elf32Shdr.sh_type,      mEndian);
            ReadUint32(mpStream, elf32Shdr.sh_flags,     mEndian);
            ReadUint32(mpStream, elf32Shdr.sh_addr,      mEndian);
            ReadUint32(mpStream, elf32Shdr.sh_offset,    mEndian);
            ReadUint32(mpStream, elf32Shdr.sh_size,      mEndian);
            ReadUint32(mpStream, elf32Shdr.sh_link,      mEndian);
            ReadUint32(mpStream, elf32Shdr.sh_info,      mEndian);
            ReadUint32(mpStream, elf32Shdr.sh_addralign, mEndian);
            ReadUint32(mpStream, elf32Shdr.sh_entsize,   mEndian);

            // Coerce the 32-bit section header into a 64-bit one.
            pSectionHeaderOut->sh_name      = elf32Shdr.sh_name;
            pSectionHeaderOut->sh_type      = elf32Shdr.sh_type;
            pSectionHeaderOut->sh_flags     = elf32Shdr.sh_flags;
            pSectionHeaderOut->sh_addr      = elf32Shdr.sh_addr;
            pSectionHeaderOut->sh_offset    = elf32Shdr.sh_offset;
            pSectionHeaderOut->sh_size      = elf32Shdr.sh_size;
            pSectionHeaderOut->sh_link      = elf32Shdr.sh_link;
            pSectionHeaderOut->sh_info      = elf32Shdr.sh_info;
            pSectionHeaderOut->sh_addralign = elf32Shdr.sh_addralign;
            pSectionHeaderOut->sh_entsize   = elf32Shdr.sh_entsize;

            bReturnValue = (mpStream->GetState() == IO::kStateSuccess);
        }
        else if(mnClass == ELFCLASS64)
        {
            ReadUint32(mpStream, pSectionHeaderOut->sh_name,      mEndian);
            ReadUint32(mpStream, pSectionHeaderOut->sh_type,      mEndian);
            ReadUint64(mpStream, pSectionHeaderOut->sh_flags,     mEndian);
            ReadUint64(mpStream, pSectionHeaderOut->sh_addr,      mEndian);
            ReadUint64(mpStream, pSectionHeaderOut->sh_offset,    mEndian);
            ReadUint64(mpStream, pSectionHeaderOut->sh_size,      mEndian);
            ReadUint32(mpStream, pSectionHeaderOut->sh_link,      mEndian);
            ReadUint32(mpStream, pSectionHeaderOut->sh_info,      mEndian);
            ReadUint64(mpStream, pSectionHeaderOut->sh_addralign, mEndian);
            ReadUint64(mpStream, pSectionHeaderOut->sh_entsize,   mEndian);

            bReturnValue = (mpStream->GetState() == IO::kStateSuccess);
        }
    }

    return bReturnValue;
}


///////////////////////////////////////////////////////////////////////////////
// SectionNameMatches
//
bool ELFFile::SectionNameMatches(const char8_t* pSectionName)
{
    #ifdef EA_DEBUG
        char  sectionName[128]; sectionName[0] = 0;
        char* p    = sectionName;
        char* pEnd = p + 127;

        while((p < pEnd) && mpStream->Read(p, 1) && *p)
            ++p;
        *p = 0;

        return (strcmp(sectionName, pSectionName) == 0);
    #else
      char8_t chIn;

      for(mpStream->Read(&chIn, 1); 
            chIn && *pSectionName; 
            mpStream->Read(&chIn, 1), pSectionName++)
        {
            if(chIn != *pSectionName) 
                return false;
        }

        return (chIn == *pSectionName);
    #endif
}



} // namespace Callstack
} // namespace EA















