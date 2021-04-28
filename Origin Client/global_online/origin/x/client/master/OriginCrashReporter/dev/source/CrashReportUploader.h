// Copyright (c) Electronic Arts, Inc. -- All Rights Reserved.

#ifndef CRASHREPORTUPLOADER_H
#define CRASHREPORTUPLOADER_H

#include <QObject>

#include "CrashReportData.h"

#include <BugSentry/bugsentryconfig.h>
#include <BugSentry/bugsentrymgr.h>

// The maximum length of any user-supplied additional info about the crash included in the report.
#define MAXIMUM_ADDITIONAL_INFO_TEXT_BYTES 1024

class CrashReportUploader : public QObject
{
    Q_OBJECT
public:
    explicit CrashReportUploader( CrashReportData& crashData, QObject *parent = 0 );
    
    void uploadCrashReport();
    
private:
    CrashReportData& mCrashData;
    bool mCrashReportingEnabled;
    
    QByteArray mSku;
    QByteArray mBuildSignature;
    
    EA::BugSentry::BugSentryConfig     mBugSentryConfig;
    EA::BugSentry::BugSentryMgr*       mBugSentryManager;

public slots:
    void setCrashReportingEnabled( bool );
    void sendTelemetryAndCrashReport( QString ); // param is any user-supplied additional info about the crash.
    
};

#endif // CRASHREPORTUPLOADER_H
