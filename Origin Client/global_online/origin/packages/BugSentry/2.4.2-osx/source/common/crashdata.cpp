/******************************************************************************/
/*! 
    CrashData
    Copyright 2010 Electronic Arts Inc.

    Version       1.0     07/16/2009 (gsharma)
*/
/******************************************************************************/


/*** Includes *****************************************************************/
#include "BugSentry/crashdata.h"
#include "EAStdC/EAMemory.h"

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        /******************************************************************************/
        /*! CrashData :: CrashData

            \brief  Constructor.

            \param           - none.
            \return          - void.
        */
        /******************************************************************************/
        CrashData::CrashData() : BugData()
        {
            Init();
        }

        /******************************************************************************/
        /*! CrashData :: CrashData

            \brief  Constructor.

            \param  buildSignature  - Game provided build signature identifier.
            \param  sku             - The unique SKU identifier for this title, must match the BugSentry SKU and differ for each platform of this title.
            \return                 - void.
        */
        /******************************************************************************/
        CrashData::CrashData(const char* buildSignature, const char* sku, const char* sessionId) : BugData(buildSignature, sku, sessionId)
        {
            Init();
        }

        /******************************************************************************/
        /*! CrashData :: Init

            \brief  Initializes member variables.

            \param          - none.
            \return         - void.
        */
        /******************************************************************************/
        void CrashData::Init()
        {
            EA::StdC::Memset8(mStackAddresses, 0, sizeof(mStackAddresses));
            mExceptionInformation = NULL;
            mExceptionCode = 0;
            mStackAddressesCount = 0;
            mContextData = NULL;
            mCategoryId = NULL;
        }
    }
}
