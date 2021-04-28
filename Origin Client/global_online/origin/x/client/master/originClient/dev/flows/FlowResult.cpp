///////////////////////////////////////////////////////////////////////////////
// FlowResult.cpp
//
// Copyright (c) 2014 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "flows/FlowResult.h"
#include "services/debug/DebugService.h"

namespace Origin
{
	namespace Client
	{
		QString resultString(const Result result)
        {
#define RETURN_RESULT_STRING(resultValue) if (result == resultValue) { return #resultValue; }

            RETURN_RESULT_STRING(FLOW_SUCCEEDED);
            RETURN_RESULT_STRING(FLOW_FAILED);

#undef RETURN_RESULT_STRING

            // New enum value added but not being handled here. Please
            // add a condition to convert this value to string.
            ORIGIN_ASSERT( 0 );

            return "UNKNOWN";
        }

        QString ssoFlowActionString(const SSOFlowResult::Action action)
        {
#define RETURN_SSO_STRING(actionValue) if (action == SSOFlowResult::actionValue) { return #actionValue; }

            RETURN_SSO_STRING(SSO_NONE);
            RETURN_SSO_STRING(SSO_LOGIN);
            RETURN_SSO_STRING(SSO_LOGOUT);
            RETURN_SSO_STRING(SSO_LOGGEDIN_USERMATCHES);
            RETURN_SSO_STRING(SSO_LOGGEDIN_USERUNKNOWN);

#undef RETURN_SSO_STRING

            // New enum value added but not being handled here. Please
            // add a condition to convert this value to string.
            ORIGIN_ASSERT( 0 );

            return "UNKNOWN";
        }
	}
}