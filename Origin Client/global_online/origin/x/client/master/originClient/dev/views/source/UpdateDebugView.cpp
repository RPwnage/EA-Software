/////////////////////////////////////////////////////////////////////////////
// UpdateDebugView.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "UpdateDebugView.h"
#include "ui_UpdateDebugView.h"
#include "services/debug/DebugService.h"

namespace Origin 
{
namespace Client
{

UpdateDebugViewItem::UpdateDebugViewItem(const QString& text)
: QTableWidgetItem(text, Qt::UserRole)
{
}

bool UpdateDebugViewItem::operator < (const QTableWidgetItem &other) const
{
    return data(Qt::UserRole).toInt() < other.data(Qt::UserRole).toInt();
}

UpdateDebugView::UpdateDebugView(QWidget* parent /*= NULL*/)
: QWidget(parent),
  ui(new Ui::UpdateDebugView()),
  mLocalizer(QLocale("en_US"))
{
	ui->setupUi(this);

    // Set up table headers.
    QStringList horizontalHeaderLabels;
    horizontalHeaderLabels.append(tr("ebisu_client_notranslate_file_name"));
    horizontalHeaderLabels.append(tr("ebisu_client_notranslate_file_size"));
    horizontalHeaderLabels.append(tr("ebisu_client_notranslate_install_location"));
    horizontalHeaderLabels.append(tr("ebisu_client_notranslate_update_crc"));
    horizontalHeaderLabels.append(tr("ebisu_client_notranslate_local_crc"));
    ui->tblUpdateFiles->setHorizontalHeaderLabels(horizontalHeaderLabels);
}

UpdateDebugView::~UpdateDebugView()
{
    delete ui;
    ui = NULL;
}

void UpdateDebugView::addFile(qint32 index, const QString& fileName, quint64 size, const QString& installLocation, quint32 packageFileCrc, quint32 diskFileCrc)
{
    QTableWidgetItem *item;
    
    item = new QTableWidgetItem(fileName);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    ui->tblUpdateFiles->setItem(index, kColumnHeaderFileName, item);
    
    item = new UpdateDebugViewItem(mLocalizer.makeByteSizeString(size));
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    item->setData(Qt::UserRole, size);
    ui->tblUpdateFiles->setItem(index, kColumnHeaderFileSize, item);
    
    item = new QTableWidgetItem(installLocation);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    ui->tblUpdateFiles->setItem(index, kColumnHeaderInstallLocation, item);
    
    item = new QTableWidgetItem(QString("0x%1").arg(packageFileCrc, 8, 16));
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    ui->tblUpdateFiles->setItem(index, kColumnHeaderPackageCrcLocation, item);

    item = new QTableWidgetItem((diskFileCrc == 0 ? "No File" : QString("0x%1").arg(diskFileCrc, 8, 16)));
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    ui->tblUpdateFiles->setItem(index, kColumnHeaderFileCrcLocation, item);
}

void UpdateDebugView::setSource(const QString& source)
{
    ui->lblSourceValue->setText(source);
}

void UpdateDebugView::setSize(quint64 size)
{
    ui->lblSizeValue->setText(mLocalizer.makeByteSizeString(size));
}

void UpdateDebugView::setFileCount(quint32 count)
{
    ui->tblUpdateFiles->setRowCount(count);
}

void UpdateDebugView::clear()
{
    ui->tblUpdateFiles->clearContents();
}

void UpdateDebugView::setSortingEnabled(bool enable)
{
    ui->tblUpdateFiles->setSortingEnabled(enable);
}

void UpdateDebugView::showUpdateFiles(bool show)
{
    if(show)
    {
        ui->spinner->stopSpinner();
        ui->stackedWidget->setCurrentIndex(1);
    }
    else
    {
        ui->spinner->startSpinner();
        ui->stackedWidget->setCurrentIndex(0);
    }
}

}
}