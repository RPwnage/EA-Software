///////////////////////////////////////////////////////////////////////////////
// ShowEligibleFilesPopup.cpp
// 
// Copyright (c) 2010 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include "ShowEligibleFilesPopup.h"
#include "Qt/originwindow.h"
#include "Qt/originwindowmanager.h"
#include "ui_ShowEligibleFilesPopup.h"


#include <QCloseEvent>
#include <QDesktopWidget>
#include <QtWebKitWidgets/QWebFrame>
#include <QListWidget>
#include <QListWidgetItem>
#include <QList>
#include "services/debug/DebugService.h"

namespace Origin
{
namespace Client
{
ShowEligibleFilesPopup::ShowEligibleFilesPopup(const QString& titleStr)
	: QObject()
    , mWindow(NULL)
    , mTitleStr(titleStr)
{
    mListWidget = new QWidget();
    ui = new Ui::ShowEligibleFilesPopup();
    ui->setupUi(mListWidget);
    this->init();
}


ShowEligibleFilesPopup::~ShowEligibleFilesPopup()
{
    if (mWindow)
        mWindow->deleteLater();
}


void ShowEligibleFilesPopup::init()
{
    using namespace Origin::UIToolkit;
    mWindow = new OriginWindow((OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Close),
        mListWidget, OriginWindow::Custom, QDialogButtonBox::Ok);

    mWindow->setFixedSize(640, 320);

    mWindow->setTitleBarText(mTitleStr);
    mWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), mWindow, SLOT(close()));
    ORIGIN_VERIFY_CONNECT(ui->btnOk, SIGNAL(clicked()), mWindow, SLOT(close()));
    ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(closeEventTriggered()), this, SLOT(closeEvent()));

}

void ShowEligibleFilesPopup::showDialog()
{
    if (mWindow)
        mWindow->show();
}
void ShowEligibleFilesPopup::execDialog()
{
    if (mWindow)
        mWindow->exec();
}

void ShowEligibleFilesPopup::hideDialog()
{
    if (mWindow)
        mWindow->hide();
}

void ShowEligibleFilesPopup::closeSettings()
{
	mWindow->close();
}

void ShowEligibleFilesPopup::closeEvent()
{
	emit(EnableOnClose());	
}

void ShowEligibleFilesPopup::addFileList(const QStringList & filesList)
{
    for (int i = 0; i < filesList.length(); ++i)
    {
        ui->listWidget->addItem(filesList.at(i));
    }
}

QStringList ShowEligibleFilesPopup::getFiles()
{
    QStringList strList;
    if (mWindow)
    {
        QList<QListWidgetItem *> litems = ui->listWidget->selectedItems();
        for (int i = 0; i < litems.length(); ++i)
        {
            strList.append(litems.at(i)->text());
        }
    }
    return strList;
}
}
}
