/*! *********************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef EA_TDF_BLOB_H
#define EA_TDF_BLOB_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/************* Include files ***************************************************************************/
#include <EATDF/internal/config.h>
#include <EATDF/typedescription.h>
#include <EATDF/tdfobject.h>
#include <EATDF/tdffactory.h>

/************ Defines/Macros/Constants/Typedefs *******************************************************/

namespace EA
{

namespace TDF
{

/*! ***************************************************************/
/*! \class TdfBlob
    \brief Simple blob class used by the tdf types to carry variable length binary data.
*******************************************************************/
class EATDF_API TdfBlob : public TdfObject
#if EA_TDF_INCLUDE_CHANGE_TRACKING
    ,public TdfChangeTracker
#endif
{
public:
    const TypeDescription& getTypeDescription() const override;

    static const char8_t* DEFAULT_BLOB_ALLOCATION_NAME;

    /*! \brief Construct an empty blob. */
    TdfBlob(EA::Allocator::ICoreAllocator& allocator EA_TDF_DEFAULT_ALLOCATOR_ASSIGN, const char8_t* debugMemName = EA_TDF_DEFAULT_ALLOC_NAME);

    /*! \brief Copy constructor. */
    TdfBlob(const TdfBlob& rhs, const char8_t* allocationName = DEFAULT_BLOB_ALLOCATION_NAME);

#if EA_TDF_INCLUDE_CHANGE_TRACKING
    ~TdfBlob() override { release(); }
#else
    ~TdfBlob() { release(); }
#endif

    /*! \brief Get pointer to the head of data. */
    uint8_t* getData() const { return mData; }

    /*! ***************************************************************/
    /*! \brief Set the given length new data for this blob. Data is copied into the blob's internal buffer.

        \param[in] value the raw data to be set
        \param[in] len size of the raw data passed in
        \param[in] allocatioName If any allocations are required, they are tagged with this name from the mem system.
        \return    flag to show if the init/update succeeds
    ********************************************************************/
    bool setData(const uint8_t* value, TdfSizeVal len, const char8_t* allocationName = DEFAULT_BLOB_ALLOCATION_NAME);

    /*! ***************************************************************/
    /*! \brief Set the given length new data for this blob. Data is not copied but merely referred to.  Data must
               remain until the blob is destroyed or new data is set.

        \param[in] value the raw data to be set
        \param[in] len size of the raw data passed in
        \return    flag to show if the init/update succeeds
    ********************************************************************/
    bool assignData(uint8_t* value, TdfSizeVal len);

    /*! \brief Get size of blob. */
    TdfSizeVal getSize() const { return mSize; }

    /*! \brief Get number of bytes in use. */
    TdfSizeVal getCount() const { return mCount; }

    /*! \brief Set number of bytes in use. */
    void setCount(TdfSizeVal count) { mCount = count; }

    /*! ***************************************************************/
    /*! \brief realloc the blob using given size.  The contents of the
         blob may be preserved when (newSize >= mCount)

        \param[in] newSize size of the new blob
        \param[in] toPreserve flag to indicate if needs to keep the old blob data (only work if newSize > mCount)
        \return    flag to show if resize succeeds
    *******************************************************************/
    bool resize(TdfSizeVal newSize, bool toPreserve = false, const char8_t* allocationName = DEFAULT_BLOB_ALLOCATION_NAME);

    // clone creates a copy of the this object, using the supplied allocator
    TdfBlob* clone(EA::Allocator::ICoreAllocator& allocator EA_TDF_DEFAULT_ALLOCATOR_ASSIGN, const char8_t* allocationName = DEFAULT_BLOB_ALLOCATION_NAME) const;

    void copyInto(TdfBlob& lhsBlob, const char8_t* allocationName = DEFAULT_BLOB_ALLOCATION_NAME) const { copyInto(lhsBlob, MemberVisitOptions(), allocationName); }
    void copyIntoObject(TdfObject& lhsBlob, const MemberVisitOptions& options) const override { copyInto(static_cast<TdfBlob&>(lhsBlob), options, DEFAULT_BLOB_ALLOCATION_NAME); }
    void copyInto(TdfBlob& lhsBlob, const MemberVisitOptions& options, const char8_t* allocationName = DEFAULT_BLOB_ALLOCATION_NAME) const;
    
    bool operator==(const TdfBlob& rhs) const;
    bool operator<(const TdfBlob& rhs) const;
    TdfBlob& operator=(const TdfBlob& other);

    EA::Allocator::ICoreAllocator& getAllocator() const { return mAllocator; }

    /*************************************************************************************************/
    /*!
        \brief encode

        Encode the blob's binary data into base 64 format and output the encoded data into the
        char buffer.

        \param[in] the output buffer
        \param[in] the size of output buffer (only work if outputBufSize >= calcBase64EncodedSize() + 1, and the 1 extra space is for \0)
        \return - The number of characters written to the char buffer, not including the null
                  character; 0 on empty blob; -1 on error or overflow
    */
    /*************************************************************************************************/
    int32_t encodeBase64(char8_t* output, size_t outputBufSize) const;
    static int32_t encodeBase64(const char8_t* input, TdfSizeVal inputBufSize, char8_t* output, size_t outputBufSize);

    /*************************************************************************************************/
    /*!
        \brief decode

        Decode base64 encoded data into the blob discarding padding, line breaks and noise.

        \param[in] the input buffer
        \param[in] the number of bytes need to be decoded
        \return - The number of characters written to the output buffer, not including the null
        character; -1 on error
    */
    /*************************************************************************************************/
    int32_t decodeBase64(const char8_t* input, TdfSizeVal inputBufSize);
    static int32_t decodeBase64(const char8_t* input, TdfSizeVal inputBufSize, char8_t** output, size_t* outputBufSize);

    // Calculate the size of the encoded blob 
    size_t calcBase64EncodedSize() const;
    static size_t calcBase64EncodedSize(TdfSizeVal inputBufSize);

#if EA_TDF_REGISTER_ALL
    static TypeRegistrationNode REGISTRATION_NODE;
#endif

protected:
    void release();
    static void encodeBase64Block(uint8_t in[3], char8_t* out, TdfSizeVal len);
    static void decodeBase64Block(uint8_t in[4], uint8_t* out);

    TdfSizeVal mSize;       //!< Total size of the blob
    uint8_t* mData;         //!< Pointer to the data
    TdfSizeVal mCount;      //!< Number of bytes currently in use
    bool mOwnsMem;          //!< Whether we own the memory
    EA::Allocator::ICoreAllocator& mAllocator;
    
};

typedef tdf_ptr<TdfBlob> TdfBlobPtr;

template <>
struct TypeDescriptionSelector<TdfBlob >
{
    static const TypeDescriptionObject& get() { 
        static TypeDescriptionObject result(TDF_ACTUAL_TYPE_BLOB, TDF_ACTUAL_TYPE_BLOB, "blob", (EA::TDF::TypeDescriptionObject::CreateFunc)  &TdfObject::createInstance<TdfBlob>); 
        return result;
    }
};

template <>
struct TdfPrimitiveAllocHelper<TdfBlob>
{
    TdfBlob operator()(const TdfBlob& value, EA::Allocator::ICoreAllocator& allocator) const
    {
        TdfBlob blobWithAllocator(allocator);
        blobWithAllocator = value;
        return blobWithAllocator;
    }
    TdfBlob operator()(EA::Allocator::ICoreAllocator& allocator) const
    {
        return TdfBlob(allocator);
    }
};

} //namespace TDF

} //namespace EA


namespace EA
{
    namespace Allocator
    {
        namespace detail
        {
            template <>
            inline void DeleteObject(Allocator::ICoreAllocator*, ::EA::TDF::TdfBlob* object) { delete object; }
        }
    }
}

#endif // EA_TDF_H

