//    Copyright © 2011-2012, Electronic Arts
//    All rights reserved.
//    Author: 

#ifndef NONORIGINGAMEVIEWCONTROLLER_H
#define NONORIGINGAMEVIEWCONTROLLER_H

#include "NonOriginGameView.h"

#include "engine/content/NonOriginContentController.h"
#include "engine/content/Entitlement.h"

#include "engine/content/NonOriginGameData.h"
#include "services/plugin/PluginAPI.h"

#include <QObject>

namespace Origin
{
namespace Client
{
    class NonOriginGameFlow;

    class ORIGIN_PLUGIN_API NonOriginGameViewController : public QObject
    {
        Q_OBJECT

    public:

        NonOriginGameViewController(NonOriginGameFlow* parent);
        ~NonOriginGameViewController();
            
        void initialize();
        void focus();

    signals:

        void propertiesChanged(const Origin::Engine::Content::NonOriginGameData&, bool forceUpdate);
        void removeBrokenGame();
        void redeemGameCode();
        void showSubscriptionPage();

    public slots:

        void onShowAddGameDialog();
        void onShowSelectGamesDialog(const QList<Engine::Content::ScannedContent>& games);
        void onShowPropertiesDialog(Engine::Content::EntitlementRef game);
        void onShowGameRepairDialog(Engine::Content::EntitlementRef game);
        void onShowConfirmRemoveGame(Engine::Content::EntitlementRef game);

        void onGameAlreadyExists(const QString& gameName);
        void onInvalidFileType(const QString& fileName);

    private slots:

        void onBrowseForGames();
        void onScanForGames();
        void onRemoveGameConfirmed();

    private:
        NonOriginGameFlow* mParent;
        NonOriginGameView* mNonOriginGameView;
        bool mShowingRepairDialog;

    };

}
} 

#endif //NONORIGINGAMEVIEWCONTROLLER_H
