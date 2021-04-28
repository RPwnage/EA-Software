/////////////////////////////////////////////////////////////////////////////
// TelemetryDebugView.cpp
//
// Copyright (c) 2014, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#include <Qt/originwindow.h>
#include "TelemetryDebugView.h"
#include "ui_TelemetryDebugView.h"
#include "services/debug/DebugService.h"
#include "services/platform/PlatformService.h"
#include "TelemetryAPIDLL.h"

#include <QDateTime>
#include <QBrush>
#include <QColor>
#include <QDir>

namespace Origin 
{
namespace Plugins
{
namespace DeveloperTool
{

TelemetryDebugView::TelemetryDebugView(QObject* parent /*= NULL*/)
: QObject(parent),
  mWindow(NULL),
  ui(NULL),
  mCurrentFilterBehavior(kFilterBehaviorMatchAll)
{
}

TelemetryDebugView::~TelemetryDebugView()
{
    onClose();
}

void TelemetryDebugView::showWindow()
{
    if(mWindow == NULL)
    {
        QWidget* content = new QWidget();
        ui = new Ui::TelemetryDebugView();
        ui->setupUi(content);

        ORIGIN_VERIFY_CONNECT(ui->btnApplyFilters, SIGNAL(clicked()), this, SLOT(onApplyFilters()));
        ORIGIN_VERIFY_CONNECT(ui->btnClearFilters, SIGNAL(clicked()), this, SLOT(onClearFilters()));
        ORIGIN_VERIFY_CONNECT(ui->tblEventViewer, SIGNAL(itemSelectionChanged()), this, SLOT(onSelectionChanged()));
        ORIGIN_VERIFY_CONNECT(ui->btnClearSession, SIGNAL(clicked()), this, SLOT(clear()));
        ORIGIN_VERIFY_CONNECT(ui->btnViewXML, SIGNAL(clicked()), this, SLOT(onViewXML()));

        ui->ddMatch->addItem("Match All Filters", kFilterBehaviorMatchAll);
        ui->ddMatch->addItem("Match Any Filter", kFilterBehaviorMatchAny);
        ui->ddMatch->setCurrentIndex(kFilterBehaviorMatchAll);

        // Set up table headers.
        QStringList horizontalHeaderLabels;
        horizontalHeaderLabels.append("Timestamp");
        horizontalHeaderLabels.append("Module");
        horizontalHeaderLabels.append("Group");
        horizontalHeaderLabels.append("String");
        ui->tblEventViewer->setHorizontalHeaderLabels(horizontalHeaderLabels);
        ui->tblEventViewer->setColumnWidth(0, 168);

        QPalette palette = ui->tblEventViewer->palette();
        palette.setColor( QPalette::Active, QPalette::Highlight, Qt::yellow );
        palette.setColor( QPalette::Active, QPalette::HighlightedText, Qt::black);
        palette.setColor( QPalette::Inactive, QPalette::HighlightedText, Qt::black);
        ui->tblEventViewer->setPalette( palette );

        GetTelemetryInterface()->AddEventListener(this);
        GetTelemetryInterface()->Metric_ODT_TELEMETRY_VIEWER_OPENED();

        using namespace UIToolkit;
        mWindow = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close, content);
        mWindow->setObjectName("TelemetryLiveViewer");
        mWindow->setTitleBarText(tr("Telemetry Live Viewer"));
        ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(onClose())); 
        mWindow->showWindow();
    }
    else
    {
        mWindow->raise();
        mWindow->activateWindow();
    }
}

void TelemetryDebugView::onClose()
{
    if(mWindow)
    {
        ORIGIN_VERIFY_DISCONNECT(mWindow, SIGNAL(rejected()), this, SLOT(onClose())); 
        GetTelemetryInterface()->RemoveEventListener(this);
        GetTelemetryInterface()->Metric_ODT_TELEMETRY_VIEWER_CLOSED();

        mWindow->hide();
        mWindow->deleteLater();
        mWindow = NULL;

        delete ui;
        ui = NULL;
    }
}

void TelemetryDebugView::processEvent(const TYPE_U64 timestamp, const TYPE_S8 module, const TYPE_S8 group, const TYPE_S8 string, const eastl::map<eastl::string8, TelemetryFileAttributItem*>& data)
{
    const int row = ui->tblEventViewer->rowCount();
    ui->tblEventViewer->insertRow(row);

    const QString sTimestamp = QString("%1\n(%2)").arg(timestamp != 0 ? QDateTime::fromMSecsSinceEpoch(timestamp * 1000, Qt::UTC).toString() : "").arg(QString::number(timestamp, 16));

    QTableWidgetItem *item = new QTableWidgetItem(sTimestamp);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    ui->tblEventViewer->setItem(row, kColumnHeaderTimeReceived, item);

    item = new QTableWidgetItem(module);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    ui->tblEventViewer->setItem(row, kColumnHeaderModule, item);

    item = new QTableWidgetItem(group);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    ui->tblEventViewer->setItem(row, kColumnHeaderGroup, item);

    item = new QTableWidgetItem(string);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    // Add attribute data as user data so we can show it when the user selects this row.
    QString userData;
    for(auto it = data.begin(); it != data.end(); ++it)
    {
        TelemetryFileAttributItem* attr = it->second;
        if(attr)
        {
            userData += it->first.c_str();
            userData += ": ";

            TelemetryDataFormatType *tmp = (TelemetryDataFormatType*)&attr->value;
            switch(attr->type)
            {
                case TYPEID_I8:
                    userData += QString::number(tmp->i8);
                    break;

                case TYPEID_I16:
                    userData += QString::number(tmp->i16);
                    break;

                case TYPEID_U16:
                    userData += QString::number(tmp->u16);
                    break;

                case TYPEID_I32:
                    userData += QString::number(tmp->i32);
                    break;

                case TYPEID_U32:
                    userData += QString::number(tmp->u32);
                    break;

                case TYPEID_I64:
                    userData += QString::number(tmp->i64);
                    break;

                case TYPEID_U64:
                    userData += QString::number(tmp->u64);
                    break;

                case TYPEID_S8:
                    userData.append(QByteArray((char8_t*)&attr->value, attr->valueSize));
                    break;

                case TYPEID_S16:
                case TYPEID_S32:
                default:
                    {
                        // Unsupported types, so hex dump
                        QByteArray ba(&attr->value, attr->valueSize);
                        userData += QString("[%1] %2").arg(QString::number(attr->type)).arg(QString(ba.toHex()));
                    }
                    break;
            }

            userData += "\n";
        }
    }
    item->setData(Qt::UserRole, userData);

    ui->tblEventViewer->setItem(row, kColumnHeaderString, item);

    // Invoke on next event loop because setRowHidden doesn't seem to apply to newly added rows.
    QMetaObject::invokeMethod(this, "updateRowVisibility", Qt::QueuedConnection, Q_ARG(int, row));
}

void TelemetryDebugView::onSelectionChanged()
{
    // Clear any previous data.
    ui->txtEventDetails->clear();

    // Populate file info table with data from selected file.
    QList<QTableWidgetItem*> items = ui->tblEventViewer->selectedItems();
    if(!items.isEmpty())
    {
        int row = items[0]->row();

        QTableWidgetItem* item = ui->tblEventViewer->item(row, kColumnHeaderString);
        if(item && !item->data(Qt::UserRole).isNull())
        {
            ui->txtEventDetails->setText(item->data(Qt::UserRole).toString());
        }
    }
}

void TelemetryDebugView::onApplyFilters()
{
    mCurrentModuleFilter = ui->leModule->text();
    mCurrentGroupFilter = ui->leGroup->text();
    mCurrentStringFilter = ui->leString->text();
    mCurrentFilterBehavior = (FilterBehavior)ui->ddMatch->currentData(Qt::UserRole).toInt();

    for(int row = 0; row < ui->tblEventViewer->rowCount(); ++row)
    {
        updateRowVisibility(row);
    }
}

void TelemetryDebugView::onClearFilters()
{
    ui->leModule->setText("");
    ui->leGroup->setText("");
    ui->leString->setText("");
    ui->ddMatch->setCurrentIndex(kFilterBehaviorMatchAll);
    
    onApplyFilters();
}

void TelemetryDebugView::onViewXML()
{
    QString telemetryFile = QString("%1Telemetry%2telemetryFile.xml").arg(Services::PlatformService::commonAppDataPath()).arg(QDir::separator());
    GetTelemetryInterface()->Metric_ODT_BUTTON_PRESSED("view_telemetry_xml");
    
    if (!QFile::exists(telemetryFile))
    {
        UIToolkit::OriginWindow::alert(UIToolkit::OriginMsgBox::Error, "VIEW TELEMETRY XML", "Could not load telemetry file.  File does not exist: " + telemetryFile, tr("ebisu_client_notranslate_close"));
    }
    else
    {
        Services::PlatformService::asyncOpenUrl(QUrl("file:///" + telemetryFile));
    }
}

void TelemetryDebugView::clear()
{
    ui->tblEventViewer->clearContents();
    ui->txtEventDetails->clear();
    ui->tblEventViewer->setRowCount(0);
}

void TelemetryDebugView::updateRowVisibility(int rowNumber)
{
    const QString& rowModule = ui->tblEventViewer->item(rowNumber, kColumnHeaderModule)->text();
    const QString& rowGroup = ui->tblEventViewer->item(rowNumber, kColumnHeaderGroup)->text();
    const QString& rowString = ui->tblEventViewer->item(rowNumber, kColumnHeaderString)->text();

    const bool moduleMatched = rowModule.contains(mCurrentModuleFilter, Qt::CaseInsensitive);
    const bool groupMatched = rowGroup.contains(mCurrentGroupFilter, Qt::CaseInsensitive);
    const bool stringMatched = rowString.contains(mCurrentStringFilter, Qt::CaseInsensitive);

    bool filterMatched = false;
    if(mCurrentFilterBehavior == kFilterBehaviorMatchAny)
    {
        // Only compare if filter is not empty, since an empty string will "match" all fields.
        if(!mCurrentModuleFilter.isEmpty())
            filterMatched |= moduleMatched;
        
        if(!mCurrentGroupFilter.isEmpty())
            filterMatched |= groupMatched;
        
        if(!mCurrentStringFilter.isEmpty())
            filterMatched |= stringMatched;
    }
    else
    {
        filterMatched = moduleMatched && groupMatched && stringMatched;
    }

    ui->tblEventViewer->setRowHidden(rowNumber, !filterMatched);
}

}
}
}