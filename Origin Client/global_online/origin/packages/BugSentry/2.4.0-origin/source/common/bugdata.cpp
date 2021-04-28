/******************************************************************************/
/*! 
    BugData
    Copyright 2010 Electronic Arts Inc.

    Version       1.0     07/16/2009 (gsharma)
*/
/******************************************************************************/

/*** Includes *****************************************************************/
#include "BugSentry/bugdata.h"
#include "EAStdC/EAMemory.h"
#include "EAStdC/EAString.h"

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        /******************************************************************************/
        /*! BugData :: BugData

            \brief  Constructor.
        */
        /******************************************************************************/
        BugData::BugData() : mBuildSignature(NULL), mSku(NULL)
        {
            EA::StdC::Memset8(mSessionId, 0, sizeof(mSessionId));
        }

        /******************************************************************************/
        /*! BugData :: BugData

            \brief  Constructor.

            \param  buildSignature - Game provided build signature identifier.
            \param  sku         - The unique SKU identifier for this title, must match the BugSentry SKU and differ for each platform of this title.
            \param  sessionId   - The session ID for this instance of BugSentry.
            \return             - void.
        */
        /******************************************************************************/
        BugData::BugData(const char* buildSignature, const char* sku, const char* sessionId) : mBuildSignature(buildSignature), mSku(sku)
        {
            EA::StdC::Memset8(mSessionId, 0, sizeof(mSessionId));
            EA::StdC::Strlcpy(mSessionId, sessionId, sizeof(mSessionId));
        }
    }
}
