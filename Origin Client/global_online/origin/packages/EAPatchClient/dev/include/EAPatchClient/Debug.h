///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/Debug.h
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHCLIENT_DEBUG_H
#define EAPATCHCLIENT_DEBUG_H


#include <EAPatchClient/Base.h>
#include <EAAssert/eaassert.h>


namespace EA
{
    namespace Patch
    {
        ///////////////////////////////////////////////////////////////////////
        /// RandomizeMemory
        ///
        /// Fills the given memory with random bytes. The randomness is not
        /// guaranteed to reproduce from run to run.
        ///
        void RandomizeMemory(void* p, size_t size);


        /// EAPATCH_ASSERT_ENABLED
        /// EAPATCH_ASSERT
        /// EAPATCH_ASSERT_MESSAGE
        /// EAPATCH_ASSERT_FORMATTED
        /// EAPATCH_FAIL_MESSAGE
        ///
        /// This matches EAAssert equivalents by default, though they can be redefined to 
        /// be something alternative.
        ///
        /// Example usage:
        ///     EAPATCH_ASSERT(ParseXMLFile(pXMLFilePath));
        ///     EAPATCH_ASSERT_MESSAGE(ParseXMLFile(pXMLFilePath), "EAPatchInfo: Failed to parse XML file.");
        ///     EAPATCH_ASSERT_FORMATTED("EAPatchInfo: Failed to parse XML file %s", pXMLFilePath);
        ///     EAPATCH_FAIL_MESSAGE("EAPatchInfo: Failed to parse XML file.");
        ///     #if EAPATCH_ASSERT_ENABLED // For the case that we need to create variables.
        ///         String sTemp;
        ///         sTemp.Sprintf("%s.", pXMLFilePath);
        ///         EAPATCH_ASSERT_FORMATTED("EAPatchInfo: Failed to parse XML file %s", sTemp.c_str());
        ///     #endif
        ///
        /// To do: Make these unilaterally do a trace before the assert. The programmer shouldn't have to 
        ///        redundantly make an EAPATCH_ASSERT_MESSAGE then EAPATCH_TRACE_MESSAGE call.
        /// To do: Make this header not directly use EA_ASSERT. The reason why is that we need all asserts
        ///        in this package to use our assertion function and must guaranteed that nobody is allowed
        ///        to mistakenly use EA_ASSERT. In practice we want to be able to use EA_ASSERT underneath.
        ///
        #if !defined(EAPATCH_ASSERT_ENABLED)
            #if defined(EA_DEBUG)
                #define EAPATCH_ASSERT_ENABLED 1
            #else
                #define EAPATCH_ASSERT_ENABLED 0
            #endif
        #endif

        #if !defined(EAPATCH_ASSERT)
            #if EAPATCH_ASSERT_ENABLED
                #define EAPATCH_ASSERT EA_ASSERT
            #else
                #define EAPATCH_ASSERT(...)
            #endif
        #endif

        #if !defined(EAPATCH_ASSERT_MESSAGE)
            #if EAPATCH_ASSERT_ENABLED
                #define EAPATCH_ASSERT_MESSAGE EA_ASSERT_MESSAGE
            #else
                #define EAPATCH_ASSERT_MESSAGE(...)
            #endif

        #endif

        #if !defined(EAPATCH_ASSERT_FORMATTED)
            #if EAPATCH_ASSERT_ENABLED
                #define EAPATCH_ASSERT_FORMATTED EA_ASSERT_FORMATTED
            #else
                #define EAPATCH_ASSERT_FORMATTED(...)
            #endif
        #endif

        #if !defined(EAPATCH_FAIL_MESSAGE)
            #if EAPATCH_ASSERT_ENABLED
                #define EAPATCH_FAIL_MESSAGE EA_FAIL_MESSAGE
            #else
                #define EAPATCH_FAIL_MESSAGE(...)
            #endif
        #endif

        #if !defined(EAPATCH_FAIL_FORMATTED)
            #if EAPATCH_ASSERT_ENABLED
                #define EAPATCH_FAIL_FORMATTED EA_FAIL_FORMATTED
            #else
                #define EAPATCH_FAIL_FORMATTED(...)
            #endif
        #endif




