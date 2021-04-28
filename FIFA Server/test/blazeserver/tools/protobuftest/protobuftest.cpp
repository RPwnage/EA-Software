/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    Simple program to test protobuf <-> EATDF serialization/deserialization performance. 
    For functional testing, use unit tests.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/component/blazerpc.h"
#include "framework/tdf/protobuftest_server.h"
#include "EATDF/tdffactory.h"

#include "framework/protocol/shared/encoder.h"
#include "framework/protocol/shared/heat2encoder.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/io/coded_stream.h"

#include <iostream>

#include <ctime>
#include <ratio>
#include <chrono>

namespace protobuftest
{

/*** Public functions ****************************************************************************/
static void printMessage(const char8_t* msg)
{
    printf("%s", msg);
#if defined EA_PLATFORM_WINDOWS
    OutputDebugString(msg);
#endif
}


void runPerformanceTest(int32_t numEntriesInCollection, bool warmup)
{
#ifdef BLAZE_PROTOBUF_ENABLED
    // Serialization/Deserialization Performance test
    {
        using namespace std::chrono;
        high_resolution_clock::time_point start;
        high_resolution_clock::time_point end;

        const int64_t kNumIterations = 100 * 1000; // Number of times the serialization/deserialization routines are run
        const int32_t kNumEntriesInCollections = numEntriesInCollection; // Number of entries pushed in map/list
        const int32_t kMaxMessageSize = 8192; // Max. size we expect of the message. This is just for allocation purpose and fairness to each test.

        double protobufSerializeArrayDuration = 0.0, protobufSerializeStreamDuration = 0.0, protobufDeserializeDuration = 0.0, eatdfSerializeDuration = 0.0, eatdfDeserializaeDuration = 0.0;
        bool resultsValid = true;

        Blaze::Protobuf::Test::PerfTest1 test;
        test.setBool(true);
        test.setUInt8(40);
        test.setUInt16(41);
        test.setUInt32(42);
        test.setUInt64(43);
        test.setInt8(-40);
        test.setInt16(-41);
        test.setInt32(-42);
        test.setInt64(-43);
        test.setFloat(42.42f);
        test.setString("Hello World!");
        test.setOutOfClassEnum(Blaze::Protobuf::Test::OUT_OF_CLASS_ENUM_VAL_1);
        test.setInClassEnum(Blaze::Protobuf::Test::PerfTest1::IN_OF_CLASS_ENUM_VAL_1);
        test.getUnion().setInt32(42);
        test.getBitField().setBIT_1_2(2);

        uint8_t blob[5] = { '1', '2', '\0', '4', '5' };
        test.getBlob().setData(blob, 5);
        test.getNestedClass().setUInt32(42);
        test.getNestedClass().setString("Hello World!");

        for (int32_t index = 0; index < kNumEntriesInCollections; ++index)
        {
            // Lists
            test.getInt32List().push_back(42);
            { auto& perfClass = *(test.getClassList().pull_back()); perfClass.setUInt32(42); perfClass.setString("Hello World!"); }

            // Maps
            test.getInt32Int32Map()[index] = 42;
            { auto perfClass = test.getInt32ClassMap().allocate_element(); perfClass->setUInt32(42); perfClass->setString("Hello World!"); test.getInt32ClassMap()[index] = perfClass; }
        }


        {
            std::string buffer;
            buffer.resize(kMaxMessageSize);

            start = high_resolution_clock::now();

            test.ByteSize(); // Calculate only once as we do the same for other 2.
            for (int64_t index = 0; index < kNumIterations; ++index)
            {
                int size = test.GetCachedSize();
                google::protobuf::io::ArrayOutputStream out(reinterpret_cast<google::protobuf::uint8 *>(&buffer[0]), size);
                google::protobuf::io::CodedOutputStream coded_out(&out);
                test.SerializeWithCachedSizes(&coded_out);
            }

            end = high_resolution_clock::now();

            protobufSerializeStreamDuration = duration_cast<duration<double>>(end - start).count();
        }


        {
            std::string buffer;
            buffer.resize(kMaxMessageSize);

            start = high_resolution_clock::now();

            test.ByteSize(); // Calculate only once as we do the same for other 2.
            for (int64_t index = 0; index < kNumIterations; ++index)
            {
                test.SerializeWithCachedSizesToArray(reinterpret_cast<google::protobuf::uint8 *>(&buffer[0]));
            }

            end = high_resolution_clock::now();

            protobufSerializeArrayDuration = duration_cast<duration<double>>(end - start).count();


            eastl::vector<Blaze::Protobuf::Test::PerfTest1> test2Instances;
            test2Instances.resize(kNumIterations);

            start = high_resolution_clock::now();

            for (int64_t index = 0; index < kNumIterations; ++index)
            {
                test2Instances[index].ParsePartialFromString(buffer);
            }

            end = high_resolution_clock::now();
            
            protobufDeserializeDuration = duration_cast<duration<double>>(end - start).count();
            resultsValid = resultsValid && test.equalsValue(test2Instances[kNumIterations - 1]);
        }


        {
            Blaze::RawBuffer buffer(kMaxMessageSize);
            Blaze::Heat2Encoder encoder;

            start = high_resolution_clock::now();

            for (int64_t index = 0; index < kNumIterations; ++index)
            {
                encoder.encode(buffer, test);
                buffer.reset();
            }

            end = high_resolution_clock::now();

            eatdfSerializeDuration = duration_cast<duration<double>>(end - start).count();

            encoder.encode(buffer, test);
            int32_t bufferSize = buffer.size();

            eastl::vector<Blaze::Protobuf::Test::PerfTest1> test2Instances;
            test2Instances.resize(kNumIterations);

            Blaze::Heat2Decoder decoder;

            start = high_resolution_clock::now();

            for (int64_t index = 0; index < kNumIterations; ++index)
            {
                decoder.decode(buffer, test2Instances[index]);
                buffer.reset();
                buffer.put(bufferSize);
            }

            end = high_resolution_clock::now();
            
            eatdfDeserializaeDuration = duration_cast<duration<double>>(end - start).count();
            resultsValid = resultsValid && test.equalsValue(test2Instances[kNumIterations - 1]);
        }

        if (!warmup)
        {
            if (resultsValid)
            {
                printMessage("\n");
                
                printMessage("  Serialization Results:\n");
                char8_t durationMsg[1024];
                
                sprintf(durationMsg, "      Protobuf Serialize(Array version) took %f seconds.\n", protobufSerializeArrayDuration);
                printMessage(durationMsg);

                sprintf(durationMsg, "      Protobuf Serialize(Stream version) took %f seconds.\n", protobufSerializeStreamDuration);
                printMessage(durationMsg);
                
                sprintf(durationMsg, "      EATDF Serialize took %f seconds.\n", eatdfSerializeDuration);
                printMessage(durationMsg);
                
                printMessage("\n");
                printMessage("  DeSerialization Results:\n");
                
                sprintf(durationMsg, "      Protobuf deserialize took %f seconds.\n", protobufDeserializeDuration);
                printMessage(durationMsg);
                
                sprintf(durationMsg, "      EATDF deserialize took %f seconds.\n", eatdfDeserializaeDuration);
                printMessage(durationMsg);

                printMessage("\n");
            }
            else
            {
                EA_ASSERT(false);

                char8_t failedMsg[1024];
                sprintf(failedMsg, "    test and test2 instances do not match. Performance test did not run successfully.\n");
                printMessage(failedMsg);
            }
        }
    }
#endif
}

int protobuftest_main(int argc, char** argv)
{
#ifdef BLAZE_PROTOBUF_ENABLED
    EA::TDF::TdfFactory::fixupTypes();

    // The performance tests between Protobuf and EATDF.
    const int32_t kNumEntries = 5;
    printMessage("Warming up....it may take a minute.....\n");
    runPerformanceTest(0, true);
    runPerformanceTest(kNumEntries, true);
    printMessage("warm up done.\n");

    printMessage("***Running performance test without any container entries.***\n");
    runPerformanceTest(0, false);
    
    printMessage("***Running performance test with container entries.***\n");
    runPerformanceTest(kNumEntries, false);

    printMessage("protobuf <-> EATDF serialization/deserialization test finished.\n");
#endif
    return 0;
}

}
