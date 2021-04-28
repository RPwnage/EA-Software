//*! *************************************************************************************************/
/*!
    \file debug.h


    \attention
       (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/


#ifndef BLAZE_DEBUG_H
#define BLAZE_DEBUG_H

#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/blazesdk.h"
#endif

namespace Blaze
{


/*! ***************************************************************************/
/*!     
    \brief Provides logging and assert/verify support for Blaze SDK classes.
*******************************************************************************/
class Debug
{
public:

    virtual ~Debug() {}

    // TODO: we probably need to support a hook per api instance, so static functions don't really work
    // TODO: rename to lower case functions
    // TODO: implement the verify macros below

    typedef void (*LogFunction)(const char8_t *pText, void *data);

    /*! ***************************************************************************/
    /*!     
        \brief Sets the function to use to output debug messages.
        \param hook   The function to use to output debug messages.
        \param data   An optional data pointer to be passed back to the logging function.
    *****************************************************************************/
    static void SetLoggingHook(LogFunction hook, void *data);   

    /*! ***************************************************************************/
    /*!     
        \brief Logs a debug message.
        \param msg  The debug message to log.
    *****************************************************************************/
    static void Log(const char8_t *msg);

    
    /*! ***************************************************************************/
    /*!     
        \brief Logs a formatted debug message.
        \param msg   The format string to log.  
    *****************************************************************************/
    static void Logf(const char8_t *msg, ...);
    

    typedef void (*AssertFunction)(bool assertValue, const char8_t *assertLiteral, const char8_t *file, size_t line, void* data);

    /*! ***************************************************************************/
    /*!     
    \brief Sets the function to use to output assert messages. If SetAssertHook is not used, 
            there is a default AssertFunction implementation that simply logs an assertation failure message. 
    \param hook   The function to use to output assert messages. Can be set to nullptr to suppress all blaze Asserts.
    \param data   An optional data pointer to be passed back to the assert function.
    *****************************************************************************/
    static void SetAssertHook(AssertFunction hook, void *data); 

    /*! ************************************************************************************************/
    /*! \brief Calls the current AssertHook function. The default AssertHook function simply logs the message.
                This should not be called directly, instead, use the BlazeAssert macro.

    \param[in] assertValue the value to assert on
    \param[in] assertLiteral the string-ified assertion value
    \param[in] file the file the assertion was thrown from
    \param[in] line the line the assertion was thrown from
    ***************************************************************************************************/
    static void Assert(bool assertValue, const char8_t *assertLiteral, const char8_t *file, size_t line);

private:
    /*! ************************************************************************************************/
    /*! \brief The default assert function simply logs an assertion failure message.

    \param[in] the value to assert on
    \param[in] assertLiteral the string-ified assertion value
    \param[in] the file the assertion was thrown from
    \param[in] the line the assertion was thrown from
    ***************************************************************************************************/
    static void DefaultAssertFunction(bool assertValue, const char8_t *assertLiteral, const char8_t *file, size_t line, void *data);

    static AssertFunction sAssertFunction;
    static void* sAssertData;
};


#ifdef _DEBUG
// NOTE: it's inefficient to always call the asserter function, but it prevents error C4127 on windows (if statement with constant value)
//  the assert function disables the warning, but I don't know of a way to disable or reenable it from inside a macro
#define BlazeAssert(x) { Blaze::Debug::Assert(x, #x, __FILE__, __LINE__); }
#else
#define BlazeAssert(x)
#endif // _DEBUG

#define BlazeVerify(x) 
#define BlazeVerifyNoErr(x) 

}       // namespace Blaze

#endif  // BLAZE_DEBUG_H
