/////////////////////////////////////////////////////////////////////////////
// ClientSystemMenu.h
//
// Copyright (c) 2015, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef CLIENTSYSTEMMENU_H
#define CLIENTSYSTEMMENU_H

#include "CustomQMenu.h"
#include "engine/content/ContentTypes.h"

namespace Origin
{
namespace Client
{
class ORIGIN_PLUGIN_API OriginSysTrayMenuItem : public QObject
{
	Q_OBJECT
public:
	OriginSysTrayMenuItem(QString title, QString product_id);
	~OriginSysTrayMenuItem();

	QAction* mMenuItem;
	QString mProductID;

public slots:
	void onPlay();

protected:
	QString mTitle;
};

class ORIGIN_PLUGIN_API ClientSystemMenu : public CustomQMenu
{
    Q_OBJECT
public:
    ClientSystemMenu(QWidget* parent = NULL);
    ~ClientSystemMenu();
    void addRecentGameToMenu(QString title, QString product_id);
    void updateRecentPlayedGames();
    void removeAllRecent();

signals:
    void redeemTriggered();
    void logoutTriggered();
    void quitTriggered();
    void openTriggered();


private slots:
    void onActionTriggered();
    void onPlayingGameTimeUpdated(Origin::Engine::Content::EntitlementRef);

private:
    enum Action {
        Action_Open,
        Action_Quit,
        Action_Redeem,
        Action_Logout
    };
    QAction* addSystemTrayOption(const QString& text, const Action& actionType, const bool& enabled = true);
    QList<OriginSysTrayMenuItem*> mRecentGames;
    // pointer to the top-most item so we can add from the top
    QAction* mMostRecent;
    QAction* mOpenSeparator;
};
}
}
#endif