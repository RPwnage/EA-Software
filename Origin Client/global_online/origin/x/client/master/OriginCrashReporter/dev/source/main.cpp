// Copyright (c) Electronic Arts, Inc. -- All Rights Reserved.

#include <QtWidgets/QApplication>

#include "CrashReportData.h"
#include "CrashReportWindow.h"
#include "CrashReportUploader.h"
#include "OriginLauncher.h"
#include "crashreportwindow.h"
#include "EbisuTranslator.h"
#include "TelemetryAPIDLL.h"

#include "services/settings/SettingsManager.h"

int main(int argc, char *argv[])
{
    // Initialize and startup the application.
    Q_INIT_RESOURCE(OriginCrashReporter);
    QApplication app(argc, argv);

#ifdef ORIGIN_PC
    app.setWindowIcon(QIcon(":/Resources/eadm.png"));
#endif

    QStringList args = QApplication::arguments();
    
    // Bail if we do not have exactly one command line argument
    if ( args.count() != 2 ) return 1;
    
    QString filenameArgument = args.at( 1 );

    // Prepare to load the crash report data.
    CrashReportData crashData;

    // If the command line does not specify a dummy crash,
    if ( filenameArgument != "-fake" )
    {
        // Load the specified crash report data file from disk.
        QFile crashReport( filenameArgument );
        crashReport.open( QIODevice::ReadOnly );
        crashData.loadFromDisk( crashReport );
        crashReport.close();
        
        // Delete the crash report data file.
        crashReport.remove();
    }

    OriginTelemetry::init("ocr_");
    // Get the telemetry pump going so we're ready to send.
    GetTelemetryInterface()->startSendingTelemetry();
    
    // Initialize the language translation system.
    EbisuTranslator* translator = new EbisuTranslator;
    app.installTranslator( translator );
    translator->loadHAL( ":/Resources/lang/CrashStrings_", crashData.mLocale );
    QLocale::setDefault( QLocale( crashData.mLocale ) );

    
    // Initialize the uploader with the crash report data.
    CrashReportUploader uploader( crashData );

    // Initialize the Origin relauncher.
    OriginLauncher launcher( crashData.mOriginPath );
    
    // If the user should be prompted,
    if ( crashData.mOptOutMode == Origin::Services::AskMe )
    {
        // Initialize the crash report dialog window.
        CrashReportWindow crashDialog( crashData );
        
        // Hook up the dialog to the appropriate functions.
        QApplication::connect( &crashDialog, SIGNAL( signalReportShouldBeSent( bool ) ), &uploader, SLOT( setCrashReportingEnabled( bool ) ) );
        QApplication::connect( &crashDialog, SIGNAL( dialogFinished( QString ) ), &uploader, SLOT( sendTelemetryAndCrashReport( QString ) ) );
        QApplication::connect( &crashDialog, SIGNAL( signalRestartOrigin() ), &launcher, SLOT( launchOrigin() ) );

        // Display and execute the dialog.
        crashDialog.show();
        return app.exec();
    }
    
    // Otherwise, if the upload should happen automatically,
    else if ( crashData.mOptOutMode == Origin::Services::Always )
    {
        // Upload the report.
        uploader.setCrashReportingEnabled( true );
        uploader.sendTelemetryAndCrashReport( "" );
        
        // Restart Origin.
        launcher.launchOrigin();
    }
    
    // Otherwise, simply exit.
    return 0;
}
