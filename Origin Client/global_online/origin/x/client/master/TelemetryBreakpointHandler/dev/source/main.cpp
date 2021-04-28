// Copyright (c) Electronic Arts, Inc. -- All Rights Reserved.

#include "TelemetryBreakpointWindow.h"
#include <QApplication>

#ifdef ORIGIN_MAC
#   include "signal.h"
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QStringList args = QApplication::arguments();

    // Bail if we do not have exactly one command line argument
    if ( args.count() != 2 ) return 1;

    TelemetryBreakpointWindow window;
    QString text = "Telemetry Breakpoint Hit:\n";
    text += args.at(1);

    window.setText(text);
    window.show();

	app.exec();

    return window.exitCode();
}
