/******************************************************************************/
/*!
    BugSentryMgr

    Copyright 2010 Electronic Arts Inc.

    Version       1.0     04/09/2010 (mbaylis)
*/
/******************************************************************************/

/*** Include guard ************************************************************/
#ifndef BUGSENTRYMGR_H
#define BUGSENTRYMGR_H

/*** Includes *****************************************************************/
#include "BugSentry/bugsentrymgrbase.h"

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        class BugSentryMgr : public BugSentryMgrBase
        {
        public:
            BugSentryMgr(BugSentryConfig* config);
            virtual ~BugSentryMgr() {};

            virtual void ReportCrash(void* exceptionPointers, unsigned int exceptionCode, const char* contextData, const ImageInfo* imageInfo, const char* categoryId = NULL);
            virtual void ReportCrash(void* exceptionPointers, unsigned int exceptionCode, const char* contextData, const char* categoryId = NULL);
            virtual void ReportDesync(const char* categoryId, const char* desyncId, const char* desyncInfo, const ImageInfo* imageInfo);
            virtual void ReportDesync(const char* categoryId, const char* desyncId, const char* desyncInfo);

        private:
            virtual void CreateSessionId(char* sessionId);

            const ImageInfo* mImageInfo;
        };
    }
}

#endif // BUGSENTRYMGR_H
