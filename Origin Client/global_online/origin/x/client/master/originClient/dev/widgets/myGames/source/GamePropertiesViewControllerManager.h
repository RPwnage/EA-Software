/////////////////////////////////////////////////////////////////////////////
// GamePropertiesViewControllerManager.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef GAMEPROPERTIESVIEWCONTROLLERMANAGER_H
#define GAMEPROPERTIESVIEWCONTROLLERMANAGER_H

#include <QWidget>
#include <QSharedPointer>
#include "engine/content/ContentTypes.h"
#include "services/plugin/PluginAPI.h"

namespace Ui 
{
    class GamePropertiesView;
}

namespace Origin
{
namespace UIToolkit
{
class OriginWindow;
}
namespace Client
{
class ORIGIN_PLUGIN_API GamePropertiesViewController : public QWidget
{
    Q_OBJECT

public:
    GamePropertiesViewController(Engine::Content::EntitlementRef game, QWidget* parent = 0);
    ~GamePropertiesViewController();

public slots:
    void onApplyChanges();


private:
    QString mInitMultiLaunchValue;
    QString mInitCommandLineArgValue;
    QString mInitLang;
    bool mInitOIGDisabledValue;

    Engine::Content::EntitlementRef mGame;
    Ui::GamePropertiesView* ui;
};

}
} 
#endif 