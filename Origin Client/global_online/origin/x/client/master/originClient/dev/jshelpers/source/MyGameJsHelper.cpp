///////////////////////////////////////////////////////////////////////////////
// MyGameJsHelper.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "MyGameJsHelper.h"
#include "engine/content/ContentController.h"
#include "flows/ClientFlow.h"
#include "flows/MainFlow.h"
#include "ContentFlowController.h"
#include "LocalContentViewController.h"

namespace Origin
{
    namespace Client
    {
        MyGameJsHelper::MyGameJsHelper(QObject *parent) : QObject(parent)
        {
        }


        MyGameJsHelper::~MyGameJsHelper()
        {
        }

        void MyGameJsHelper::refreshEntitlements()
        {
            // TODO: Is this a full or incremental request via this helper?
            Origin::Engine::Content::ContentController::currentUserContentController()->refreshUserGameLibrary();
        }

        void MyGameJsHelper::startDownload(const QString& id)
        {
            Origin::Engine::Content::ContentController *cc = Origin::Engine::Content::ContentController::currentUserContentController();
            if (cc && !cc->downloadById(id))
            {
                Engine::Content::EntitlementRef entRef = cc->entitlementById(id);
                if (entRef.isNull())
                {
                    // If this entitlement was not found, the client may not be aware of it yet.  Download it on the next refresh.
                    cc->autoStartDownloadOnNextRefresh(id);
                }
                else if (!entRef->parent().isNull() && !entRef->parent()->localContent()->installed(true))
                {
                    // If the parent entitlement is not installed, prompt the user to install it.
                    MainFlow::instance()->contentFlowController()->localContentViewController()->showParentNotInstalledPrompt(entRef);
                }
            }
        }

        void MyGameJsHelper::showGameDetails(const QString& id)
        {
            Origin::Engine::Content::ContentController *cc = Origin::Engine::Content::ContentController::currentUserContentController();
            if(cc)
            {
                Origin::Engine::Content::EntitlementRef entRef = cc->entitlementById(id);

                if(entRef.isNull())
                {
                    Origin::Client::ClientFlow::instance()->showMyGames();
                }
                else if(!entRef->parent().isNull() && !entRef->contentConfiguration()->isBaseGame())
                {
                    // Client has a parent and is not a top level game, so show the parent's details page.
                    Origin::Client::ClientFlow::instance()->showGameDetails(entRef->parent());
                }
                else
                {
                    Origin::Client::ClientFlow::instance()->showGameDetails(entRef);
                }
            }
        }

        void MyGameJsHelper::showGames()
        {
            Origin::Client::ClientFlow::instance()->showMyGames();
        }

        void MyGameJsHelper::loadStoreHome()
        {
            Origin::Client::ClientFlow::instance()->showStoreHome();
        }

        QString MyGameJsHelper::getState(const QString& id)
        {
            Origin::Engine::Content::EntitlementRef entRef;
            entRef = Origin::Engine::Content::ContentController::currentUserContentController()->entitlementById(id);

            if(entRef->localContent()->installed(true))
            {
                return "INSTALLED";
            }
            else if(entRef->localContent()->downloadable())
            {
                return (entRef->localContent()->releaseControlState() == Origin::Engine::Content::LocalContent::RC_Preload) ? "PRELOADABLE" : "DOWNLOADABLE";
            }
            else if(entRef->localContent()->installable())
            {
                return "INSTALLABLE";
            }
            return "UNRELEASED";

        }
    }
}