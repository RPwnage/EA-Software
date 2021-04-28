/////////////////////////////////////////////////////////////////////////////
// MiniToastView.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "MiniToastView.h"
#include "ui_MiniToastView.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/settings/Setting.h"
#include "originlabel.h"
#include <QNetworkReply>

namespace Origin
{
namespace Client
{

MiniToastView::MiniToastView()
: QWidget(NULL)
, ui(new Ui::MiniToastView)
{
	mContext = IGOScope;
	ui->setupUi(this);
}


MiniToastView::~MiniToastView()
{
    if(ui)
    {
        delete ui;
        ui = NULL;
    }
}


void MiniToastView::setIcon(const QPixmap& pixmap)
{
    if (pixmap.isNull())
    {
        ui->horizontalLayout->removeWidget(ui->icon);
    }
    else
    {
        QPixmap scaledPixmap = pixmap.scaled(20, 20, Qt::KeepAspectRatio);
        ui->icon->setPixmap(scaledPixmap);
    }
    adjustSize();
}


void MiniToastView::setIcon(QNetworkReply* reply)
{
    QPixmap p;
    if(reply->error())
    {
        p = QPixmap(":/notifications/defaultorigin.png");
    }
    else 
    {
        QByteArray bArray = reply->readAll();
        p.loadFromData(bArray, "PNG");
    }

    if(p.width() != ui->icon->width())
    {
        p = p.scaled(QSize(28, 28), Qt::KeepAspectRatio);
    }
    adjustSize();

    emit showParent(topLevelWidget());
}

void MiniToastView::setTitle(QString title)
{
	if(!title.isNull())
    {
        ui->lblUser->setLabelType( UIToolkit::OriginLabel::DialogDescription);
		ui->lblUser->setVisible(true);
		ui->lblUser->setText(title);
    }
    adjustSize();
}

void MiniToastView::setName(QString name)
{
	mName = name;
}


}
}
