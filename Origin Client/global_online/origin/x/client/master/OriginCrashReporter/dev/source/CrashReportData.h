// Copyright (c) Electronic Arts, Inc. -- All Rights Reserved.

#ifndef CRASHREPORTDATA_H
#define CRASHREPORTDATA_H

#include <EAStdC/EAString.h>
#include <EASTL/fixed_string.h>
#include <QFile>
#include <QString>

struct CrashReportData
{
    CrashReportData();
    
    void writeToDisk( QFile& file ) const;
    void loadFromDisk( QFile& file );
    
    // The locale to use
    QString mLocale;
    
    // The path to Origin.app
    QString mOriginPath;
    
    // The URL to open when clicking for crash report details. (i.e., the client log file)
    QString mLocalizedFaqUrl;

    // The BugSentry SKU string
    QString mSKU;
    
    // The BugSentry build signature
    QString mBuildSignature;
    
    // The BugSentry configuration mode (i.e., TEST or PROD)
    qint32 mBugSentryMode;
    
    // The Opt-Out state -- whether or not to automatically upload the crash report without user intervention.
    qint32 mOptOutMode;
    
    // String identifier contains a portion of the crash address.
    QByteArray mCategoryIdString; // 0x1234abcd => "abcd"
    
    // The crash report itself.
    QByteArray mReportText;
};

#endif // CRASHREPORTDATA_H
