/******************************************************************************/
/*! 
    DesyncData
    Copyright 2010 Electronic Arts Inc.

    Version       1.0     02/25/2010 (mbaylis)
*/
/******************************************************************************/


/*** Includes *****************************************************************/
#include "BugSentry/desyncdata.h"

/*** Implementation ***********************************************************/
namespace EA
{
    namespace BugSentry
    {
        /******************************************************************************/
        /*! DesyncData :: DesyncData

            \brief  Constructor.

            \param           - none.
            \return          - void.
        */
        /******************************************************************************/
        DesyncData::DesyncData() : BugData(), mDesyncInfo(NULL), mCategoryId(NULL), mDesyncId(NULL)
        {
        }

        /******************************************************************************/
        /*! DesyncData :: DesyncData

            \brief  Constructor.

            \param  buildSignature  - Game provided build signature identifier.
            \param  sku             - The unique SKU identifier for this title, must match the BugSentry SKU and differ for each platform of this title.
            \param  sessionId       - The unique session identifier for this boot session.
            \return                 - void.
        */
        /******************************************************************************/
        DesyncData::DesyncData(const char* buildSignature, const char* sku, const char* sessionId) : BugData(buildSignature, sku, sessionId), mDesyncInfo(NULL), mCategoryId(NULL), mDesyncId(NULL)
        {
        }
    }
}
