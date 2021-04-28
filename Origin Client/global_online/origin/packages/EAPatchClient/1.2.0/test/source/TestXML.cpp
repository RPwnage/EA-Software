/////////////////////////////////////////////////////////////////////////////
// EAPatchClientTest/TestXML.cpp
//
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include <EAPatchClientTest/Test.h>
#include <EAPatchClient/Base.h>
#include <EAPatchClient/Hash.h>
#include <EAPatchClient/Locale.h>
#include <EAPatchClient/Serialization.h>
#include <EAPatchClient/XML.h>
#include <EAStdC/EADateTime.h>
#include <EAIO/EAStreamMemory.h>
#include <EAIO/FnMatch.h>
#include <EATest/EATest.h>


namespace
{
    using namespace EA::Patch;

    struct MyStruct
    {
        bool                mBool;
        uint64_t            mUint64;
        double              mDouble;
        String              mString;
        StringArray         mStringArray;
        LocalizedString     mLocalizedString;
        HashValueMD5        mHashMD5;
        HashValueMD5Brief   mHashMD5Brief;
        FileFilterArray     mFileFilterArray;
        EA::StdC::DateTime  mDateTime;

        MyStruct() : mBool(false), mUint64(0), mDouble(), mString(), mStringArray(), mLocalizedString(),
                     mHashMD5(), mHashMD5Brief(), mFileFilterArray(), mDateTime(0) {}
    };

    bool operator==(const MyStruct& a, const MyStruct& b)
    {
        return (a.mBool == b.mBool) &&
               (a.mUint64 == b.mUint64) &&
               (a.mDouble == b.mDouble) &&
               (a.mString == b.mString) &&
               (a.mStringArray == b.mStringArray) &&
               (a.mLocalizedString == b.mLocalizedString) &&
               (a.mHashMD5 == b.mHashMD5) &&
               (a.mHashMD5Brief == b.mHashMD5Brief) &&
               (a.mFileFilterArray == b.mFileFilterArray) &&
               (a.mDateTime == b.mDateTime);
    }


    struct MyStructSerializer : public SerializationBase
    {
        bool Serialize(const MyStruct& myStruct, EA::IO::IStream* pStream);
        bool Deserialize(MyStruct& myStruct, EA::IO::IStream* pStream);
    };

    bool MyStructSerializer::Serialize(const MyStruct& myStruct, EA::IO::IStream* pStream)
    {
        XMLWriter writer;

        writer.BeginDocument(pStream);
          writer.BeginParent("MyStruct");
            writer.WriteValue("mBool",            myStruct.mBool);
            writer.WriteValue("mDouble",          myStruct.mDouble);
            writer.WriteValue("mUint64",          myStruct.mUint64, true);
            writer.WriteValue("mString",          myStruct.mString);
            writer.WriteValue("mStringArray",     myStruct.mStringArray);
            writer.WriteValue("mLocalizedString", myStruct.mLocalizedString);
            writer.WriteValue("mHashMD5",         myStruct.mHashMD5);
            writer.WriteValue("mHashMD5Brief",    myStruct.mHashMD5Brief);
            writer.WriteValue("mFileFilterArray", myStruct.mFileFilterArray);
            writer.WriteValue("mDateTime",        myStruct.mDateTime, true);
          writer.EndParent("MyStruct");
        writer.EndDocument();

        TransferError(writer);

        return mbSuccess;
    }

    bool MyStructSerializer::Deserialize(MyStruct& myStruct, EA::IO::IStream* pStream)
    {
        XMLReader reader;
        bool      bFound;
        int       nErrorCount = 0;

        reader.BeginDocument(pStream);
          reader.BeginParent("MyStruct", bFound);                                       EATEST_VERIFY(bFound);
            reader.ReadValue("mBool",            bFound, myStruct.mBool);               EATEST_VERIFY(bFound);
            reader.ReadValue("mDouble",          bFound, myStruct.mDouble);             EATEST_VERIFY(bFound);
            reader.ReadValue("mUint64",          bFound, myStruct.mUint64);             EATEST_VERIFY(bFound);
            reader.ReadValue("mString",          bFound, myStruct.mString);             EATEST_VERIFY(bFound);
            reader.ReadValue("mStringArray",     bFound, myStruct.mStringArray, 1);     EATEST_VERIFY(bFound);
            reader.ReadValue("mLocalizedString", bFound, myStruct.mLocalizedString, 1); EATEST_VERIFY(bFound);
            reader.ReadValue("mHashMD5",         bFound, myStruct.mHashMD5);            EATEST_VERIFY(bFound);
            reader.ReadValue("mHashMD5Brief",    bFound, myStruct.mHashMD5Brief);       EATEST_VERIFY(bFound);
            reader.ReadValue("mFileFilterArray", bFound, myStruct.mFileFilterArray, 1); EATEST_VERIFY(bFound);
            reader.ReadValue("mDateTime",        bFound, myStruct.mDateTime);           EATEST_VERIFY(bFound);
          reader.EndParent("MyStruct");
        reader.EndDocument();

        // To consider: Do something with nErrorCount.
        TransferError(reader);

        return mbSuccess;
    }
}

