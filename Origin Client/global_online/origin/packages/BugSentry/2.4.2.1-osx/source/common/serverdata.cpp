/******************************************************************************/
/*! 
    ServerData
    Copyright 2010 Electronic Arts Inc.

    Version       1.0     04/05/2010 (mbaylis)
*/
/******************************************************************************/


/*** Includes *****************************************************************/
#include "BugSentry/serverdata.h"
#include "BugSentry/bugsentrymgrbase.h"

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        /******************************************************************************/
        /*! ServerData :: ServerData

            \brief  Constructor.

            \param           - none.
            \return          - void.
        */
        /******************************************************************************/
        ServerData::ServerData() : BugData()
        {
            Init();
        }

        /******************************************************************************/
        /*! ServerData :: ServerData

            \brief  Constructor.

            \param  buildSignature  - Game provided build signature identifier.
            \param  sku             - The unique SKU identifier for this title, must match the BugSentry SKU and differ for each platform of this title.
            \param  sessionId       - The unique session identifier for this boot session.
            \return                 - void.
        */
        /******************************************************************************/
        ServerData::ServerData(const char* buildSignature, const char* sku, const char* sessionId) : BugData(buildSignature, sku, sessionId)
        {
            Init();
        }

        /******************************************************************************/
        /*! ServerData :: Init

            \brief  Initializes member variables.

            \param          - none.
            \return         - void.
        */
        /******************************************************************************/
        void ServerData::Init()
        {
            mCategoryId = NULL;
            mServerName = NULL;
            mServerType = NULL;
            mServerErrorType = SERVER_ERROR_TYPE_INVALID;
            mContextData = NULL;
        }
    }
}
