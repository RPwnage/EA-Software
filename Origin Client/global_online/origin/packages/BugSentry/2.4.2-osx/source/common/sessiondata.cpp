/******************************************************************************/
/*! 
    CrashData
    Copyright 2010 Electronic Arts Inc.

    Version       1.0     04/05/2010 (mbaylis)
*/
/******************************************************************************/


/*** Includes *****************************************************************/
#include "BugSentry/sessiondata.h"
#include "BugSentry/bugsentrymgrbase.h"
#include "EAStdC/EAMemory.h"

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        /******************************************************************************/
        /*! SessionData :: SessionData

            \brief  Constructor.

            \param           - none.
            \return          - void.
        */
        /******************************************************************************/
        SessionData::SessionData() : BugData()
        {
            Init();
        }

        /******************************************************************************/
        /*! SessionData :: SessionData

            \brief  Constructor.

            \param  buildSignature  - Game provided build signature identifier.
            \param  sku             - The unique SKU identifier for this title, must match the BugSentry SKU and differ for each platform of this title.
            \return                 - void.
        */
        /******************************************************************************/
        SessionData::SessionData(const char* buildSignature, const char* sku, const char* sessionId) : BugData(buildSignature, sku, sessionId)
        {
            Init();
        }

        /******************************************************************************/
        /*! SessionData :: Init

            \brief  Initializes member variables to 0.

            \param          - none.
            \return         - void.
        */
        /******************************************************************************/
        void SessionData::Init()
        {
            mSessionType = SESSION_INVALID;
        }
    }
}
