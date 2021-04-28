/////////////////////////////////////////////////////////////////////////////
// DirtyBitsTrafficDebugView.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#include <limits>
#include <QDateTime>

#include <Qt/originwindow.h>
#include "DirtyBitsTrafficDebugView.h"
#include "ui_DirtyBitsTrafficDebugView.h"
#include "services/debug/DebugService.h"

#include <QJsonDocument>
#include <QBrush>
#include <QColor>

namespace Origin 
{
namespace Client
{

static QMap<QString, QString> sUserFriendlyContextMap;

QMap<QString, QString>& userFriendlyContextMap()
{
    if(sUserFriendlyContextMap.isEmpty())
    {
        QString contextFormat = "%1 %2";

        sUserFriendlyContextMap.insert("ach", contextFormat.arg(QObject::tr("ebisu_client_notranslate_dbit_ach")).arg(QObject::tr("ebisu_client_notranslate_notification")));
        sUserFriendlyContextMap.insert("email", contextFormat.arg(QObject::tr("ebisu_client_notranslate_dbit_email")).arg(QObject::tr("ebisu_client_notranslate_notification")));
        sUserFriendlyContextMap.insert("password", contextFormat.arg(QObject::tr("ebisu_client_notranslate_dbit_password")).arg(QObject::tr("ebisu_client_notranslate_notification")));
        sUserFriendlyContextMap.insert("originid", contextFormat.arg(QObject::tr("ebisu_client_notranslate_dbit_originid")).arg(QObject::tr("ebisu_client_notranslate_notification")));
        sUserFriendlyContextMap.insert("privacy", contextFormat.arg(QObject::tr("ebisu_client_notranslate_dbit_privacy")).arg(QObject::tr("ebisu_client_notranslate_notification")));
        sUserFriendlyContextMap.insert("gamelib", contextFormat.arg(QObject::tr("ebisu_client_notranslate_dbit_gamelib")).arg(QObject::tr("ebisu_client_notranslate_notification")));
        sUserFriendlyContextMap.insert("avatar", contextFormat.arg(QObject::tr("ebisu_client_notranslate_dbit_avatar")).arg(QObject::tr("ebisu_client_notranslate_notification")));
        sUserFriendlyContextMap.insert("entitlement", contextFormat.arg(QObject::tr("ebisu_client_notranslate_dbit_entitlement")).arg(QObject::tr("ebisu_client_notranslate_notification")));
        sUserFriendlyContextMap.insert("catalog", contextFormat.arg(QObject::tr("ebisu_client_notranslate_dbit_catalog")).arg(QObject::tr("ebisu_client_notranslate_notification")));
    }

    return sUserFriendlyContextMap;
}

void DirtyBitsTrafficDebugView::showWindow()
{
    if(mWindow == NULL)
    {
        QWidget* content = new QWidget();
        ui->setupUi(content);

        ORIGIN_VERIFY_CONNECT(ui->tblTrafficView, SIGNAL(itemSelectionChanged()), this, SLOT(onSelectionChanged()));
        ORIGIN_VERIFY_CONNECT(ui->btnClearSession, SIGNAL(clicked()), this, SLOT(clear()));

        // Set up table headers.
        QStringList horizontalHeaderLabels;
        horizontalHeaderLabels.append(tr("ebisu_client_notranslate_event"));
        horizontalHeaderLabels.append(tr("ebisu_client_notranslate_time_received"));
        horizontalHeaderLabels.append(tr("ebisu_client_notranslate_status"));
        ui->tblTrafficView->setHorizontalHeaderLabels(horizontalHeaderLabels);

        QPalette palette = ui->tblTrafficView->palette();
        palette.setColor( QPalette::Active, QPalette::Highlight, Qt::yellow );
        palette.setColor( QPalette::Active, QPalette::HighlightedText, Qt::black);
        palette.setColor( QPalette::Inactive, QPalette::HighlightedText, Qt::black);
        ui->tblTrafficView->setPalette( palette );

        Origin::Engine::DirtyBitsClient::instance()->registerDebugListener(this);

        using namespace UIToolkit;
        mWindow = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close, content);
        mWindow->setObjectName("DirtyBitsTrafficTool");
        mWindow->setTitleBarText(tr("ebisu_client_notranslate_dirty_bits_viewer"));
        ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(onClose())); 
        mWindow->showWindow();
    }
    else
    {
        mWindow->raise();
        mWindow->activateWindow();
    }
}

