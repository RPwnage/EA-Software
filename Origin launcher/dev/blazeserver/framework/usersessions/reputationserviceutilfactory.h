/*! ************************************************************************************************/
/*!
    \file reputationserviceutilfactory.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef COMPONENT_REPUTATION_SERVICE_UTIL_FACTORY_H
#define COMPONENT_REPUTATION_SERVICE_UTIL_FACTORY_H

#include "blazerpcerrors.h"
#include "EASTL/intrusive_ptr.h"
#include "framework/usersessions/reputationserviceutil.h"
#include "framework/util/expression.h"

namespace Blaze
{
    namespace ReputationService
    {
        class TestHook;
        typedef eastl::intrusive_ptr<ReputationServiceUtil> ReputationServiceUtilPtr;

        class ReputationServiceFactory
        {
            public:
                ReputationServiceFactory() :
                  mRealServiceCreated(false),
                  mReputationServiceUtil(nullptr),
                  mReputationExpression(nullptr)
                {}

                ~ReputationServiceFactory();

                ReputationService::ReputationServiceUtilPtr getReputationServiceUtil() { return mReputationServiceUtil; }
                
                // Returns false if any non-mock reputation service util exists:
                bool isReputationDisabled() { return !mRealServiceCreated;  }

                bool createReputationServiceUtil();
                bool createReputationExpression();

                void destroyReputationServiceUtil();
                void destroyReputationExpression();

                const Expression* getReputationExpression() const { return mReputationExpression; }

            private:
                


                static void resolveReputationType(const char8_t* nameSpace, const char8_t* name, void* context, Blaze::ExpressionValueType& type);

                bool mRealServiceCreated;
                ReputationService::ReputationServiceUtilPtr mReputationServiceUtil;
                Expression *mReputationExpression;

                friend class Blaze::ReputationService::TestHook;
        };
    }
}

#endif
