///////////////////////////////////////////////////////////////////////////////
// CustomizeBoxartFlow.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef CUSTOMIZEBOXARTFLOW_H
#define CUSTOMIZEBOXARTFLOW_H

#include "engine/content/Entitlement.h"
#include "flows/AbstractFlow.h"
#include "services/entitlements/BoxartData.h"
#include "services/plugin/PluginAPI.h"
#include "widgets/myGames/source/CustomizeBoxartViewController.h"

namespace Origin
{
namespace Engine
{
    namespace Content
    {
        class CustomBoxartController;
    }
}

namespace Client
{

    class ORIGIN_PLUGIN_API CustomizeBoxartFlow : public AbstractFlow
    {
        Q_OBJECT

    public:

        CustomizeBoxartFlow();
        ~CustomizeBoxartFlow();

        void initialize(Engine::Content::EntitlementRef game = Engine::Content::EntitlementRef(NULL));

        virtual void start();
        void focus();

    public slots:

        void onBoxartChanged(const Services::Entitlements::Parser::BoxartData& gameData);
        void onRemoveBoxart();
        void onCacheUpdated();

        void stop();

    signals:

        void finished(CustomizeBoxartFlowResult);

    private:

        void cleanup();

        CustomizeBoxartViewController* mViewController;
        Engine::Content::EntitlementRef mGame;
        Engine::Content::CustomBoxartController* mCustomBoxartController;
    };

} // Client
} // Origin

#endif // NONORIGINGAMEFLOW_H
