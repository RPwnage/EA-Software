///////////////////////////////////////////////////////////////////////////////
// CrashReportData.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef CRASHREPORTDATA_H
#define CRASHREPORTDATA_H

#include <QString>
#include <QFile>
#include <QDataStream>

namespace Origin
{
    namespace Services
    {
        namespace Exception
        {
            /// The data structure containing all the details necessary to prompt the user and submit bugs to EA.
            /// This structure is lifted from the OriginCrashReporter's CrashReportData.h header.
            struct CrashReportData
            {
                /// Writes out the crash report data in a format compatible with the OriginCrashReporter.
                void writeToDisk( QFile& file ) const
                {
                    QDataStream out( &file );
                    out << mLocale << mOriginPath << mLocalizedFaqUrl << mSKU << mBuildSignature << mBugSentryMode << mOptOutMode << mCategoryIdString << mReportText;
                }
            
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
        }
    }
}

#endif // Header include guard
