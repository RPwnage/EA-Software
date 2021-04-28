///////////////////////////////////////////////////////////////////////////////
// ProgressView.cpp
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ui_ProgressView.h"
#include "ProgressView.h"

#include "Services/debug/DebugService.h"

ProgressViewWidget::ProgressViewWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ProgressViewWidget)
{
    ui->setupUi(this);
}

ProgressViewWidget::~ProgressViewWidget()
{
    if (ui)
    {
        delete ui;
        ui = NULL;

    }
}

void ProgressViewWidget::setProgressCancel()
{
    ui->mProgressStateLabel->setText(tr("diag_tool_canceling"));
}

void ProgressViewWidget::onBackendProgress(DataCollectedType completedType, DataCollectedType startingType)
{
    // UI UPDATES

    switch (startingType)
    {
    case START_OR_END:
        {
            emit submitFinished();
            break;
        }
    case BOOTSTRAPPER_LOG:
        {
            ui->mProgressStateLabel->setText(tr("diag_tool_collecting_bootstrapper_log"));
            break;
        }
    case HARDWARE:
        {
            ui->mProgressStateLabel->setText(tr("diag_tool_collecting_hardware_survey"));
            break;
        }
    case DXDIAG_DUMP:
        {
            ui->mProgressStateLabel->setText(tr("diag_tool_collecting_dxdiag"));
            break;
        }
    case READ_DXDIAG:
        {
            ui->mProgressStateLabel->setText(tr("diag_tool_processing_dxdiag"));
            break;
        }
    case NETWORK:
        {
            ui->mProgressStateLabel->setText(tr("diag_tool_submitting_diagnostics"));
            break;
        }
    }
}
