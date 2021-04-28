/////////////////////////////////////////////////////////////////////////////
// MiniToastView.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef MINITOASTVIEW_H
#define MINITOASTVIEW_H

#include <QVector>
#include <QWidget>
#include <QHBoxLayout>
#include "UIScope.h"
#include "services/plugin/PluginAPI.h"

class QNetworkReply;

namespace Ui
{
    class MiniToastView;
}

namespace Origin
{
namespace UIToolkit
{
    class OriginLabel;
}
namespace Client
{

class ORIGIN_PLUGIN_API MiniToastView : public QWidget
{
    Q_OBJECT

public:
    MiniToastView();
    ~MiniToastView();
	
    void setIcon(const QPixmap& pixmap);
    void setTitle(QString title);
	void setName(QString name);
	QString name(){return mName;};

	UIScope& getContext(){return mContext;};

signals:
    void showParent(QWidget*);

public slots:
    void setIcon(QNetworkReply* reply);

private:
    Ui::MiniToastView* ui;
	QString mName;
	UIScope mContext;
};

}
}
#endif // MINITOASTVIEW_H
