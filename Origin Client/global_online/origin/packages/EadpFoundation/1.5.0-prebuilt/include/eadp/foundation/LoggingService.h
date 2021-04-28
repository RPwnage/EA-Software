// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>

#include <eadp/foundation/Hub.h>
#include <eadp/foundation/Callback.h>
#include <EAAssert/eaassert.h>
#include <string>
#include <vector>


/*!
 * @brief EADPSDK_LOG macro to help logging a message
 *
 * Game can define EADPSDK_DISABLE_LOGGING to void all the logging and remove all the overhead for performance,
 * but it will make debugging harder.
 */
#ifdef EADPSDK_DISABLE_LOGGING

#   define EADPSDK_LOGV(...)
#   define EADPSDK_LOGD(...)
#   define EADPSDK_LOGI(...)
#   define EADPSDK_LOGW(...)
#   define EADPSDK_LOGE(...)
#   define EADPSDK_LOGF(...)
#   define EADPSDK_LOG_VERSION(...)

#else // not define DISABLE_EADPSDK_LOGGING

#if !defined(EADPSDK_PACKAGE_NAME)
#define EADPSDK_PACKAGE_NAME \0
#endif

#   define _EADPSDK_LOG_T(hub, packageName, level, title, ...) (hub==nullptr)?eadp::foundation::LoggingService::logFailure(title, __VA_ARGS__):\
eadp::foundation::LoggingService::getService(hub)->writeMessage(packageName, eadp::foundation::ILoggingService::LogLevel::level, title, __VA_ARGS__) /*!< internal */

#   define STRINGIFY(x) #x
#   define EADPSDK_STRINGIFY(x) STRINGIFY(x)
#   define EADPSDK_LOGV(hub, title, ...) _EADPSDK_LOG_T(hub, EADPSDK_STRINGIFY(EADPSDK_PACKAGE_NAME), kVerbose, title, __VA_ARGS__)
#   define EADPSDK_LOGD(hub, title, ...) _EADPSDK_LOG_T(hub, EADPSDK_STRINGIFY(EADPSDK_PACKAGE_NAME), kDebug, title, __VA_ARGS__)
#   define EADPSDK_LOGI(hub, title, ...) _EADPSDK_LOG_T(hub, EADPSDK_STRINGIFY(EADPSDK_PACKAGE_NAME), kInfo, title, __VA_ARGS__)
#   define EADPSDK_LOGW(hub, title, ...) _EADPSDK_LOG_T(hub, EADPSDK_STRINGIFY(EADPSDK_PACKAGE_NAME), kWarn, title, __VA_ARGS__)
#   define EADPSDK_LOGE(hub, title, ...) _EADPSDK_LOG_T(hub, EADPSDK_STRINGIFY(EADPSDK_PACKAGE_NAME), kError, title, __VA_ARGS__)
#   define EADPSDK_LOGF(hub, title, ...) _EADPSDK_LOG_T(hub, EADPSDK_STRINGIFY(EADPSDK_PACKAGE_NAME), kFatal, title, __VA_ARGS__)
#   define EADPSDK_LOG_VERSION(hub, title, version) EADPSDK_LOGI(hub, title, "module version=%s", STRINGIFY(version));

#endif

namespace eadp
{
namespace foundation
{

/*!
 * @brief Logging service helps application to record key information of SDK and application running.
 *
 * The record information can be channel to different output device, including console, file and callback.
 */
class EADPSDK_API ILoggingService
{
public:
    /*!
     * @brief Log level
     */
    enum class LogLevel : uint8_t
    {
        ALL = 0, //!< threshold only, not for writeMessage
        kAll = ALL, //!< Deprecated use above value instead
        VERBOSE = 1, //!< The most verbose level available.
        kVerbose = VERBOSE, //!< Deprecated use above value instead
        DEBUG = 2,  //!< Debug level information, the second most verbose level.
        kDebug = DEBUG, //!< Deprecated use above value instead
        INFO = 4, //!< Info level - general useful information not related to warnings or errors.
        kInfo = INFO, //!< Deprecated use above value instead
        WARN = 8, //!< Warning level - information that could be the result of undesired behavior or bad data.
        kWarn = WARN, //!< Deprecated use above value instead
        ERR = 16, //!< Error level - information pertaining to errors in the application.
        kError = ERR, //!< Deprecated use above value instead
        FATAL = 32, //!< Error level - information pertaining to errors in the application from which we can't recover.
        kFatal = FATAL, //!< Deprecated use above value instead
        NONE = 255, //!< threshold only, not for writeMessage
        kNone = NONE, //!< Deprecated use above value instead
    };

    /*!
     * @brief The log message structure
     *
     * It is used in the logging service to wrap the log message, the logging callback will be trigger with it.
     */
    struct EADPSDK_API LogMessage
    {
    public:
        LogLevel level;
        String title;
        String message;
        Timestamp timestamp;

        /*!
         * @brief Log message constructor
         *
         * This will fill timestamp with the time of now.
         *
         * @param level The log level
         * @param title The log message title, normally is the class name or scope description
         * @param message The log message body
         */
        LogMessage(Allocator& allocator,
                   LogLevel level,
                   String title,
                   String message);

        /*!
         * @brief Convert the log message into string
         *
         * This will generate the string in "[{timestamp}] {level}>{title}>{message}" format
         *
         * @param withTimestamp If timestamp string is prefixed.
         */
        String format(Allocator& allocator, bool withTimestamp = true) const;

