///////////////////////////////////////////////////////////////////////////////
// TelemetryBreakpointWindow.cpp
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "TelemetryBreakpointWindow.h"
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

// WARNING: If you change these, you will also need to change them in 'EbisuTelemetryAPI.cpp'.
enum ExitCode
{
    EXITCODE_CONTINUE = 2,
    EXITCODE_IGNORE = 3,
    EXITCODE_IGNOREALL = 4
};

TelemetryBreakpointWindow::TelemetryBreakpointWindow()
: QWidget()
, mText(new QLabel)
, mExitCode(0)
{
    // Hide all window titlebar controls such as close, minimize, maximize, zoom buttons.
    setWindowFlags( Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowStaysOnTopHint );

    QPushButton *continueButton = new QPushButton("&Continue");
    QPushButton *ignoreButton = new QPushButton("&Ignore");
    QPushButton *ignoreAllButton = new QPushButton("Ignore &All");

    QObject::connect(continueButton, SIGNAL(clicked()), this, SLOT(onContinueClicked()));
    QObject::connect(ignoreButton, SIGNAL(clicked()), this, SLOT(onIgnoreClicked()));
    QObject::connect(ignoreAllButton, SIGNAL(clicked()), this, SLOT(onIgnoreAllClicked()));

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(ignoreAllButton);
    hLayout->addWidget(ignoreButton);
    hLayout->addWidget(continueButton);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addWidget(mText);
    vLayout->addLayout(hLayout);

    setLayout(vLayout);
}

TelemetryBreakpointWindow::~TelemetryBreakpointWindow()
{

}

void TelemetryBreakpointWindow::setText(const QString& text )
{
    mText->setText(text);
}

void TelemetryBreakpointWindow::onContinueClicked()
{
    mExitCode = EXITCODE_CONTINUE;
    close();
}

void TelemetryBreakpointWindow::onIgnoreClicked()
{
    mExitCode = EXITCODE_IGNORE;
    close();
}

void TelemetryBreakpointWindow::onIgnoreAllClicked()
{
    mExitCode = EXITCODE_IGNOREALL;
    close();
}