        /// EAPATCH_TRACE_ENABLED
        /// EAPATCH_TRACE_MESSAGE
        /// EAPATCH_TRACE_FORMATTED
        ///
        /// Implements tracing (debug printing) to a user-supplied callback function.
        /// 
        /// Example usage:
        ///     EAPATCH_TRACE_MESSAGE("EAPatchInfo: Failed to parse XML file.");
        ///     EAPATCH_TRACE_FORMATTED("EAPatchInfo: Failed to parse XML file %s.", pXMLFilePath);
        ///     #if EAPATCH_TRACE_ENABLED // For the case that we need to create variables.
        ///         String sTemp;
        ///         sTemp.Sprintf("%s.", pXMLFilePath);
        ///         EAPATCH_TRACE_FORMATTED("EAPatchInfo: Failed to parse XML file %s", sTemp.c_str());
        ///     #endif
        ///
        /// To consider: Add severity levels for debug tracing.
        ///
        #if !defined(EAPATCH_TRACE_ENABLED)
            #if (EAPATCH_DEBUG > 0)
                #define EAPATCH_TRACE_ENABLED 1
            #else
                #define EAPATCH_TRACE_ENABLED 0
            #endif
        #endif

        EAPATCHCLIENT_API void EAPATCH_TRACE_FORMATTED_IMP(const char* pFile, int nLine, const char* pFormat, ...);

        #if !defined(EAPATCH_TRACE_MESSAGE)
            #if EAPATCH_TRACE_ENABLED
                #define EAPATCH_TRACE_MESSAGE(pMsg) EAPATCH_TRACE_FORMATTED_IMP(__FILE__, __LINE__, "%s", pMsg)
            #else
                #define EAPATCH_TRACE_MESSAGE(...)
            #endif
        #endif

        #if !defined(EAPATCH_TRACE_FORMATTED)
            #if EAPATCH_TRACE_ENABLED
                #define EAPATCH_TRACE_FORMATTED(...) EAPATCH_TRACE_FORMATTED_IMP(__FILE__, __LINE__, __VA_ARGS__)
            #else
                #define EAPATCH_TRACE_FORMATTED(...)
            #endif
        #endif




        ///////////////////////////////////////////////////////////////////////
        /// DebugTraceInfo
        ///
        /// To consider: Use string objects instead of char pointers.
        ///
        struct DebugTraceInfo
        {
            const char8_t* mpFile;      // __FILE__, may be empty, but never NULL.
            int            mLine;       // __LINE__, has no meaning if mpFile or empty.
            const char8_t* mpText;      // Trace text. Should always be valid.
            int            mSeverity;   // Severify level. To do: Define formal severity levels.

            DebugTraceInfo();
        };


        ///////////////////////////////////////////////////////////////////////
        /// RegisterUserDebugTraceHandler
        ///
        /// This is used by the application to intercept debug trace calls from EAPatch.
        /// The handler may be called from a thread different than the current
        /// one, though the handler will not be called during a time when it 
        /// is unsafe to execute arbitrary application code, such as might be the 
        /// case within a Unix signal handler, for example.
        ///
        /// The DebugTraceInfo passed to the user is good only for the duration of 
        /// the callback. Any pointers to data cannot be saved and their contents
        /// need to be copied if you want to use them later.
        ///
        /// Currently DebugTraceHandlers are application-global. We can revise this
        /// to be EAPatch session-specific if necessary.
        ///
        /// Example usage:
        ///     class SomeClass : public DebugTraceHandler{
        ///         void Init()
        ///             { EA::Patch::RegisterDebugTraceHandler(this, NULL); }
        ///         
        ///         void HandleEAPatchDebugTrace(EA::Patch::DebugTraceInfo& info, intptr_t userContext)
        ///             { printf("%s":, info.mpText); }
        ///     };
        ///
        struct DebugTraceHandler
        {
            virtual ~DebugTraceHandler(){}
            virtual void HandleEAPatchDebugTrace(const DebugTraceInfo& info, intptr_t userContex) = 0;
        };

        EAPATCHCLIENT_API void               RegisterUserDebugTraceHandler(DebugTraceHandler* pDebugTraceHandler, intptr_t userContext);
        EAPATCHCLIENT_API DebugTraceHandler* GetUserDebugTraceHandler(intptr_t* pUserContext = NULL);


    } // namespace Patch

} // namespace EA



#endif // Header include guard



 




