/////////////////////////////////////////////////////////////////////////////
// DownloadDebugView.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "DownloadDebugView.h"
#include "ui_DownloadDebugView.h"
#include "services/debug/DebugService.h"

namespace Origin 
{
namespace Client
{

DownloadDebugViewItem::DownloadDebugViewItem(const QString& text)
: QTableWidgetItem(text, Qt::UserRole)
{
}

bool DownloadDebugViewItem::operator < (const QTableWidgetItem &other) const
{
    return data(Qt::UserRole).toInt() < other.data(Qt::UserRole).toInt();
}

DownloadDebugView::DownloadDebugView(QWidget* parent /*= NULL*/)
: QWidget(parent),
  ui(new Ui::DownloadDebugView()),
  mLocalizer(QLocale("en_US"))
{
	ui->setupUi(this);

    ORIGIN_VERIFY_CONNECT(ui->tblProgressInfo, SIGNAL(itemSelectionChanged()), this, SLOT(onSelectionChanged()));

    // Set up table headers.
    QStringList horizontalHeaderLabels;
    horizontalHeaderLabels.append(tr("ebisu_client_notranslate_file_name"));
    horizontalHeaderLabels.append(tr("ebisu_client_notranslate_file_size"));
    horizontalHeaderLabels.append(tr("ebisu_client_notranslate_install_location"));
    horizontalHeaderLabels.append(tr("ebisu_client_notranslate_status"));
    ui->tblProgressInfo->setHorizontalHeaderLabels(horizontalHeaderLabels);
    
    QStringList verticalHeaderLabels;
    verticalHeaderLabels.append(tr("ebisu_client_notranslate_file"));
    verticalHeaderLabels.append(tr("ebisu_client_notranslate_error"));
    verticalHeaderLabels.append(tr("ebisu_client_notranslate_info"));
    ui->tblFileInfo->setVerticalHeaderLabels(verticalHeaderLabels);
    
    ui->tblFileInfo->verticalHeader()->setSectionsClickable(false);
    ui->tblFileInfo->verticalHeader()->setDefaultAlignment(Qt::AlignTop | Qt::AlignLeft);
}

DownloadDebugView::~DownloadDebugView()
{
    delete ui;
    ui = NULL;
}

void DownloadDebugView::addFile(qint32 index, const QString& fileName, qint64 size, const QString& installLocation, Origin::Engine::Debug::FileStatus status)
{
    QTableWidgetItem *item;
    
    item = new QTableWidgetItem(fileName);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    ui->tblProgressInfo->setItem(index, kColumnHeaderFileName, item);
    
    item = new DownloadDebugViewItem("");
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    item->setData(Qt::UserRole, size);
    ui->tblProgressInfo->setItem(index, kColumnHeaderFileSize, item);
    ui->tblProgressInfo->setCellWidget(index, kColumnHeaderFileSize, new QLabel(mLocalizer.makeByteSizeString(size)));
    
    item = new QTableWidgetItem(installLocation);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    ui->tblProgressInfo->setItem(index, kColumnHeaderInstallLocation, item);
    
    item = new DownloadDebugViewItem(Engine::Debug::fileStatusAsString(status));
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    item->setData(Qt::UserRole, status);
    ui->tblProgressInfo->setItem(index, kColumnHeaderStatus, item);
}

void DownloadDebugView::updateStatus(const QString& installLocation, Origin::Engine::Debug::FileStatus status)
{
    QList<QTableWidgetItem*> items = ui->tblProgressInfo->findItems(installLocation, Qt::MatchFixedString);
    for(QList<QTableWidgetItem*>::iterator it = items.begin(); it != items.end(); ++it)
    {
        int row = ui->tblProgressInfo->row(*it);

        // Update status column.
        QTableWidgetItem *item = ui->tblProgressInfo->item(row, kColumnHeaderStatus);
        if(item)
        {
            item->setText(Engine::Debug::fileStatusAsString(status));
            break;
        }
    }
}

void DownloadDebugView::setSource(const QString& source)
{
    ui->lblSourceValue->setText(source);
}

void DownloadDebugView::setDestination(const QString& destination)
{
    ui->lblDestinationValue->setText(destination);
}

void DownloadDebugView::setFileCount(qint32 count)
{
    ui->tblProgressInfo->setRowCount(count);
}

void DownloadDebugView::setFileInfo(const QString& fileName, quint64 errorCode, const QString& errorDescription)
{
    QTableWidgetItem *item;
    
    item = new QTableWidgetItem(fileName);
    item->setFlags(Qt::NoItemFlags);
    item->setTextAlignment(Qt::AlignTop | Qt::AlignLeft);
    ui->tblFileInfo->setItem(kRowHeaderFile, 0, item);
    
    item = new QTableWidgetItem(QString::number(errorCode));
    item->setFlags(Qt::NoItemFlags);
    item->setTextAlignment(Qt::AlignTop | Qt::AlignLeft);
    ui->tblFileInfo->setItem(kRowHeaderError, 0, item);
    
    item = new QTableWidgetItem(errorDescription);
    item->setFlags(Qt::NoItemFlags);
    item->setTextAlignment(Qt::AlignTop | Qt::AlignLeft);
    ui->tblFileInfo->setItem(kRowHeaderInfo, 0, item);
        
    // Resize to fit contents if necessary.
    ui->tblFileInfo->resizeColumnsToContents();
    ui->tblFileInfo->resizeRowsToContents();
}

void DownloadDebugView::onSelectionChanged()
{
    // Clear any previous data.
    ui->tblFileInfo->clearContents();

    // Clear styling of previously selected row.  We must do this because we're using a QLabel to display file size markup.
    for(int row = 0; row < ui->tblProgressInfo->rowCount(); ++row)
    {
        QLabel* label = dynamic_cast<QLabel*>(ui->tblProgressInfo->cellWidget(row, kColumnHeaderFileSize));
        if(label && label->property("selected").toString() == "true")
        {
            label->setProperty("selected", QVariant::Invalid);
            label->setStyle(QApplication::style());
        }
    }

    // Populate file info table with data from selected file.
    QList<QTableWidgetItem*> items = ui->tblProgressInfo->selectedItems();
    if(!items.isEmpty())
    {
        int row = items[0]->row();

        QTableWidgetItem* item = ui->tblProgressInfo->item(row, kColumnHeaderInstallLocation);
        if(item)
        {
            emit selectionChanged(item->text());
        }

        QLabel* label = dynamic_cast<QLabel*>(ui->tblProgressInfo->cellWidget(row, kColumnHeaderFileSize));
        if(label)
        {
            label->setProperty("selected", true);
            label->setStyle(QApplication::style());
        }
    }
}

void DownloadDebugView::clear()
{
    ui->tblProgressInfo->clearContents();
}

void DownloadDebugView::setSortingEnabled(bool enable)
{
    ui->tblProgressInfo->setSortingEnabled(enable);
}

}
}