/*************************************************************************************************/
/*!
\file mocklogger.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SERVER_TEST_MOCKLOGGER_H
#define BLAZE_SERVER_TEST_MOCKLOGGER_H

#include "framework/blazedefines.h"
#include "framework/util/shared/stringbuilder.h"
#include "framework/tdf/logging.h"


#include <gmock/gmock.h>


namespace BlazeServerTest
{

// This mock class doesn't actually inherit from Logger.
// Due to Logger's static nature, we rely on our #defines to abstract logging from our testing.
class MockLogger
{

public:
    MockLogger() {}

    MOCK_METHOD3(log, void(int32_t category, Blaze::Logging::Level level, Blaze::StringBuilder& message));

    Blaze::StringBuilder currentMessage;
};

} // namespace BlazeServerTest

static BlazeServerTest::MockLogger gMockLogger;

/* !!!!!!!!!!!!!!!
 Developer Note:
 This doesn't actually work.  Framework builds slntest in a way that builds the main sln (blazeserver) before the test sln, and the links in the main as a libary.
 So, the included Blaze code will be preprocessed w/o these defines, and logging will still go through the logger.
 We define them here now for compiling purposes only.  This prevents the need to include framework/logger.h all over our test code.
 This means that default Blazeserver logging will occur during the test, and logs will be written to STDOUT
 Options going forward
  * Call Logger::configure and set stout to false, and log to a file.
  * Refactor logger to not be static
  * Move our tests out of slntest, and just make it its own sln.
  !!!!!!!!!!!!!! */

#ifdef BLAZE_COMPONENT_TYPE_INDEX_NAME
#define LOGGER_CATEGORY ::Blaze::BLAZE_COMPONENT_TYPE_INDEX_NAME
#else
#define LOGGER_CATEGORY ::Blaze::Log::SYSTEM
#endif

#undef WARN_LOG
#define WARN_LOG(message) \
        { \
             gMockLogger.log(LOGGER_CATEGORY, Blaze::Logging::WARN, gMockLogger.currentMessage.reset() << message); \
        }

#undef ERR_LOG
#define ERR_LOG(message) \
        { \
            gMockLogger.log(LOGGER_CATEGORY, Blaze::Logging::ERR, gMockLogger.currentMessage.reset() << message); \
        }

#undef SPAM_LOG
#define SPAM_LOG(message) \
        { \
            gMockLogger.log(LOGGER_CATEGORY, Blaze::Logging::SPAM, gMockLogger.currentMessage.reset() << message); \
        }

#undef TRACE_LOG
#define TRACE_LOG(message) \
        { \
            gMockLogger.log(LOGGER_CATEGORY, Blaze::Logging::TRACE, gMockLogger.currentMessage.reset() << message); \
        }

#undef INFO_LOG
#define INFO_LOG(message) \
        { \
            gMockLogger.log(LOGGER_CATEGORY, Blaze::Logging::INFO, gMockLogger.currentMessage.reset() << message); \
        }

#undef FAIL_LOG
#define FAIL_LOG(message) \
        { \
            gMockLogger.log(LOGGER_CATEGORY, Blaze::Logging::FAIL, gMockLogger.currentMessage.reset() << message); \
        }

#undef ASSERT_LOG
#define ASSERT_LOG(message) \
        { \
            gMockLogger.log(LOGGER_CATEGORY, Blaze::Logging::ASSERT, gMockLogger.currentMessage.reset() << message); \
        }

#undef ASSERT_COND_LOG
#define ASSERT_COND_LOG(expr, message) \
        { \
            if (!(expr))\
                 gMockLogger.log(LOGGER_CATEGORY, Blaze::Logging::ASSERT_COND_LOG, gMockLogger.currentMessage.reset() << message); \
        }

#undef FATAL_LOG
#define FATAL_LOG(message) \
        { \
            gMockLogger.log(LOGGER_CATEGORY, Blaze::Logging::FATAL, gMockLogger.currentMessage.reset() << message); \
        }

// force the mock logging to always be on.
#ifndef IS_LOGGING_ENABLED
#define IS_LOGGING_ENABLED(level) true
#endif




#endif
