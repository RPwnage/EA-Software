/////////////////////////////////////////////////////////////////////////////
// ActionableToastView.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef ACTIONABLETOASTVIEW_H
#define ACTIONABLETOASTVIEW_H

#include <QWidget>
#include "UIScope.h"
#include "services/plugin/PluginAPI.h"

class QNetworkReply;

namespace Ui
{
    class ActionableToastView;
}

namespace Origin
{
    namespace UIToolkit
    {
        class OriginLabel;
    }
    namespace Client
    {

        class ORIGIN_PLUGIN_API ActionableToastView : public QWidget
        {
            Q_OBJECT

            public:
                ActionableToastView(QWidget* parent = 0);
                ~ActionableToastView();

            signals:
                void acceptClicked();
                void declineClicked();

            private:
                Ui::ActionableToastView* ui;

        };

    }
}
#endif
