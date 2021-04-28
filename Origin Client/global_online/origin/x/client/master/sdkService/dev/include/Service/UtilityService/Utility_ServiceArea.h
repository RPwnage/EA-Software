///////////////////////////////////////////////////////////////////////////////
//
//  Copyright c 2009 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File:	Utility_ServiceArea.h
//	Utility manager service area definition
//
//	Author: Sergey Zavadski

#ifndef _UTILITY_SERVICE_AREA_H_
#define _UTILITY_SERVICE_AREA_H_

#include "Service/BaseService.h"
#include "services/rest/AuthPortalServiceResponses.h"

namespace Origin
{
    namespace SDK
    {
        class Utility_ServiceArea : public BaseService
        {
        public:
            virtual ~Utility_ServiceArea( );

            // Singleton functions
            static Utility_ServiceArea* instance();
            static Utility_ServiceArea* create(Lsx::LSX_Handler* handler);
            void destroy();

        private:
            // block constructors
            Utility_ServiceArea( Lsx::LSX_Handler* handler );
            Utility_ServiceArea();
            //
            //	LSX Handlers
            //
            void getInternetConnectedState( Lsx::Request& request, Lsx::Response& response );
            void getAccessToken( Lsx::Request& request, Lsx::Response& response );
            void getAuthCode( Lsx::Request& request, Lsx::Response& response );

            bool AuthCodeResponse(Origin::Services::AuthPortalAuthorizationCodeResponse * resp, Lsx::Response& response);
        };
    }
}

#endif //_UTILITY_SERVICE_AREA_H_