char8_t kMyStructXML[] = 
    "<MyStruct>\n"
    "    <mBool>1</mBool>\n"
    "    <mDouble>123.456</mDouble>\n"
    "    <mUint64>0xffffffffffffffff</mUint64>\n"
    "    <mString>Hail to the Chief</mString>\n"
    "    <mStringArray>Str index 0</mStringArray>\n"
    "    <mStringArray>Str index 1</mStringArray>\n"
    "    <mStringArray>Str index 2</mStringArray>\n"
    "    <mLocalizedString L='en_us'>Hello</mLocalizedString>\n"
    "    <mLocalizedString L='es_es'>Hola</mLocalizedString>\n"
    "    <mHashMD5>8343a4bc3f248abb30902d1bc754a3e4</mHashMD5>\n"
    "    <mHashMD5Brief>8343a4bc3f248abb</mHashMD5Brief>\n"
    "    <mFileFilterArray flags=''>GameData/Plugins1/</mFileFilterArray>\n"
    "    <mFileFilterArray flags='CaseFold UnixPath'>GameData/Plugins2/*</mFileFilterArray>\n"
    "    <mDateTime>2012-06-15 23:59:59 GMT</mDateTime>\n"
    "</MyStruct>";


///////////////////////////////////////////////////////////////////////////////
// TestXML
//
int TestXML()
{
    using namespace EA::IO;
    using namespace EA::Patch;

    int nErrorCount(0);
    
    {
        MyStructSerializer myStructSerializer;
        MyStruct           myStructExpected;

        {  // Set up myStructExpected.
            myStructExpected.mBool              = true;
            myStructExpected.mUint64            = UINT64_C(0xffffffffffffffff);
            myStructExpected.mDouble            = 123.456;
            myStructExpected.mString            = "Hail to the Chief";
            myStructExpected.mStringArray.push_back("Str index 0");
            myStructExpected.mStringArray.push_back("Str index 1");
            myStructExpected.mStringArray.push_back("Str index 2");
            myStructExpected.mLocalizedString.AddString("en_us", "Hello");
            myStructExpected.mLocalizedString.AddString("es_es", "Hola");
            const uint8_t md5Data[]      = { 0x83, 0x43, 0xa4, 0xbc, 0x3f, 0x24, 0x8a, 0xbb, 0x30, 0x90, 0x2d, 0x1b, 0xc7, 0x54, 0xa3, 0xe4 };
            const uint8_t md5BriefData[] = { 0x83, 0x43, 0xa4, 0xbc, 0x3f, 0x24, 0x8a, 0xbb };
            memcpy(myStructExpected.mHashMD5.mData, md5Data, sizeof(md5Data));
            memcpy(myStructExpected.mHashMD5Brief.mData, md5BriefData, sizeof(md5BriefData));

            FileFilter fileFilter;
            fileFilter.mFilterSpec = "GameData/Plugins1/";
            fileFilter.mMatchFlags = EA::IO::kFNMNone;
            myStructExpected.mFileFilterArray.push_back(fileFilter);
            fileFilter.mFilterSpec = "GameData/Plugins2/*";
            fileFilter.mMatchFlags = EA::IO::kFNMCaseFold | EA::IO::kFNMUnixPath;
            myStructExpected.mFileFilterArray.push_back(fileFilter);

            myStructExpected.mDateTime.Set(2012, 6, 15, 23, 59, 59, 0);
        }

        {   // XMLWriter
            // We test serializing from MyStruct to XML text.
            MemoryStream stream;
            stream.AddRef();
            stream.SetOption(MemoryStream::kOptionResizeEnabled, 1);

            // Write out myStructExpected to XML text.
            EATEST_VERIFY(myStructSerializer.Serialize(myStructExpected, &stream));
            EATEST_VERIFY(!myStructSerializer.HasError());

            if(!myStructSerializer.HasError())
            {
                stream.Write("", 1); // 0-terminate it, so we can write it as a string to output.
                EA::UnitTest::ReportVerbosity(1, "MyStructSerializer::Serialize:\n%s", (char*)stream.GetData());
            }

            // Read the XML text back in to MyStruct, verify the round trip lost no information.
            MyStruct myStruct;

            myStructSerializer.ClearError();
            stream.SetPosition(0);              // Move back to the beginning of the stream.
            EATEST_VERIFY(myStructSerializer.Deserialize(myStruct, &stream));
            EATEST_VERIFY(!myStructSerializer.HasError());
            EATEST_VERIFY(myStruct == myStructExpected);
        }

        {   // XMLReader
            // We test deserializing from XML text to MyStruct. This is somewhat redundant
            // to the round trip test above, but in this case we're starting with manually 
            // authored HTML, which may differ from the generated HTML struct above.
            MemoryStream stream(kMyStructXML, sizeof(kMyStructXML), true, false, (MemoryStream::Allocator*)NULL, NULL);
            stream.AddRef();

            MyStruct myStruct;

            EATEST_VERIFY(myStructSerializer.Deserialize(myStruct, &stream));
            EATEST_VERIFY(!myStructSerializer.HasError());
            EATEST_VERIFY(myStruct == myStructExpected);
        }
    }

    return nErrorCount;
}

