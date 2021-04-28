/////////////////////////////////////////////////////////////////////////////
// EATraceExtra.h
//
// Copyright (c) 2003, Electronic Arts Inc. All rights reserved.
//
// This file implements extensions of the basic trace and assert facilities
// defined in eatrace.h. These extensions include variations that let users
// specify group and level output.
//
// The names selected were chosen because they are in line with what is 
// most often used in the PC programming (particularly Microsoft) world. 
// The Linux/GCC world has implemented similar facilities but hasn't 
// really seemed to find a consistent naming scheme. Thus we stick with
// the Microsoft/PC-like conventions.
//
// Created by Paul Pedriana, Maxis
/////////////////////////////////////////////////////////////////////////////


#ifndef EATRACE_EATRACEEXTRA_H
#define EATRACE_EATRACEEXTRA_H

// For now just enable EATrace.h functionality only. We may kill EATraceExtra.
#ifndef EATRACE_EATRACE_H
    #include <EATrace/EATrace.h>
#endif



/* To consider: Eliminate this file and use log statements instead if you need group/level specification.

#ifndef INCLUDED_eabase_H
    #include "EABase/eabase.h"
#endif
#ifndef EATRACE_EATRACE_H
    #include "EATrace/EATrace.h"
#endif


#if EA_TRACE_SYSTEM_ENABLED


#if EA_TRACE_ENABLED
    #define EA_TRACE_LEVEL(level, pMessage)    EA_TRACE_GROUP_LEVEL(NULL, level, pMessage)
    #define EA_TRACE_GROUP(pGroup, pMessage)  EA_TRACE_GROUP_LEVEL(pGroup, EA::Trace::kLevelDebug, pMessage)
    void     EA_TRACE_GROUP_LEVEL(const char* pGroup, int level, const char* pMessage);

    #define EA_TRACE_LEVEL_FORMATTED(level, pFormatAndArguments)                                        // To do.
    #define EA_TRACE_GROUP_FORMATTED(pGroup, pFormatAndArguments)                                      // To do.
    #define EA_TRACE_GROUP_LEVEL_FORMATTED(pGroup, level, pFormatAndArguments)                     // To do.

    #define EA_WARN_LEVEL(expression, level, pMessage)    EA_WARN_GROUP_LEVEL(expression, NULL, level, pMessage)
    #define EA_WARN_GROUP(expression, pGroup, pMessage)  EA_WARN_GROUP_LEVEL(expression, pGroup, EA::Trace::kLevelDebug, pMessage)
    void     EA_WARN_GROUP_LEVEL(bool bExpression, const char* pGroup, int level, const char* pMessage);

    #define EA_WARN_LEVEL_FORMATTED(bExpression, level, pFormatAndArguments)                        // To do.
    #define EA_WARN_GROUP_FORMATTED(bExpression, pGroup, pFormatAndArguments)                      // To do.
    #define EA_WARN_GROUP_LEVEL_FORMATTED(bExpression, pGroup, level, pFormatAndArgumentsts)  // To do.

    #define EA_NOTIFY_LEVEL             EA_WARN_LEVEL          // Note that we intentionally map NOTIFY to WARN here.
    #define EA_NOTIFY_GROUP             EA_WARN_GROUP
    #define EA_NOTIFY_GROUP_LEVEL     EA_WARN_GROUP_LEVEL

    #define EA_NOTIFY_LEVEL_FORMATTED(bExpression, level, pFormatAndArguments)                     // To do.
    #define EA_NOTIFY_GROUP_FORMATTED(bExpression, pGroup, pFormatAndArguments)                    // To do.
    #define EA_NOTIFY_GROUP_LEVEL_FORMATTED(bExpression, pGroup, level, pFormatAndArguments)  // To do.

#else // EA_TRACE_ENABLED == 0

    #define EA_TRACE_LEVEL(level, message)                                     // Nothing
    #define EA_TRACE_GROUP(group, message)                                     // Nothing
    #define EA_TRACE_GROUP_LEVEL(group, level, message)                    // Nothing

    #define EA_TRACE_LEVEL_FORMATTED                                             EA_SWALLOW_ARGS2
    #define EA_TRACE_GROUP_FORMATTED                                             EA_SWALLOW_ARGS2
    #define EA_TRACE_GROUP_LEVEL_FORMATTED                                     EA_SWALLOW_ARGS3

    #define EA_WARN_LEVEL(expression, level, message)                      // Nothing
    #define EA_WARN_GROUP(expression, group, message)                      // Nothing
    #define EA_WARN_GROUP_LEVEL(expression, group, level, message)     // Nothing

    #define EA_WARN_LEVEL_FORMATTED                                              EA_SWALLOW_ARGS3
    #define EA_WARN_GROUP_FORMATTED                                              EA_SWALLOW_ARGS3
    #define EA_WARN_GROUP_LEVEL_FORMATTED                                      EA_SWALLOW_ARGS4

    #define EA_NOTIFY_LEVEL(expression, level, message)                    if(true) { (void)(expression); } else // Recall that NOTIFY is required to always evaluate the test expression.
    #define EA_NOTIFY_GROUP(expression, group, message)                    if(true) { (void)(expression); } else
    #define EA_NOTIFY_GROUP_LEVEL(expression, group, level, message)  if(true) { (void)(expression); } else

    #define EA_NOTIFY_LEVEL_FORMATTED(expression, level, pFormatAndArguments)                    if(true) { (void)(expression); } else // Recall that NOTIFY is required to always evaluate the test expression.
    #define EA_NOTIFY_GROUP_FORMATTED(expression, group, pFormatAndArguments)                    if(true) { (void)(expression); } else 
    #define EA_NOTIFY_GROUP_LEVEL_FORMATTED(expression, group, level, pFormatAndArguments)  if(true) { (void)(expression); } else 

#endif // TRACE_ENABLED

// Abbreviations
#define EA_TRACE_L        EA_TRACE_LEVEL
#define EA_TRACE_G        EA_TRACE_GROUP
#define EA_TRACE_GL      EA_TRACE_GROUP_LEVEL
#define EA_TRACE_LF      EA_TRACE_LEVEL_FORMATTED
#define EA_TRACE_GF      EA_TRACE_GROUP_FORMATTED
#define EA_TRACE_GLF     EA_TRACE_GROUP_LEVEL_FORMATTED

#define EA_WARN_L         EA_WARN_LEVEL
#define EA_WARN_G         EA_WARN_GROUP
#define EA_WARN_GL        EA_WARN_GROUP_LEVEL
#define EA_WARN_LF        EA_WARN_LEVEL_FORMATTED
#define EA_WARN_GF        EA_WARN_GROUP_FORMATTED
#define EA_WARN_GLF      EA_WARN_GROUP_LEVEL_FORMATTED

#define EA_NOTIFY_L      EA_NOTIFY_LEVEL
#define EA_NOTIFY_G      EA_NOTIFY_GROUP_LEVEL
#define EA_NOTIFY_GL     EA_NOTIFY_GROUP_LEVEL
#define EA_NOTIFY_LF     EA_NOTIFY_LEVEL_FORMATTED
#define EA_NOTIFY_GF     EA_NOTIFY_GROUP_FORMATTED
#define EA_NOTIFY_GLF    EA_NOTIFY_GROUP_LEVEL_FORMATTED



#if EA_TRACE_ENABLED
    // To do:
    #define EA_ASSERT_LEVEL(expression, level)
    #define EA_ASSERT_GROUP(expression, group)
    #define EA_ASSERT_GROUP_LEVEL(expression, group, level)

    #define EA_ASSERT_LEVEL_MESSAGE(expression, level, message)
    #define EA_ASSERT_GROUP_MESSAGE(expression, group, message)
    #define EA_ASSERT_GROUP_LEVEL_MESSAGE(expression, group, level, message)

    #define EA_ASSERT_LEVEL_FORMATTED(expression, level, pFormatAndArguments)
    #define EA_ASSERT_GROUP_FORMATTED(expression, group, pFormatAndArguments)
    #define EA_ASSERT_GROUP_LEVEL_FORMATTED(expression, group, level, pFormatAndArguments)

    // To do:
    #define EA_VERIFY_LEVEL(expression, level)
    #define EA_VERIFY_GROUP(expression, group)
    #define EA_VERIFY_GROUP_LEVEL(expression, group, level)

    #define EA_VERIFY_LEVEL_MESSAGE(expression, level, message)
    #define EA_VERIFY_GROUP_MESSAGE(expression, group, message)
    #define EA_VERIFY_GROUP_LEVEL_MESSAGE(expression, group, level, message)

    #define EA_VERIFY_LEVEL_FORMATTED(expression, level, pFormatAndArguments)
    #define EA_VERIFY_GROUP_FORMATTED(expression, group, pFormatAndArguments)
    #define EA_VERIFY_GROUP_LEVEL_FORMATTED(expression, group, level, pFormatAndArguments)

    // To do:
    #define EA_FAIL_LEVEL(level)
    #define EA_FAIL_GROUP(group)
    #define EA_FAIL_GROUP_LEVEL(group, level)

    #define EA_FAIL_LEVEL_MESSAGE(level, message)
    #define EA_FAIL_GROUP_MESSAGE(group, message)
    #define EA_FAIL_GROUP_LEVEL_MESSAGE(group, level, message)

    #define EA_FAIL_LEVEL_FORMATTED(level, pFormatAndArguments)
    #define EA_FAIL_GROUP_FORMATTED(group, pFormatAndArguments)
    #define EA_FAIL_GROUP_LEVEL_FORMATTED(group, level, pFormatAndArguments)

#else // EA_TRACE_ENABLED == 0

    #define EA_ASSERT_LEVEL(expression, level)                    // Nothing
    #define EA_ASSERT_GROUP(expression, group)                    // Nothing
    #define EA_ASSERT_GROUP_LEVEL(expression, group, level)  // Nothing

    #define EA_ASSERT_LEVEL_MESSAGE(expression, level, message)                     // Nothing
    #define EA_ASSERT_GROUP_MESSAGE(expression, group, message)                     // Nothing
    #define EA_ASSERT_GROUP_LEVEL_MESSAGE(expression, group, level, message)    // Nothing

    #define EA_ASSERT_LEVEL_FORMATTED                                EA_SWALLOW_ARGS2
    #define EA_ASSERT_GROUP_FORMATTED                                EA_SWALLOW_ARGS2
    #define EA_ASSERT_GROUP_LEVEL_FORMATTED                        EA_SWALLOW_ARGS3

    #define EA_VERIFY_LEVEL(expression, level)                    if(true) { (void)(expression); } else // Recall that VERIFY is required to always evaluate the test expression.
    #define EA_VERIFY_GROUP(expression, group)                    if(true) { (void)(expression); } else
    #define EA_VERIFY_GROUP_LEVEL(expression, group, level)  if(true) { (void)(expression); } else

    #define EA_VERIFY_LEVEL_MESSAGE(expression, level, message)                     if(true) { (void)(expression); } else
    #define EA_VERIFY_GROUP_MESSAGE(expression, group, message)                     if(true) { (void)(expression); } else
    #define EA_VERIFY_GROUP_LEVEL_MESSAGE(expression, group, level, message)    if(true) { (void)(expression); } else

    #define EA_VERIFY_LEVEL_FORMATTED(expression, level, pFormatAndArguments)                  if(true) { (void)(expression); } else
    #define EA_VERIFY_GROUP_FORMATTED(expression, group, pFormatAndArguments)                  if(true) { (void)(expression); } else
    #define EA_VERIFY_GROUP_LEVEL_FORMATTED(expression, group, level, pFormatAndArguments) if(true) { (void)(expression); } else

    #define EA_FAIL_LEVEL(level)                                          // Nothing
    #define EA_FAIL_GROUP(group)                                          // Nothing
    #define EA_FAIL_GROUP_LEVEL(group, level)                         // Nothing

    #define EA_FAIL_LEVEL_MESSAGE(level, message)                    // Nothing
    #define EA_FAIL_GROUP_MESSAGE(group, message)                    // Nothing
    #define EA_FAIL_GROUP_LEVEL_MESSAGE(group, level, message)  // Nothing

    #define EA_FAIL_LEVEL_FORMATTED(level, pFormatAndArguments)                  // Nothing
    #define EA_FAIL_GROUP_FORMATTED(group, pFormatAndArguments)                  // Nothing
    #define EA_FAIL_GROUP_LEVEL_FORMATTED(group, level, pFormatAndArguments) // Nothing


#endif // EA_TRACE_ENABLED


// Abbreviations
#define EA_ASSERT_L      EA_ASSERT_LEVEL
#define EA_ASSERT_G      EA_ASSERT_GROUP
#define EA_ASSERT_GL     EA_ASSERT_GROUP_LEVEL

#define EA_ASSERT_LM     EA_ASSERT_LEVEL_MESSAGE
#define EA_ASSERT_GM     EA_ASSERT_GROUP_MESSAGE
#define EA_ASSERT_GLM    EA_ASSERT_GROUP_LEVEL_MESSAGE

#define EA_ASSERT_LF     EA_ASSERT_LEVEL_FORMATTED
#define EA_ASSERT_GF     EA_ASSERT_GROUP_FORMATTED
#define EA_ASSERT_GLF    EA_ASSERT_GROUP_LEVEL_FORMATTED

#define EA_VERIFY_L      EA_VERIFY_LEVEL
#define EA_VERIFY_G      EA_VERIFY_GROUP
#define EA_VERIFY_GL     EA_VERIFY_GROUP_LEVEL

#define EA_VERIFY_LM     EA_VERIFY_LEVEL_MESSAGE
#define EA_VERIFY_GM     EA_VERIFY_GROUP_MESSAGE
#define EA_VERIFY_GLM    EA_VERIFY_GROUP_LEVEL_MESSAGE

#define EA_VERIFY_LF     EA_VERIFY_LEVEL_FORMATTED
#define EA_VERIFY_GF     EA_VERIFY_GROUP_FORMATTED
#define EA_VERIFY_GLF    EA_VERIFY_GROUP_FORMATTED

#define EA_FAIL_L         EA_FAIL_LEVEL
#define EA_FAIL_G         EA_FAIL_GROUP
#define EA_FAIL_GL        EA_FAIL_GROUP_LEVEL

#define EA_FAIL_LM        EA_FAIL_LEVEL_MESSAGE
#define EA_FAIL_GM        EA_FAIL_GROUP_MESSAGE
#define EA_FAIL_GLM      EA_FAIL_GROUP_LEVEL_MESSAGE

#define EA_FAIL_LF        EA_FAIL_LEVEL_FORMATTED
#define EA_FAIL_GF        EA_FAIL_GROUP_FORMATTED
#define EA_FAIL_GLF      EA_FAIL_GROUP_LEVEL_FORMATTED


#endif // EA_TRACE_SYSTEM_ENABLED

*/

#endif // EATRACE_EATRACEEXTRA_H
