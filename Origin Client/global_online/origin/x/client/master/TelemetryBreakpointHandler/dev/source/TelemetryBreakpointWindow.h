///////////////////////////////////////////////////////////////////////////////
// TelemetryBreakpointWindow.h
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef TELEMETRYBREAKPOINTWINDOW_H
#define TELEMETRYBREAKPOINTWINDOW_H

#include <EABase/eabase.h>
#include <QWidget>
#include <QString>
#include <QLabel>

class TelemetryBreakpointWindow : public QWidget
{
    Q_OBJECT

public:
    TelemetryBreakpointWindow();
    ~TelemetryBreakpointWindow();

    void setText(const QString& text );
    int exitCode() const { return mExitCode; }

private slots:
    void onContinueClicked();
    void onIgnoreClicked();
    void onIgnoreAllClicked();

private:
    QLabel *mText;
    int mExitCode;
};
#endif // TELEMETRYBREAKPOINTWINDOW_H