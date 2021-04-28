/*! *********************************************************************************************/
/*!
    \file 


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#include <testdata.h>

// Defaults
const char8_t* MAP_STRING_KEY = "abc123";
const char8_t* STRING_VAL = "averybigcat";

const EA::TDF::TimeValue TIME_VAL("23s", EA::TDF::TimeValue::TIME_FORMAT_INTERVAL);
const EA::TDF::ObjectType OBJECT_TYPE_VAL(1,2);
const EA::TDF::ObjectId OBJECT_ID_VAL(OBJECT_TYPE_VAL, 3);

const uint8_t BLOB_VAL[] = {1,2,3,4,5,6,7,8,9,10};
const uint8_t BLOB_VAL_SIZE = sizeof(BLOB_VAL) / sizeof(uint8_t);

// Alternates
const char8_t* MAP_STRING_KEY2 = "def456";
const char8_t* STRING_VAL2 = "myfatdog";

const EA::TDF::TimeValue TIME_VAL2("2m", EA::TDF::TimeValue::TIME_FORMAT_INTERVAL);
const EA::TDF::ObjectType OBJECT_TYPE_VAL2(43,21);
const EA::TDF::ObjectId OBJECT_ID_VAL2(OBJECT_TYPE_VAL2, 9999);

const uint8_t BLOB_VAL2[] = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
const uint8_t BLOB_VAL_SIZE2 = sizeof(BLOB_VAL2) / sizeof(uint8_t);

const TestData altTestData(BOOL_VAL2, UINT8_VAL2, UINT16_VAL2, UINT32_VAL2, UINT64_VAL2,
                           INT8_VAL2, INT16_VAL2, INT32_VAL2, INT64_VAL2, ENUM_INDEX2,
                           FLOAT_VAL2, STRING_VAL2, MAP_INT_KEY2, MAP_STRING_KEY2, 
                           TIME_VAL2, OBJECT_TYPE_VAL2, OBJECT_ID_VAL2, 
                           BLOB_VAL2, BLOB_VAL_SIZE2, BITFIELD_VAL2, 0);

using namespace EA::TDF;
using namespace EA::TDF::Test;


TestData::TestData(bool boolVal, uint8_t uint8Val, uint16_t uint16Val, uint32_t uint32Val, uint64_t uint64Val,
         int8_t int8Val, int16_t int16Val, int32_t int32Val, int64_t int64Val, size_t enumIndex, 
         float floatVal, const char8_t* stringVal, uint32_t mapIntKey, const char8_t* mapStringKey, 
         const EA::TDF::TimeValue& timeVal, const EA::TDF::ObjectType& objTypeVal, const EA::TDF::ObjectId& objIdVal, 
         const uint8_t* blobVal, uint8_t blobValSize, uint32_t bitFieldVal, uint32_t unionIndexVal, 
         TdfId testNonDefaultCopyTdfIdVal, uint32_t testNonDefaultCopyIndexVal) :
    BOOL_VAL(boolVal),
    UINT8_VAL(uint8Val),
    UINT16_VAL(uint16Val),
    UINT32_VAL(uint32Val),
    UINT64_VAL(uint64Val),
    INT8_VAL(int8Val),
    INT16_VAL(int16Val),
    INT32_VAL(int32Val),
    INT64_VAL(int64Val),
    ENUM_INDEX(enumIndex),
    FLOAT_VAL(floatVal),
    STRING_VAL(stringVal),
    MAP_INT_KEY(mapIntKey),
    MAP_STRING_KEY(mapStringKey),
    TIME_VAL(timeVal),
    OBJECT_TYPE_VAL(objTypeVal),
    OBJECT_ID_VAL(objIdVal),
    BLOB_VAL(blobVal),
    BLOB_VAL_SIZE(blobValSize),
    BITFIELD_VAL(bitFieldVal),
    UNION_INDEX(unionIndexVal),
    TEST_NON_DEFAULT_COPY_TDF_ID(testNonDefaultCopyTdfIdVal),
    TEST_NON_DEFAULT_COPY_INDEX(testNonDefaultCopyIndexVal)
{
}

void fillTestData(Tdf& tdf, const TestData& testData, const FillTestDataOpts& opts)
{
    if (opts.fillDataDepth != 1)
    {
        FillTestDataOpts newOpts = opts;

        if (newOpts.fillDataDepth != 0)
            --newOpts.fillDataDepth;

        TdfMemberIterator itr(tdf);

        if (newOpts.testNonDefaultCopy)
        {
            if (tdf.getTdfId() == testData.TEST_NON_DEFAULT_COPY_TDF_ID)
            {
                // Fill chosen member only
                itr.moveTo(testData.TEST_NON_DEFAULT_COPY_INDEX);
                fillTestData(itr, testData, newOpts);
                return;
            }
        } 

        while (itr.next())
                fillTestData(itr, testData, newOpts);
    }
}


void fillTestData(TdfUnion& tdf, const TestData& testData, const FillTestDataOpts& opts)
{
    if (opts.setIndex)
        tdf.switchActiveMember(testData.UNION_INDEX);

    if (opts.fillDataDepth != 1)
    {
        FillTestDataOpts newOpts = opts;

        if (newOpts.fillDataDepth != 0)
            --newOpts.fillDataDepth;

    TdfMemberIterator itr(tdf);
    if (itr.next())
            fillTestData(itr, testData, newOpts);
    }
}


void fillTestData(TdfGeneric& ref, const TestData& testData, const FillTestDataOpts& opts)
{
    // If fillDataDepth == 1, ref is the last value to set before ending recursion on fillTestData
    // If fillDataDepth == 0, we ignore fillDataDepth and recurse as necessary
    FillTestDataOpts newOpts = opts;
    if (newOpts.fillDataDepth > 1)
        --newOpts.fillDataDepth;

    switch (ref.getType())
    {
    case TDF_ACTUAL_TYPE_MAP:                
        {
            if (opts.overwriteCollections)
            {
                ref.asMap().clearMap();
                ref.asMap().clearIsSet();
            }

            TdfBlob blobKey;
            TdfGenericValue mapKey;
            TdfGenericReference mapVal;
            TdfMapBase& mapBase = ref.asMap();
            if (mapBase.getTypeDescription().keyType.isIntegral())
            {
                if (mapBase.getTypeDescription().keyType.isEnum())
                    {
                    const TdfEnumMap& enumMap = *mapBase.getKeyTypeDesc().asEnumMap();
                            size_t enumIndex = 0;
                            int32_t enumValue = 0;
                            for (TdfEnumMap::NamesByValue::const_iterator i = enumMap.getNamesByValue().begin(), e = enumMap.getNamesByValue().end(); 
                                i != e; ++i, ++enumIndex)
                            {
                                if (enumIndex == testData.ENUM_INDEX)
                                {
                                    enumValue = static_cast<const TdfEnumInfo&>(*i).mValue;
                                    break;
                                }
                            }
                            
                    mapKey.set(enumValue, *mapBase.getKeyTypeDesc().asEnumMap());
                        }
                        else
                            mapKey.set(testData.MAP_INT_KEY);
                    }
            else if (mapBase.getTypeDescription().keyType.isString())
            {
                    mapKey.set(testData.MAP_STRING_KEY);
            }
            else if (mapBase.getTypeDescription().keyType.type == EA::TDF::TDF_ACTUAL_TYPE_BLOB)
                    {
                        blobKey.setData(testData.BLOB_VAL, testData.BLOB_VAL_SIZE);
                        mapKey.set(blobKey);
                    }
            else
                    EA_ASSERT_MESSAGE(false, "Cannot set unknown map key type");

            if (!mapBase.insertKeyGetValue(mapKey, mapVal))
                EA_ASSERT_MESSAGE(false, "Attempt to reassign value to map key");

            if (newOpts.fillDataDepth != 1)
            {
                fillTestData(mapVal, testData, newOpts);
            }
            else
            {
                // For collections, copyInto justifiably doesn't check whether an element itself is set before calling copyInto() on it. 
                // So for tests utilizing maxFillDepth, we still need to mark nested collections/classes as set 
                // (e.g. if ref is a map of lists of X's, each list of X's should return true when isSet() is called on it).
                switch (mapVal.getType())
                {
                    case TDF_ACTUAL_TYPE_MAP:                mapVal.asMap().markSet(); break;
                    case TDF_ACTUAL_TYPE_LIST:               mapVal.asList().markSet(); break;
                    case TDF_ACTUAL_TYPE_VARIABLE:           mapVal.asVariable().markSet(); break;
                    default:
                        break;
                }
            }
        }
        break;
    case TDF_ACTUAL_TYPE_LIST:               
        {
            if (opts.overwriteCollections)
            {
                ref.asList().clearVector();
                ref.asList().clearIsSet();
            }

            TdfGenericReference listRef;
            ref.asList().pullBackRef(listRef);

            if (newOpts.fillDataDepth != 1)
            {
                fillTestData(listRef, testData, newOpts);
            }
            else
            {
                switch (listRef.getType())
                {
                case TDF_ACTUAL_TYPE_MAP:                listRef.asMap().markSet(); break;
                case TDF_ACTUAL_TYPE_LIST:               listRef.asList().markSet(); break;
                case TDF_ACTUAL_TYPE_VARIABLE:           listRef.asVariable().markSet(); break;
                default:
                    break;
                }
            }
        }
        break;
    case TDF_ACTUAL_TYPE_VARIABLE:           
        {
            AllPrimitivesClass* var = static_cast<AllPrimitivesClass*>(EA::TDF::TdfObject::createInstance<AllPrimitivesClass>(*EA::Allocator::ICoreAllocator::GetDefaultAllocator(), "Var", 0));

            if (newOpts.fillDataDepth != 1)
                fillTestData(*var, testData, newOpts);

            ref.asVariable().set(*var);
        }
        break;

    // For the following types, we don't check for newOpts.fillDataDepth == 1 because either this is a 
    // tdf/union and the next call to fillTestData will handle the case of fillDataDepth == 1,
    // or there will be no further calls to fillTestData
    case TDF_ACTUAL_TYPE_UNION:
        {
            newOpts.setIndex = true;
            fillTestData(ref.asUnion(), testData, newOpts); 
        }
        break;
    case TDF_ACTUAL_TYPE_TDF:
        {
            fillTestData(ref.asTdf(), testData, newOpts); 
        }
        break;
    case TDF_ACTUAL_TYPE_ENUM:
        {
            const EA::TDF::TdfEnumMap::NamesByValue& nbv = ref.getTypeDescription().asEnumMap()->getNamesByValue();
            size_t index = nbv.size() > (size_t)testData.ENUM_INDEX ? (size_t)testData.ENUM_INDEX : 0;  //take given index unless there's not enough
            EA::TDF::TdfEnumMap::NamesByValue::const_iterator itr = nbv.begin();
            eastl::advance(itr, index);
            ref.asEnum() = ((const TdfEnumInfo&)*itr).mValue;
        }
        break;
    case TDF_ACTUAL_TYPE_GENERIC_TYPE:
        {
            ref.asGenericType().setType<AllPrimitivesClass>();          // Alternatively, this could call .create("EA::TDF::Test::AllPrimitivesClass");
            fillTestData(ref.asGenericType().get().asTdf(), testData);
            break;
        }
    case TDF_ACTUAL_TYPE_STRING:             ref.asString().set(testData.STRING_VAL); break;
    case TDF_ACTUAL_TYPE_FLOAT:              ref.asFloat() = testData.FLOAT_VAL; break;
    case TDF_ACTUAL_TYPE_BITFIELD:           ref.asBitfield().setBits(testData.BITFIELD_VAL); break;
    case TDF_ACTUAL_TYPE_BLOB:               ref.asBlob().setData(testData.BLOB_VAL, testData.BLOB_VAL_SIZE); break;
    case TDF_ACTUAL_TYPE_OBJECT_TYPE:        ref.asObjectType() = testData.OBJECT_TYPE_VAL; break;
    case TDF_ACTUAL_TYPE_OBJECT_ID:          ref.asObjectId() = testData.OBJECT_ID_VAL; break;
    case TDF_ACTUAL_TYPE_TIMEVALUE:          ref.asTimeValue() = testData.TIME_VAL; break;
    case TDF_ACTUAL_TYPE_BOOL:               ref.asBool() = testData.BOOL_VAL; break;
    case TDF_ACTUAL_TYPE_INT8:               ref.asInt8() = testData.INT8_VAL; break;
    case TDF_ACTUAL_TYPE_UINT8:              ref.asUInt8() = testData.UINT8_VAL; break;
    case TDF_ACTUAL_TYPE_INT16:              ref.asInt16() = testData.INT16_VAL; break;
    case TDF_ACTUAL_TYPE_UINT16:             ref.asUInt16() = testData. UINT16_VAL; break;
    case TDF_ACTUAL_TYPE_INT32:              ref.asInt32() = testData.INT32_VAL; break;
    case TDF_ACTUAL_TYPE_UINT32:             ref.asUInt32() = testData.UINT32_VAL; break;
    case TDF_ACTUAL_TYPE_INT64:              ref.asInt64() = testData.INT64_VAL; break;
    case TDF_ACTUAL_TYPE_UINT64:             ref.asUInt64() = testData.UINT64_VAL; break;
    default:
        break;
    }
}



void fillTestData(EA::TDF::TdfGenericValue& val, const EA::TDF::TypeDescription& typeDesc, const TestData& testData)
{
    val.setType(typeDesc);

    fillTestData(val, testData);
}

bool compareChangeBits(const EA::TDF::TdfGenericReferenceConst& a, const EA::TDF::TdfGenericReferenceConst& b, const TestData& testData)
{
    if (a.getType() != b.getType())
        return false;

    switch (a.getType())
{
    case TDF_ACTUAL_TYPE_TDF:
    case TDF_ACTUAL_TYPE_UNION:
        return compareChangeBits(a.asTdf(), b.asTdf(), testData);
    case TDF_ACTUAL_TYPE_MAP:
        return compareChangeBits(a.asMap(), b.asMap(), testData);
    case TDF_ACTUAL_TYPE_LIST:
        return compareChangeBits(a.asList(), b.asList(), testData);
    case TDF_ACTUAL_TYPE_VARIABLE:
        return compareChangeBits(a.asVariable(), b.asVariable(), testData);
    default:
        return true;
    }
}

bool compareChangeBits(const EA::TDF::VariableTdfBase& a, const EA::TDF::VariableTdfBase& b, const TestData& testData)
    {
    if (a.isValid() != b.isValid() || a.isSet() != b.isSet())
        return false;

    if (!a.isValid())
        return true;

    return compareChangeBits(*a.get(), *b.get(), testData);
    }

bool compareChangeBits(const EA::TDF::Tdf& a, const EA::TDF::Tdf& b, const TestData& testData)
{
    if (a.getTdfId() != b.getTdfId())
        return false;

    TdfMemberIteratorConst ait(a);
    TdfMemberIteratorConst bit(b);
    while (ait.next())
    {
        if (!bit.next())
            return false; 
        
        if (ait.getInfo() != bit.getInfo())
            return false;

        switch (ait.getType())
         {
        case TDF_ACTUAL_TYPE_TDF:
        case TDF_ACTUAL_TYPE_UNION:
            if (!compareChangeBits(ait.asTdf(), bit.asTdf(), testData))
                return false;
            break;
        case TDF_ACTUAL_TYPE_MAP:
            if (!compareChangeBits(ait.asMap(), bit.asMap(), testData))
                return false;
            break;
        case TDF_ACTUAL_TYPE_LIST:
            if (!compareChangeBits(ait.asList(), bit.asList(), testData))
                return false;
            break;
        case TDF_ACTUAL_TYPE_VARIABLE:
            if (!compareChangeBits(ait.asVariable(), bit.asVariable(), testData))
                return false;
            break;
        default:
            if (ait.isSet() != bit.isSet())
                return false;
            break;
        }
    }

    if (bit.next())
        return false;

    return true;
}

bool compareChangeBits(const EA::TDF::TdfMapBase& a, const EA::TDF::TdfMapBase& b, const TestData& testData)
{
    if (a.getKeyType() != b.getKeyType() || a.getValueType() != b.getValueType() || a.mapSize() != b.mapSize() || a.isSet() != b.isSet())
        return false;

    if (a.mapSize() == 0)
        return true;

    if (a.getValueType() == TDF_ACTUAL_TYPE_TDF || a.getValueType() == TDF_ACTUAL_TYPE_VARIABLE || a.getValueType() == TDF_ACTUAL_TYPE_UNION || 
        a.getValueType() == TDF_ACTUAL_TYPE_LIST || a.getValueType() == TDF_ACTUAL_TYPE_MAP)
    {
        TdfGenericReferenceConst refA;
        TdfGenericReferenceConst refB;
        TdfGenericValue mapKey;
        TdfBlob blobKey;

        if (a.getKeyTypeDesc().isIntegral())
        {
            if (a.getKeyTypeDesc().isEnum())
            {
                const TdfEnumMap& enumMap = *a.getKeyTypeDesc().asEnumMap();
                size_t enumIndex = 0;
                int32_t enumValue = 0;
                for (TdfEnumMap::NamesByValue::const_iterator i = enumMap.getNamesByValue().begin(), e = enumMap.getNamesByValue().end(); 
                    i != e; ++i, ++enumIndex)
                {
                    if (enumIndex == testData.ENUM_INDEX)
                    {
                        enumValue = static_cast<const TdfEnumInfo&>(*i).mValue;
                        break;
                    }
                }

                mapKey.set(enumValue, *a.getKeyTypeDesc().asEnumMap());
            }
            else
            {
                mapKey.set(testData.MAP_INT_KEY);
            }
        }
        else
        {
            switch (a.getKeyType())
            {
            case TDF_ACTUAL_TYPE_STRING:
            {
                mapKey.set(testData.MAP_STRING_KEY);
            }
            break;
            case TDF_ACTUAL_TYPE_BLOB:
             {
                blobKey.setData(testData.BLOB_VAL, testData.BLOB_VAL_SIZE);
                mapKey.set(blobKey);
            }
            break;
            default:
                EA_ASSERT_MESSAGE(false, "Cannot get value by unknown map key type");
            }
        }

        if (!a.getValueByKey(mapKey, refA) || !b.getValueByKey(mapKey, refB))
            return false;
        if (!compareChangeBits(refA, refB, testData))
              return false;
         }

    return true;
    }

bool compareChangeBits(const EA::TDF::TdfVectorBase& a, const EA::TDF::TdfVectorBase& b, const TestData& testData)
{
    if (a.getTdfType() != b.getTdfType() || a.vectorSize() != b.vectorSize() || a.isSet() != b.isSet())
        return false;

    if (a.getTdfType() == TDF_ACTUAL_TYPE_TDF || a.getTdfType() == TDF_ACTUAL_TYPE_VARIABLE || a.getTdfType() == TDF_ACTUAL_TYPE_UNION || 
        a.getTdfType() == TDF_ACTUAL_TYPE_LIST || a.getTdfType() == TDF_ACTUAL_TYPE_MAP)
    {
        for (size_t i = 0; i < a.vectorSize(); ++i)
        {
            TdfGenericReferenceConst refA;
            TdfGenericReferenceConst refB;

            if (!a.getValueByIndex(i, refA) || !b.getValueByIndex(i, refB))
                return false;

            if (!compareChangeBits(refA, refB, testData))
        return false;  
        }
    }

    return true;
}


