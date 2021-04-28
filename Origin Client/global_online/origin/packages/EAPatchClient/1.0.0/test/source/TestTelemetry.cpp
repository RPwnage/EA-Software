/////////////////////////////////////////////////////////////////////////////
// EAPatchClientTest/TestTelemetry.cpp
//
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include <EAPatchClientTest/Test.h>
#include <EAPatchClient/Telemetry.h>
#include <EAPatchClient/SystemInfo.h>
#include <EAIO/EAStreamMemory.h>
#include <EATest/EATest.h>
#include <stdlib.h>



///////////////////////////////////////////////////////////////////////////////
// TestTelemetry
//
static int TestSystemInfo()
{
    using namespace EA::Patch;
    using namespace EA::IO;

    int nErrorCount(0);

    EA::UnitTest::Report("    TestSystemInfo\n");

    {
        SystemInfo systemInfo(true, true);
        SystemInfo systemInfoExpected(systemInfo);

        /* Serialization is currently disabled.
        SystemInfoSerializer serializer;
        MemoryStream         stream;

        stream.AddRef();
        stream.SetOption(MemoryStream::kOptionResizeEnabled, 1);

        // Write out systemInfoExpected to XML text.
        EATEST_VERIFY(serializer.Serialize(systemInfo, &stream, true));
        EATEST_VERIFY(!serializer.HasError());

        if(!serializer.HasError())
        {
            stream.Write("", 1); // 0-terminate it, so we can write it as a string to output during debugging.
            EA::UnitTest::ReportVerbosity(1, "SystemInfoSerializer::Serialize:\n%s\n", (char*)stream.GetData());
        }

        // Read the XML text back in to systemInfo, verify the round trip lost no information.
        systemInfo.Reset();
        serializer.ClearError();
        stream.SetPosition(0); // Move back to the beginning of the stream.
        EATEST_VERIFY(serializer.Deserialize(systemInfo, &stream, true));
        EATEST_VERIFY(!serializer.HasError());
        */

        EATEST_VERIFY(systemInfo == systemInfoExpected);
    }

    return nErrorCount;
}



///////////////////////////////////////////////////////////////////////////////
// TestTelemetry
//
int TestTelemetry()
{
    int nErrorCount(0);

    nErrorCount += TestSystemInfo();

    return nErrorCount;
}





