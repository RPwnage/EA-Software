/////////////////////////////////////////////////////////////////////////////
// HotkeyToastView.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef HOTKEYTOASTVIEW_H
#define HOTKEYTOASTVIEW_H

#include <QWidget>
#include <QHBoxLayout>
#include "UIScope.h"
#include "services/plugin/PluginAPI.h"

class QNetworkReply;

namespace Ui
{
    class HotkeyToastView;
}

namespace Origin
{
    namespace UIToolkit
    {
        class OriginLabel;
    }
    namespace Client
    {

        class ORIGIN_PLUGIN_API HotkeyToastView : public QWidget
        {
            Q_OBJECT

            public:
    
                enum HotkeyContext
                {
                    Open,
                    View,
                    Chat,
                    Reply,
                    Unpin,
                    Repin
                };

                HotkeyToastView(HotkeyContext context, QWidget* parent = 0);
                ~HotkeyToastView();

                void handleHotkeyLables();
	            void setOIGHotkeyStyle(bool isHotkey);

            signals:
                void showParent(QWidget*);

            private:
                HotkeyContext mContext;
                Ui::HotkeyToastView* ui;
	            void setupHotkeyLabels();
	
        };

    }
}
#endif
