/*************************************************************************************************/
/*!
    \file loggertest.cpp

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include <EASTL/string.h>
#include <EASTL/tuple.h>
#include <EASTL/vector.h>
#include "test/framework/loggertest.h"
#include "framework/logger.h"

namespace BlazeServerTest
{
namespace Framework
{

    // test Blaze::Logger::ThisFunction
    TEST_F(LoggerTest, ThisFunctionTest)
    {
        // STATIC_FUNC, Linux clang
        ASSERT_STREQ(::Blaze::Logger::ThisFunction(
            "Blaze::ExampleNamespace::ExampleClass::simpleFunction() const",
            nullptr).get(),
            "[ExampleClass:0000000000000000].simpleFunction: "
        );

        // THIS_FUNC, constructor, Linux clang
        ASSERT_STREQ(::Blaze::Logger::ThisFunction(
            "Blaze::ExampleNamespace::ExampleClass::ExampleClass(Blaze::ExampleNamespace::ExampleParameter *)",
            (const void*)0x7e535cc844f0).get(),
            "[ExampleClass:00007E535CC844F0].ExampleClass: "
        );

        // THIS_FUNC, constructor, Windows Visual C++
        ASSERT_STREQ(::Blaze::Logger::ThisFunction(
            "__cdecl Blaze::ExampleNamespace::ExampleClass::ExampleClass(class Blaze::ExampleNamespace::ExampleParameter *)",
            (const void*)0x7e535cc844f0).get(),
            "[ExampleClass:00007E535CC844F0].ExampleClass: "
        );

        // THIS_FUNC, function parameters that are template classes
        ASSERT_STREQ(::Blaze::Logger::ThisFunction(
            "Blaze::ExampleNamespace::ComplicatedClass::complicatedFunction(class EA::TDF::TdfStructVector<class Blaze::Framework::SomeTdf> &,const class eastl::hash_map<unsigned __int64,class Blaze::DbRow const *,struct eastl::hash<unsigned __int64>,struct eastl::equal_to<unsigned __int64>,class Blaze::BlazeStlAllocator,0> &) const",
            (const void*)0x7e535cc844f0).get(),
            "[ComplicatedClass:00007E535CC844F0].complicatedFunction: "
        );

        // THIS_FUNC, template class
        ASSERT_STREQ(::Blaze::Logger::ThisFunction(
            "typename T_CommandStub::Errors Blaze::TestNamespace::TemplatedCommand<T_CommandStub, T_Request, T_Response, testBool, anotherBool>::execute() [with T_CommandStub = Blaze::TestNamespace::Test_ExampleCommandStub; T_Request = Blaze::TestNamespace::ExampleRequest; T_Response = Blaze::TestNamespace::ExampleResponse; bool testBool = true; bool anotherBool = true; typename T_CommandStub::Errors = Blaze::TestNamespace::Test_ExampleError::Error]",
            (const void*)0x7e535cc844f0).get(),
            "[TemplatedCommand:00007E535CC844F0].execute: "
        );
    }

} //namespace Framework
} //namespace BlazeServerTest
