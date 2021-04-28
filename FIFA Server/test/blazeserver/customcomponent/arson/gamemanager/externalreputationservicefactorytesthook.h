/*************************************************************************************************/
/*!
    \file   externalreputationservicefactorytesthook.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_ARSONCOMPONENT_EXTERNALREPUTATIONSERVICEFACTORY_HOOK_H
#define BLAZE_ARSONCOMPONENT_EXTERNALREPUTATIONSERVICEFACTORY_HOOK_H

#include "framework/usersessions/reputationserviceutilfactory.h"
#include "framework/usersessions/reputationserviceutil.h"

namespace Blaze
{
namespace ReputationService
{
    class TestHook
    {
    protected:
        bool getReputationUtil;
        Blaze::ReputationService::ReputationServiceFactory mReputationServiceFactory;

        TestHook()
        {
            if(mReputationServiceFactory.createReputationExpression() && mReputationServiceFactory.createReputationServiceUtil())
            {
                getReputationUtil = true;
            }
            else
            {
                getReputationUtil = false;
                BLAZE_ERR_LOG(Log::USER, "ArsonComponent.externalreputationservicefactorytesthook: failed to create the reputation service util.");     
            }
        }

        ~TestHook()
        {
            mReputationServiceFactory.destroyReputationExpression();
            mReputationServiceFactory.destroyReputationServiceUtil();
        }

        /*! \brief enable testing of the xbox reputation support */
        Blaze::ReputationService::ReputationServiceUtilPtr getReputationServiceUtil() 
        {            
            return mReputationServiceFactory.getReputationServiceUtil();
        }
    };
}
}// Blaze

#endif // BLAZE_ARSONCOMPONENT_EXTERNALSERVICES_HOOK_H

