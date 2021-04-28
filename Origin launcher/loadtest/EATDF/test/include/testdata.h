/*! *********************************************************************************************/
/*!
    \file 


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/
#ifndef EA_TDF_TEST_DATA_H
#define EA_TDF_TEST_DATA_H

#include <test.h>
#include <EATDF/tdfenuminfo.h>
#include <test/tdf/alltypes.h>

// Defaults
const uint32_t MAP_INT_KEY = 123;
extern const char8_t* MAP_STRING_KEY;

const bool     BOOL_VAL   = true;
const uint8_t  UINT8_VAL  = 123;
const uint16_t UINT16_VAL = 1234;
const uint32_t UINT32_VAL = 123456;
const uint64_t UINT64_VAL = 12345678901;
const int8_t   INT8_VAL   = -123;
const int16_t  INT16_VAL  = -1234;
const int32_t  INT32_VAL  = -123456;
const int64_t  INT64_VAL  = -12345678901;
const float    FLOAT_VAL  = 123.4f;
extern const char8_t* STRING_VAL;

extern const EA::TDF::TimeValue TIME_VAL;
extern const EA::TDF::ObjectType OBJECT_TYPE_VAL;
extern const EA::TDF::ObjectId OBJECT_ID_VAL;

extern const uint8_t BLOB_VAL[];
extern const uint8_t BLOB_VAL_SIZE;
const uint32_t BITFIELD_VAL = 0x2;

const size_t ENUM_INDEX = 1;

// Alternates
const uint32_t MAP_INT_KEY2 = 789;
extern const char8_t* MAP_STRING_KEY2;

const bool     BOOL_VAL2 = false;
const uint8_t  UINT8_VAL2  = 234;
const uint16_t UINT16_VAL2 = 7890;
const uint32_t UINT32_VAL2 = 567890;
const uint64_t UINT64_VAL2 = 56789012345;
const int8_t   INT8_VAL2   = -78;
const int16_t  INT16_VAL2  = -7890;
const int32_t  INT32_VAL2  = -567890;
const int64_t  INT64_VAL2  = -56789012345;
const float    FLOAT_VAL2  = 567.8f;
extern const char8_t* STRING_VAL2;

extern const EA::TDF::TimeValue TIME_VAL2;
extern const EA::TDF::ObjectType OBJECT_TYPE_VAL2;
extern const EA::TDF::ObjectId OBJECT_ID_VAL2;

extern const uint8_t BLOB_VAL2[];
extern const uint8_t BLOB_VAL_SIZE2;
const uint32_t BITFIELD_VAL2 = 0x1;

const size_t ENUM_INDEX2 = 2;

struct TestData
{
    TestData(bool boolVal = ::BOOL_VAL, uint8_t uint8Val = ::UINT8_VAL, uint16_t uint16Val = ::UINT16_VAL, uint32_t uint32Val = ::UINT32_VAL, uint64_t uint64Val = ::UINT64_VAL,
             int8_t int8Val = ::INT8_VAL, int16_t int16Val = ::INT16_VAL, int32_t int32Val = ::INT32_VAL, int64_t int64Val = ::INT64_VAL, size_t enumIndex = ::ENUM_INDEX, 
             float floatVal = ::FLOAT_VAL, const char8_t* stringVal = ::STRING_VAL, uint32_t mapIntKey = ::MAP_INT_KEY, const char8_t* mapStringKey = ::MAP_STRING_KEY, 
             const EA::TDF::TimeValue& timeVal = ::TIME_VAL, const EA::TDF::ObjectType& objVal = ::OBJECT_TYPE_VAL, const EA::TDF::ObjectId& objId = ::OBJECT_ID_VAL, 
             const uint8_t* blobVal = ::BLOB_VAL, uint8_t blobValSize = ::BLOB_VAL_SIZE, uint32_t bitFieldVal = ::BITFIELD_VAL, uint32_t unionIndex = 1, 
             EA::TDF::TdfId testNonDefaultCopyTdfId = EA::TDF::Test::AllPrimitivesClass::TDF_ID, uint32_t testNonDefaultCopyIndex = 2);

    bool BOOL_VAL;
    uint8_t  UINT8_VAL;
    uint16_t UINT16_VAL;
    uint32_t UINT32_VAL;
    uint64_t UINT64_VAL;
    int8_t   INT8_VAL;
    int16_t  INT16_VAL;
    int32_t  INT32_VAL;
    int64_t  INT64_VAL;
    size_t   ENUM_INDEX;
    float    FLOAT_VAL;
    const char8_t* STRING_VAL;
    uint32_t MAP_INT_KEY;
    const char8_t* MAP_STRING_KEY;    
    EA::TDF::TimeValue TIME_VAL;
    EA::TDF::ObjectType OBJECT_TYPE_VAL;
    EA::TDF::ObjectId OBJECT_ID_VAL;

    const uint8_t *BLOB_VAL;
    uint8_t BLOB_VAL_SIZE;
    uint32_t BITFIELD_VAL;

    uint32_t UNION_INDEX;                        // Default 1
    EA::TDF::TdfId TEST_NON_DEFAULT_COPY_TDF_ID; // Default AllPrimitivesClass::TDF_ID
    uint32_t TEST_NON_DEFAULT_COPY_INDEX;        // Default 2. Don't use 1 because AllPrimitivesClass has a bool member at this index.
                                                 // For testing copyInto with the onlyIfNotDefault member visit option, 
                                                 // the value of the copied member should be nondefault and different from its value
                                                 // before the copy. This requirement becomes excessively restrictive when the chosen 
                                                 // member is a bool.
};

extern const TestData altTestData;

struct FillTestDataOpts
{
    FillTestDataOpts() : fillDataDepth(0), setIndex(false), testNonDefaultCopy(false), overwriteCollections(true) {}
    uint32_t fillDataDepth;     // Limits recursive calls to fillTestData. When fillDataDepth is 1, the current input is filled but any sub-fields/elements are not.
                                // Each recursive call receives a copy of the initial FillTestDataOpts with decremented fillDataDepth.
                                // If fillDataDepth is 0, recursive calls are unlimited (fillDataDepth is never decremented to 0 before a recursive call).

    bool setIndex;              // If true, set testData.UNION_INDEX as the active member index for unions
    bool testNonDefaultCopy;    // If true, for every Tdf with TdfId testData.TEST_NON_DEFAULT_COPY_TDF_ID, fill just the field whose member index is testData.TEST_NON_DEFAULT_COPY_INDEX
                                // (useful in testing copyInto with the onlyIfNotDefault MemberVisitOption)
    bool overwriteCollections;  // If true, clear maps and lists before filling them with testData
};

void fillTestData(EA::TDF::Tdf& tdf, const TestData& testData = TestData(), const FillTestDataOpts& = FillTestDataOpts());
void fillTestData(EA::TDF::TdfUnion& tdf, const TestData& testData = TestData(), const FillTestDataOpts& = FillTestDataOpts());
void fillTestData(EA::TDF::TdfGeneric& ref, const TestData& testData = TestData(), const FillTestDataOpts& = FillTestDataOpts());

template<typename T, bool enumVal = eastl::is_enum<T>::value >
struct fillTestDataRaw
{
    static void fill(T& val, const TestData& testData = TestData())
    {
        EA::TDF::TdfGenericReference ref(val);
        fillTestData(ref, testData);
    }    
};

template<typename T>
struct fillTestDataRaw<T, true>
{
    static void fill(T& val, const TestData& testData = TestData())
    {
        EA::TDF::TdfGenericReference ref(val);
        fillTestData(ref, testData);
    }    
};

bool compareChangeBits(const EA::TDF::TdfGenericReferenceConst& a, const EA::TDF::TdfGenericReferenceConst& b, const TestData& testData = TestData());
bool compareChangeBits(const EA::TDF::Tdf& a, const EA::TDF::Tdf& b, const TestData& testData = TestData());
bool compareChangeBits(const EA::TDF::TdfMapBase& a, const EA::TDF::TdfMapBase& b, const TestData& testData = TestData());
bool compareChangeBits(const EA::TDF::TdfVectorBase& a, const EA::TDF::TdfVectorBase& b, const TestData& testData = TestData());
bool compareChangeBits(const EA::TDF::VariableTdfBase& a, const EA::TDF::VariableTdfBase& b, const TestData& testData = TestData());
void fillTestData(EA::TDF::TdfGenericValue& val, const EA::TDF::TypeDescription& typeDesc, const TestData& testData = TestData());

#endif


