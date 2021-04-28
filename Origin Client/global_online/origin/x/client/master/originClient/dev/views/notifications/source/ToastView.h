/////////////////////////////////////////////////////////////////////////////
// ToastView.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef TOASTVIEW_H
#define TOASTVIEW_H

#include <QVector>
#include <QWidget>
#include "UIScope.h"

#include "services/plugin/PluginAPI.h"

class QNetworkReply;

namespace Ui
{
    class ToastView;
}

namespace Origin
{
namespace UIToolkit
{
    class OriginNotificationDialog;
}
namespace Client
{

class ORIGIN_PLUGIN_API ToastView : public QWidget
{
    Q_OBJECT

public:
    ToastView(const UIScope& context);
    ~ToastView();
	
    void setIcon(const QPixmap& pixmap);
  	void setupCustomWidgets(QVector<QWidget*> widgets, UIToolkit::OriginNotificationDialog* dialog);
    void setText(const QString& title, const QString& message);
    void setTitle(const QString& title);
	void setMessage(const QString& content);
	void setName(const QString& name);
	QString name() const {return mName;}

    void handleHotkeyLables();
    // Returns height after widget removal
    int removeCustomWidget(QWidget* widget);
	UIScope context() const {return mContext;}

signals:
    void showParent(QWidget*);
    void closeClicked();
    void clicked();

public slots:
    void setIcon(QNetworkReply* reply);

protected:
    void mousePressEvent(QMouseEvent*);
	void mouseReleaseEvent(QMouseEvent*);

private:
    Ui::ToastView* ui;
	QString mName;
	const UIScope mContext;	
};

}
}
#endif