DirtyBitsTrafficDebugView::DirtyBitsTrafficDebugView(QObject* parent /*= NULL*/)
: QObject(parent),
  mWindow(NULL),
  ui(new Ui::DirtyBitsTrafficDebugView())
{
}

DirtyBitsTrafficDebugView::~DirtyBitsTrafficDebugView()
{
    onClose();
}

void DirtyBitsTrafficDebugView::onClose()
{
    if(mWindow)
    {
        ORIGIN_VERIFY_DISCONNECT(mWindow, SIGNAL(rejected()), this, SLOT(onClose())); 
        Origin::Engine::DirtyBitsClient::instance()->unregisterDebugListener(this);
        mWindow->hide();
        mWindow->deleteLater();
        mWindow = NULL;

        if(ui)
        {
            delete ui;
            ui = NULL;
        }
    }
    emit finished();
}

void DirtyBitsTrafficDebugView::onNewDirtyBitEvent(const QString& context, long long timeStamp, const QByteArray& payload, bool isError)
{
    QTableWidgetItem *item;
    
    int row = ui->tblTrafficView->rowCount();

    ui->tblTrafficView->insertRow(row);

    if(userFriendlyContextMap().contains(context))
    {
        item = new QTableWidgetItem(userFriendlyContextMap()[context]);
    }
    else
    {
        item = new QTableWidgetItem(context);
    }
  
    if(isError)
    {
        item->setForeground(Qt::red);
    }

    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    ui->tblTrafficView->setItem(row, kColumnHeaderEvent, item);
    
    item = new QTableWidgetItem(timeStamp != 0 ? QDateTime::fromMSecsSinceEpoch(timeStamp).toString() : "");

    if(isError)
    {
        item->setForeground(Qt::red);
    }

    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    ui->tblTrafficView->setItem(row, kColumnHeaderTimeReceived, item);
    
    if(isError)
    {
        item = new QTableWidgetItem(tr("ebisu_client_notranslate_error"));
        item->setForeground(Qt::red);
    }
    else
    {
        item = new QTableWidgetItem(tr("ebisu_client_notranslate_success"));
    }

    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    if(!payload.isEmpty())
    {
        QJsonDocument jsonDocument = QJsonDocument::fromJson(payload);
        item->setData(Qt::UserRole, QString("%1\n%2").arg(tr("ebisu_client_notranslate_jsonpayload")).arg(QString::fromUtf8(jsonDocument.toJson(QJsonDocument::Indented))));
    }
    ui->tblTrafficView->setItem(row, kColumnHeaderStatus, item);
}

void DirtyBitsTrafficDebugView::onSelectionChanged()
{
    // Clear any previous data.
    ui->txtNotificationBreakdown->clear();

    // Populate file info table with data from selected file.
    QList<QTableWidgetItem*> items = ui->tblTrafficView->selectedItems();
    if(!items.isEmpty())
    {
        int row = items[0]->row();

        QTableWidgetItem* item = ui->tblTrafficView->item(row, kColumnHeaderStatus);
        if(item && !item->data(Qt::UserRole).isNull())
        {
            ui->txtNotificationBreakdown->setText(item->data(Qt::UserRole).toString());
        }
    }
}

void DirtyBitsTrafficDebugView::clear()
{
    ui->tblTrafficView->clearContents();
    ui->txtNotificationBreakdown->clear();
    ui->tblTrafficView->setRowCount(0);
}

}
}