        /*!
         * @brief Convert the log level into corresponding string
         *
         * @param logLevel The log level enum to be convert into string
         @ @return String describes the log level
         */
        static String convertLogLevelToString(Allocator& allocator, ILoggingService::LogLevel logLevel);
    };

    /*!
     * @brief Logging callback signature for custom log message monitoring
     */
    using LoggingCallback = CallbackT<void(const LogMessage& message)>;

    /*!
     * @brief Virtual default destructor for base class
     */
    virtual ~ILoggingService() = default;

    /*!
     * @brief Record a logging message with title
     * 
     * [Deprecated] Use writeMessage api with packageName in the parameter
     * @param level The log level
     * @param title The log title, it will be suffixed to the message in the output
     * @param message The logging message with variant arguments.
     */
    virtual void writeMessage(LogLevel level, const char8_t* title, const char8_t* message, ...) = 0;

    /*!
     * @brief Record a logging message with title
     *
     * @param level The log level
     * @param packageName The name of the module the message is being logged from
     * @param title The log title, it will be suffixed to the message in the output
     * @param message The logging message with variant arguments.
     */
    virtual void writeMessage(const char8_t* packageName, LogLevel level, const char8_t* title, const char8_t* message, ...) = 0;

    /*!
     * @brief Record a logging message with title
     *
     * [Deprecated] Use writeMessage api with packageName in the parameter
     * @param level The log level
     * @param title The log title, it will be suffixed to the message in the output
     * @param message The logging message
     */
    virtual void writeMessage(LogLevel level, const char8_t* title, const String& message) = 0;

    /*!
     * @brief Record a logging message with title
     *
     * @param level The log level
     * @param packageName The name of the module the message is being logged from
     * @param title The log title, it will be suffixed to the message in the output
     * @param message The logging message
     */
    virtual void writeMessage(const char8_t* packageName, LogLevel level, const char8_t* title, const String& message) = 0;

    /*!
     * @brief Set global log level threshold
     *
     * If log level is not set for individual module, any log message with log level lower than this threshold will be ignored for all modules.
     */
    virtual void setThresholdLogLevel(LogLevel threshold) = 0;

    /*!
     * @brief Set log level threshold for an individual module. After a log level is set for individual module, any log message with log level lower than this threshold will be ignored for this module.
     *
     * @param level The log level
     * @param packageName The name of the module the threshold is being set for 
     */
    virtual void setThresholdLogLevel(const char8_t* packageName, LogLevel threshold) = 0;

    /*!
     * @brief Get global log level threshold for all modules
     *
     * If log level is not set for individual module, any log message with log level lower than this threshold will be ignored for all modules.
     *
     * @return uint8_t Log threshold level
     */
    virtual LogLevel getThresholdLogLevel() const = 0;

    /*!
     * @brief Get log level threshold for an individual module. After a log level is set for individual module, any log message with log level lower than this threshold will be ignored for this module.
     *
     * @param packageName The name of the module the threshold is being set for
     *
     * @return uint8_t Log threshold level
     */
    virtual LogLevel getThresholdLogLevel(const char8_t* packageName) const = 0;

    /*!
     * @brief Check if console logging is enabled or disabled.
     *
     * @return bool True if console logging is enabled; False if it is disabled.
     */
    virtual bool isConsoleLoggingEnabled() const = 0;

    /*!
     * @brief Enable/Disable console logging
     *
     * @param enable Boolean that specifies whether to enable or disable the console logging.
     */
    virtual void enableConsoleLogging(bool enable) = 0;

    /*!
     * @brief Set log file path
     *
     * The log file is used to persist all valid log messages, so user can check the logs later.
     * Any log message with log level lower than the threshold will be skipped and not shown in
     * the logging file.
     *
     * @param path Set log file path to empty string will disable file logging; otherwise it will
     * save log messages to the file specified.
     */
    virtual void setLogFilePath(const char8_t* path) = 0;

    /*!
     * @brief Get log file path
     *
     * @see setLoggingFilePath for details about file logging.
     *
     * @return String The log file path; empty string means the file logging is disabled.
     */
    virtual String getLogFilePath() const = 0;

    /*!
     * @brief Add a callback to listen to log message.
     *
     * The callback will be triggered each time a new message is logged, so please keep the
     * implementation simple and quick.
     * Any log message with log level lower than the threshold will be skipped and will not
     * trigger callback.
     * Duplicate call to this API will lead to callback get triggered multiple times.
     *
     * @param callback A new callback.
     */
    virtual void addLoggingCallback(const LoggingCallback& callback) = 0;

protected:
    ILoggingService() = default;

    EA_NON_COPYABLE(ILoggingService);
};

/*!
 * @brief The logging service access class
 *
 * It helps to get semi-singleton logging service instance on Hub, so other service can access easily;
 * it also simplifies the logging macros
 */
class EADPSDK_API LoggingService
{
public:
    /*!
     * @brief Get the logging service instance
     *
     * The service will be available when the Hub is live.
     *
     * @param hub The hub provides the logging service.
     * @return The ILoggingService instance
     */
    static ILoggingService* getService(IHub* hub);
    
    static void logFailure(const char8_t* title, const char8_t* message, ...);
    
    static void logFailure(const char8_t* title, const String& message);
};

}
}


