//
//  EntitlementService.cpp
//  
//
//  Created by Kiss, Bela on 12-06-08.
//  Copyright (c) 2012 Electronic Arts Inc. All rights reserved.
//

#include "EntitlementService.h"
#include "LoginService.h"
#include "Console.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/entitlements/EntitlementServiceClient.h"
#include "services/entitlements/EntitlementServiceResponse.h"
#include "services/entitlements/EntitlementData.h"

#include <iostream>

using namespace Origin::Services;

EntitlementService* EntitlementService::sInstance = NULL;

void EntitlementService::init()
{
    if (sInstance == NULL)
    {
        sInstance = new EntitlementService();
        ORIGIN_ASSERT(sInstance);
    }
}

EntitlementService* EntitlementService::instance()
{
    return sInstance;
}

void EntitlementService::fetchEntitlements(QStringList const& args)
{
    Entitlements::EntitlementServiceResponse* entitlementsResponse
        = Entitlements::EntitlementServiceClient::consolidated(LoginService::currentSession());
    ORIGIN_ASSERT(entitlementsResponse);
    ORIGIN_VERIFY_CONNECT(
        entitlementsResponse, SIGNAL(finished()),
        instance(), SLOT(onFetchEntitlementsFinished()));
}

void EntitlementService::onFetchEntitlementsFinished()
{
    CONSOLE_WRITE << "received entitlements response";
    
    Origin::Services::Entitlements::EntitlementServiceResponse* response = dynamic_cast<Origin::Services::Entitlements::EntitlementServiceResponse*>(instance()->sender());
    ORIGIN_ASSERT(response);
    
    restError error = response->error();
    if (error == Origin::Services::restErrorSuccess)
    {
        typedef QVector<EntitlementServerData*> Entitlements;
        Entitlements fetchedEntitlements = response->entitlements();
        
        CONSOLE_WRITE << "#entitlements = " << fetchedEntitlements.count();
        foreach (EntitlementServerData* i, fetchedEntitlements)
        {
            CONSOLE_WRITE << "id " << i->contentId() << ", name = " << i->displayName();
        }
    }
    else
    {
        CONSOLE_WRITE << "entitlement fetch failed, err = " << error;
    }
}

EntitlementService::EntitlementService()
{
}

