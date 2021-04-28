// Copyright (c) Electronic Arts, Inc. -- All Rights Reserved.

#include <QApplication>

#include "CrashReportData.h"

#include "BugSentry/bugsentryconfig.h"

#include "services/settings/SettingsManager.h"

CrashReportData::CrashReportData()
    : mLocale( "en_US" )
    , mOriginPath( QApplication::applicationDirPath() + "/Origin" )
    , mLocalizedFaqUrl( "" )
    , mSKU( QString("ea.ears.ebisu.%1").arg(EA_PLATFORM_DESCRIPTION) )
    , mBuildSignature( "Release.9,0,0,0" )
    , mBugSentryMode( (qint32) EA::BugSentry::BUGSENTRYMODE_TEST )
    , mOptOutMode( (qint32) Origin::Services::AskMe )
    , mCategoryIdString( "beef" )
    , mReportText( " This is a test report" )
{
}


void CrashReportData::writeToDisk( QFile& file ) const
{
    QDataStream out( &file );
    out << mLocale << mOriginPath << mLocalizedFaqUrl << mSKU << mBuildSignature << mBugSentryMode << mOptOutMode << mCategoryIdString << mReportText;
}

//#include <QDebug>

void CrashReportData::loadFromDisk( QFile& file )
{
    if ( ! file.isReadable() )
    {
        return;
    }
    
    QDataStream in( &file );
    in >> mLocale >> mOriginPath >> mLocalizedFaqUrl >> mSKU >> mBuildSignature >> mBugSentryMode >> mOptOutMode >> mCategoryIdString >> mReportText;
    
//    qDebug() << "Read in the following data:"
//    << "\n     mLocale=" << mLocale
//    << "\n     mOriginPath=" << mOriginPath
//    << "\n     mLocalizedFaqURL=" << mLocalizedFaqUrl
//    << "\n     mSKU=" << mSKU
//    << "\n     mBuildSignature=" << mBuildSignature
//    << "\n     mBugSentryMode=" << mBugSentryMode
//    << "\n     mOptOutMode=" << mOptOutMode
//    << "\n     mCategoryIdString=" << mCategoryIdString
//    << "\n\n\n" << mReportText;
//    
}

