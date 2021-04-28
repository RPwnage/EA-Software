/////////////////////////////////////////////////////////////////////////////
// NetPromoterWidget.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef NetPromoterWidget_H
#define NetPromoterWidget_H

#include <QWidget>

#include "services/plugin/PluginAPI.h"

namespace Ui
{
    class NetPromoterWidget;
}

namespace Origin
{

namespace Client 
{

enum NetPromoterWidgetType 
{
    ONE_QUESTION = 0,
    TWO_QUESTION
};

class ORIGIN_PLUGIN_API NetPromoterWidget : public QWidget
{
    Q_OBJECT

    public:
        NetPromoterWidget(NetPromoterWidgetType questionType, QWidget* parent = NULL);

        unsigned short surveyNumber() const;
        QString surveyText() const;
        bool canContactUser() const;

    signals:
        void surveySelected(bool);

    protected slots:
        void surveyChanged(int);

    private:
        void changeEvent(QEvent *event);
        void translateUi();

        Ui::NetPromoterWidget* ui;
        NetPromoterWidgetType mType;
};

} // client

} // Origin

#endif
