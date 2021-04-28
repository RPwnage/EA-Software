///////////////////////////////////////////////////////////////////////////////
// NonOriginGameFlow.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef NONORIGINGAMEFLOW_H
#define NONORIGINGAMEFLOW_H

#include "engine/content/Entitlement.h"
#include "flows/AbstractFlow.h"
#include "engine/content/NonOriginGameData.h"
#include "services/plugin/PluginAPI.h"
#include "widgets/myGames/source/NonOriginGameViewController.h"

namespace Origin
{
namespace Engine
{
    namespace Content
    {
        class NonOriginContentController;
    }
}

namespace Client
{

    class ORIGIN_PLUGIN_API NonOriginGameFlow : public AbstractFlow
    {
        Q_OBJECT

    public:

        enum FlowOperation
        {
            OpAdd = 0,
            OpRemove,
            OpModify,
            OpRepair,
            OpNotSet
        };

        NonOriginGameFlow();
        ~NonOriginGameFlow();

        void initialize(FlowOperation op, Engine::Content::EntitlementRef game = Engine::Content::EntitlementRef(NULL));
        FlowOperation currentOperation() const;

        virtual void start();
        void focus();

    public slots:

        void onScanForGamesSelected();
        void onGameSelectionComplete(const QStringList& executablePaths);
        void onScanComplete();
        void onRemoveConfirmed();
        void onPropertiesChanged(const Origin::Engine::Content::NonOriginGameData& gameData, bool forceUpdate);
        void onCacheUpdated();
        void onNonOriginGameModified(Origin::Engine::Content::EntitlementRef originalNog, Origin::Engine::Content::EntitlementRef modifiedNog);
        void onShowRedemptionPage();
        void onSubscriptionPage();

        void cancel();
        void stop(bool showMyGames = true);

    signals:

        void finished(NonOriginGameFlowResult);

    private slots:

        void onRemoveBrokenGame();

    private:

        void cleanup();
        bool isValidExecutablePath(const QString& path);

        NonOriginGameViewController* mViewController;
        Engine::Content::EntitlementRef mGame;
        Engine::Content::NonOriginContentController* mNonOriginController;
        FlowOperation mOperation;
    };

} // Client
} // Origin

#endif // NONORIGINGAMEFLOW_H
