///////////////////////////////////////////////////////////////////////////////
// OriginWebPopup.h
//
// Copyright (c) 2014 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef ORIGINWEBPOPUP_H
#define ORIGINWEBPOPUP_H

#include "services/plugin/PluginAPI.h"

#include <QObject>

namespace Origin
{
namespace UIToolkit
{
    class OriginWindow;
}

namespace Client
{

class ORIGIN_PLUGIN_API OriginWebPopup : public QObject
{
    Q_OBJECT

public:
    OriginWebPopup(QObject *parent = NULL);

    UIToolkit::OriginWindow* window() { return mWindow; }

public slots:
    void show();
    void hide();

private slots:
    void onPrintRequested();

private:
    enum
    {
        DEFAULT_WIDTH = 600,
        DEFAULT_HEIGHT = 500,
    };
    UIToolkit::OriginWindow* mWindow;
};

}
}

#endif
