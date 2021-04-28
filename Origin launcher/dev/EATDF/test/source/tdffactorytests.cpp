/*! *********************************************************************************************/
/*!
    \file 


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#include <test.h>
#include <testdata.h>
#include <test/tdf/alltypes.h>
#include <EAStdC/EAHashString.h>

using namespace EA::TDF;
using namespace EA::TDF::Test;


int verifyClass(const EA::TDF::TypeDescriptionClass& classDesc)
{
    int nErrorCount = 0;
    TdfFactory& tdfFacory = TdfFactory::get();

    EXPECT_TRUE(&classDesc == tdfFacory.getTypeDesc(classDesc.fullName))
        << "class = " << classDesc.fullName;
    EXPECT_TRUE(&classDesc == tdfFacory.getTypeDesc(classDesc.id))
        << "classId = " << classDesc.id;
    if (classDesc.id > TDF_ACTUAL_TYPE_UINT64)
    {
        uint32_t fnvVal = EA::StdC::FNV1_String8(classDesc.fullName);
        EXPECT_TRUE(fnvVal == classDesc.id)
            << "class = " << classDesc.fullName;
    }


    for (int i = 0; i < classDesc.memberCount; ++i)
    {
        const EA::TDF::TypeDescription* memberTypeDesc = classDesc.memberInfo[i].getTypeDescription();

        EXPECT_TRUE(memberTypeDesc->fullName != nullptr)
            << "class = " << classDesc.fullName << " memberIndex = " << i;

        if (memberTypeDesc->fullName == nullptr)
            continue;

        const EA::TDF::TypeDescriptionClass * memberAsClass = nullptr;
        if (memberTypeDesc->type == TDF_ACTUAL_TYPE_TDF || 
            memberTypeDesc->type == TDF_ACTUAL_TYPE_UNION)
        {
            memberAsClass = static_cast<const TypeDescriptionClass*>(memberTypeDesc);
        }

        if (memberAsClass)
        {
            verifyClass(*memberAsClass);
        }
        else
        {
            EXPECT_TRUE(memberTypeDesc == tdfFacory.getTypeDesc(memberTypeDesc->fullName))
                << "class = " << classDesc.fullName << " member = " << memberTypeDesc->fullName;
            EXPECT_TRUE(memberTypeDesc == tdfFacory.getTypeDesc(memberTypeDesc->id))
                << "class = " << classDesc.fullName << " memberId = " << memberTypeDesc->id;
            if (memberTypeDesc->id > TDF_ACTUAL_TYPE_UINT64)
            {
                uint32_t fnvVal = EA::StdC::FNV1_String8(memberTypeDesc->fullName);
                EXPECT_TRUE(fnvVal == memberTypeDesc->id)
                    << "class = " << classDesc.fullName;
            }

        }

        const EA::TDF::TypeDescriptionList * memberAsList = memberTypeDesc->asListDescription();
        if (memberAsList)
        {
            const EA::TDF::TypeDescription* valueTypeDesc = &memberAsList->valueType;

            EXPECT_TRUE(valueTypeDesc == tdfFacory.getTypeDesc(valueTypeDesc->fullName))
                << "class = " << classDesc.fullName << " value = " << valueTypeDesc->fullName;
            EXPECT_TRUE(valueTypeDesc == tdfFacory.getTypeDesc(valueTypeDesc->id))
                << "class = " << classDesc.fullName << " valueId = " << valueTypeDesc->id;

            if (valueTypeDesc->id > TDF_ACTUAL_TYPE_UINT64)
            {
                uint32_t fnvVal = EA::StdC::FNV1_String8(valueTypeDesc->fullName);
                EXPECT_TRUE(fnvVal == valueTypeDesc->id)
                    << "class = " << classDesc.fullName << " valueId = " << valueTypeDesc->id;
            }
        }

        const EA::TDF::TypeDescriptionMap * memberAsMap = memberTypeDesc->asMapDescription();
        if (memberAsMap)
        {
            const EA::TDF::TypeDescription* valueTypeDesc = &memberAsMap->valueType;
            const EA::TDF::TypeDescription* keyTypeDesc = &memberAsMap->keyType;

            EXPECT_TRUE(valueTypeDesc == tdfFacory.getTypeDesc(valueTypeDesc->fullName))
                << "class = " << classDesc.fullName << " value = " << valueTypeDesc->fullName;
            EXPECT_TRUE(valueTypeDesc == tdfFacory.getTypeDesc(valueTypeDesc->id))
                << "class = " << classDesc.fullName << " valueId = " << valueTypeDesc->id;

            EXPECT_TRUE(keyTypeDesc == tdfFacory.getTypeDesc(keyTypeDesc->fullName))
                << "class = " << classDesc.fullName << " value = " << valueTypeDesc->fullName;
            EXPECT_TRUE(keyTypeDesc == tdfFacory.getTypeDesc(keyTypeDesc->id))
                << "class = " << classDesc.fullName << " valueId = " << valueTypeDesc->id;

            if (valueTypeDesc->id > TDF_ACTUAL_TYPE_UINT64)
            {
                uint32_t fnvVal = EA::StdC::FNV1_String8(valueTypeDesc->fullName);
                EXPECT_TRUE(fnvVal == valueTypeDesc->id)
                    << "class = " << classDesc.fullName << " valueId = " << valueTypeDesc->id;
            }
            if (keyTypeDesc->id > TDF_ACTUAL_TYPE_UINT64)
            {
                uint32_t fnvVal = EA::StdC::FNV1_String8(keyTypeDesc->fullName);
                EXPECT_TRUE(fnvVal == keyTypeDesc->id)
                    << "class = " << classDesc.fullName << " valueId = " << valueTypeDesc->id;
            }
        }

    }

    return nErrorCount;
}

template <class T>
int testLookupNamedTypes()
{
    const EA::TDF::TypeDescriptionClass& classDesc = static_cast<const EA::TDF::TypeDescriptionClass&>(EA::TDF::TypeDescriptionSelector<T>::get());
    return verifyClass(classDesc);
}

TEST(TdfFactorTests, testParsingFunctions)
{

// Test basic parsing logic:
    TdfFactory::SubStringList outList;
    TdfFactory& tdfFacory = TdfFactory::get();
    EATDFEastlString stringToTest;
    stringToTest = "";     EXPECT_TRUE((tdfFacory.parseString(stringToTest, outList) == false));
    stringToTest = "okay";     EXPECT_TRUE((tdfFacory.parseString(stringToTest, outList) == true && outList.size() == 1));

    stringToTest = "something.to.test";     EXPECT_TRUE((tdfFacory.parseString(stringToTest, outList) == true && outList.size() == 3));
    stringToTest = "something[to].test";    EXPECT_TRUE((tdfFacory.parseString(stringToTest, outList) == true && outList.size() == 3));
    stringToTest = "something[to][test]";   EXPECT_TRUE((tdfFacory.parseString(stringToTest, outList) == true && outList.size() == 3));
    stringToTest = "OH::BOY::HOWDY.something.to.test"; EXPECT_TRUE((tdfFacory.parseString(stringToTest, outList) == true && outList.size() == 4));

    stringToTest = "something.to.fail.";    EXPECT_TRUE((tdfFacory.parseString(stringToTest, outList) == false));
    stringToTest = "something.to...fail";   EXPECT_TRUE((tdfFacory.parseString(stringToTest, outList) == false));
    stringToTest = ".something.to.fail";    EXPECT_TRUE((tdfFacory.parseString(stringToTest, outList) == false));
    stringToTest = "something[to.fail";     EXPECT_TRUE((tdfFacory.parseString(stringToTest, outList) == false));
    stringToTest = "something[to]fail";     EXPECT_TRUE((tdfFacory.parseString(stringToTest, outList) == false));
    stringToTest = "[something].to.fail";   EXPECT_TRUE((tdfFacory.parseString(stringToTest, outList) == false));

// Test TypeDesc lookup:
    const TypeDescription* outTypeDesc = tdfFacory.getTypeDesc("EA::TDF::Test::AllPrimitivesClass.UInt64");
    EXPECT_TRUE(outTypeDesc && outTypeDesc->type == TDF_ACTUAL_TYPE_UINT64);

    outTypeDesc = tdfFacory.getTypeDesc("EA::TDF::Test::AllComplexClass.Class.UInt64");
    EXPECT_TRUE(outTypeDesc && outTypeDesc->type == TDF_ACTUAL_TYPE_UINT64);

    outTypeDesc = tdfFacory.getTypeDesc("EA::TDF::Test::AllMapsComplexClass.StringObjectId");
    EXPECT_TRUE(outTypeDesc && outTypeDesc->type == TDF_ACTUAL_TYPE_MAP);

    outTypeDesc = tdfFacory.getTypeDesc("EA::TDF::Test::AllMapsComplexClass.StringObjectId[LOVERS]");
    EXPECT_TRUE(outTypeDesc && outTypeDesc->type == TDF_ACTUAL_TYPE_OBJECT_ID);


// Test Member Accessors:
    EA::TDF::Test::AllPrimitivesClass myAllPrimitivesClass;
    TdfGenericReference outGenericRef;

    EXPECT_TRUE(tdfFacory.getTdfMemberReference(TdfGenericReference(myAllPrimitivesClass), "EA::TDF::Test::AllPrimitivesClass.UInt64", outGenericRef));
    outGenericRef.asUInt64() = 101;
    EXPECT_TRUE(myAllPrimitivesClass.getUInt64() == 101);

    EA::TDF::Test::AllComplexClass myAllComplexClass;

    outGenericRef.clear();
    EXPECT_TRUE(tdfFacory.getTdfMemberReference(TdfGenericReference(myAllComplexClass), "EA::TDF::Test::AllComplexClass.Class.UInt64", outGenericRef));
    outGenericRef.asUInt64() = 101;
    EXPECT_TRUE(myAllComplexClass.getClass().getUInt64() == 101);

    EA::TDF::Test::AllMapsComplexClass myAllMapsComplexClass;

    outGenericRef.clear();
    EXPECT_TRUE(tdfFacory.getTdfMemberReference(TdfGenericReference(myAllMapsComplexClass), "EA::TDF::Test::AllMapsComplexClass.StringObjectId", outGenericRef));
    TdfGenericReference value;
    outGenericRef.asMap().insertKeyGetValue(TdfGenericReferenceConst(TdfString("HELLO")), value);
    value.asObjectId().id = 10101;
    EXPECT_TRUE(myAllMapsComplexClass.getStringObjectId().size() == 1);
    EXPECT_TRUE(myAllMapsComplexClass.getStringObjectId().getValueByKey(TdfGenericReferenceConst(TdfString("HELLO")), value));
    EXPECT_TRUE(value.asObjectId().id == 10101);

    outGenericRef.clear();
    EXPECT_TRUE(tdfFacory.getTdfMemberReference(TdfGenericReference(myAllMapsComplexClass), "EA::TDF::Test::AllMapsComplexClass.StringObjectId[LOVERS]", outGenericRef));
    outGenericRef.asObjectId().id = 10101;
    EXPECT_TRUE(myAllMapsComplexClass.getStringObjectId().getValueByKey(TdfGenericReferenceConst(TdfString("LOVERS")), value));
    EXPECT_TRUE(value.asObjectId().id == 10101);

// Type registration helper:
    tdfFacory.registerTypeName("foo", "bar", "foo::bar");
    const char8_t* outValue; 
    EXPECT_TRUE(tdfFacory.lookupTypeName("foo", "bar", outValue));
    EXPECT_TRUE(eastl::string("foo::bar") == outValue);

    tdfFacory.registerTypeName("foo.AndMore", "bar", "foo::bar2");
    outValue = nullptr; 
    EXPECT_TRUE(tdfFacory.lookupTypeName("foo.AndMore", "bar", outValue));
    EXPECT_TRUE(eastl::string("foo::bar") != outValue);
    EXPECT_TRUE(eastl::string("foo::bar2") == outValue);
    EXPECT_TRUE(tdfFacory.lookupTypeName("foo", "bar", outValue));
    EXPECT_TRUE(eastl::string("foo::bar2") == outValue);
    EXPECT_TRUE(tdfFacory.lookupTypeName("foo.WaitWhatIsThis", "bar", outValue));
    EXPECT_TRUE(eastl::string("foo::bar2") == outValue);
}


TEST_ALL_TDFS(testLookupNamedTypes, testLookupNamedTypes);
