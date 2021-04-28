// Copyright (c) Electronic Arts, Inc. -- All Rights Reserved.

#include "CrashReportUploader.h"
#include "services/log/LogService.h"
#include "TelemetryAPIDLL.h"
#include "DirtySDK/dirtysock/netconn.h"

#include "services/settings/SettingsManager.h"

#include <coreallocator/icoreallocator_interface.h>
#include "base64.h"
#include "version/version.h"

const char* TELEMETRY_SERVICEDOMAIN = 
#if defined(ORIGIN_PC)
        "-servicename=ORIGIN-SDK14-PC";
#elif defined(ORIGIN_MAC)
        "-servicename=ORIGIN-SDK14-MAC";
#else
    #error Need to define telemetry domain name
#endif

CrashReportUploader::CrashReportUploader( CrashReportData& crashData, QObject *parent ) :
    QObject( parent )
    , mCrashData( crashData )
    , mCrashReportingEnabled( crashData.mOptOutMode != Origin::Services::Never )
    , mSku( crashData.mSKU.toLocal8Bit() )
    , mBuildSignature( crashData.mBuildSignature.toLocal8Bit() )
{
    // Initialize DirtySock to properly support SSL
    NetConnStartup(TELEMETRY_SERVICEDOMAIN);

    // Build BugSentry configuration.
    mBugSentryConfig.mAllocator        = EA::Allocator::ICoreAllocator::GetDefaultAllocator();
    mBugSentryConfig.mSku              = mSku.constData();
    mBugSentryConfig.mBuildSignature   = mBuildSignature.constData();
    mBugSentryConfig.mEnableScreenshot = false;
    mBugSentryConfig.mMode             = ( EA::BugSentry::BugSentryMode ) crashData.mBugSentryMode;
    
    // Initialize the BugSentry manager.
    mBugSentryManager = new EA::BugSentry::BugSentryMgr(&mBugSentryConfig);
}

void CrashReportUploader::setCrashReportingEnabled( bool isEnabled )
{
    mCrashReportingEnabled = isEnabled;
}

void CrashReportUploader::sendTelemetryAndCrashReport( QString additionalInfo )
{
    if ( mCrashReportingEnabled )
    {
        // Trim leading and trailing whitespace.
        additionalInfo = additionalInfo.trimmed();
        
        if ( additionalInfo.length() > 0 )
        {
            // Escape all special characters in the additional info with XML entities.
            QString buf;
            QXmlStreamWriter encoder( &buf );
            encoder.writeCharacters( additionalInfo );
            additionalInfo = buf;

            // Truncate the additional info at 1k.
            additionalInfo.truncate( MAXIMUM_ADDITIONAL_INFO_TEXT_BYTES );
            
            // Add section markers.
            additionalInfo.prepend( "\n[ADDITIONAL USER SUPPLIED INFO]\n" );
            additionalInfo.append( "\n\n" );
            
            // Add the additional info to the beginning of the report text.
            mCrashData.mReportText.prepend( additionalInfo.toUtf8() );
        }

        uploadCrashReport();
    }
}

void CrashReportUploader::uploadCrashReport()
{
    QByteArray encodedReport;
    encodedReport.resize( Base64EncodedSize( mCrashData.mReportText.length() ) );

    ORIGIN_LOG_EVENT << "Uploading crash report for: " << mCrashData.mCategoryIdString << ", " << encodedReport.size() << " bytes";
    
    Base64Encode( ( int32_t ) mCrashData.mReportText.length(), mCrashData.mReportText.data(), encodedReport.data() );
    
    mBugSentryManager->ReportCrash( 0, 0, encodedReport.data(), mCrashData.mCategoryIdString.constData() );

    GetTelemetryInterface()->Metric_BUG_REPORT( ( char8_t* ) mBugSentryManager->GetSessionId(), mCrashData.mCategoryIdString.constData() );

    OriginTelemetry::release();
}
