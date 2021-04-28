/////////////////////////////////////////////////////////////////////////////
// CodeRedemptionDebugViewController.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef CODEREDEMPTIONDEBUGVIEWCONTROLLER_H
#define CODEREDEMPTIONDEBUGVIEWCONTROLLER_H

#include <QObject>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace UIToolkit
{
class OriginWindow;
}
namespace Client
{
using namespace UIToolkit;
class ORIGIN_PLUGIN_API CodeRedemptionDebugViewController : public QObject
{
    Q_OBJECT

public:

    CodeRedemptionDebugViewController();
    ~CodeRedemptionDebugViewController();
            
    /// \brief Returns the current CodeRedemptionDebugViewController instance.
    /// \return The current CodeRedemptionDebugViewController instance.
    static CodeRedemptionDebugViewController* instance();

    /// \brief Show code redemption window
    void show();

private:
    /// \brief Creates the current CodeRedemptionDebugViewController instance.
    static void init() { sInstance = new CodeRedemptionDebugViewController(); }
    static CodeRedemptionDebugViewController* sInstance;
    UIToolkit::OriginWindow* mWindow;

private slots:
    void onCloseWindow();
};

}
}
#endif