// -- FILE ------------------------------------------------------------------
// name       : RtfInterpreterListenerFileLogger.cs
// project    : RTF Framelet
// created    : Jani Giannoudis - 2008.06.03
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using System.IO;
//using System.Globalization;
#ifndef __RTFINTERPRETERLISTENERFILELOGGER_H
#define __RTFINTERPRETERLISTENERFILELOGGER_H

#include <qfile.h>
#include <qtextstream.h>

#include "RtfInterpreterListenerBase.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfInterpreterListenerFileLogger : public RtfInterpreterListenerBase
    {
        public:

            // ----------------------------------------------------------------------
            RtfInterpreterListenerFileLogger( QString fileName, QObject* parent = 0 );
            virtual ~RtfInterpreterListenerFileLogger();

#if USE_SETTINGS
            // ----------------------------------------------------------------------
            public RtfInterpreterListenerFileLogger( string fileName, RtfInterpreterLoggerSettings settings )
#endif
            // ----------------------------------------------------------------------
            QString& FileName();

#if USE_SETTINGS
            // ----------------------------------------------------------------------
            public RtfInterpreterLoggerSettings Settings
            {
                get { return settings; }
            } // Settings
#endif
            // ----------------------------------------------------------------------
            void CloseStream ();

        protected:
            // ----------------------------------------------------------------------
            virtual void DoBeginDocument( RtfInterpreterContext* context );
            // ----------------------------------------------------------------------
            virtual void DoInsertText( RtfInterpreterContext* context, QString& text );
            // ----------------------------------------------------------------------
            virtual void DoInsertSpecialChar( RtfInterpreterContext* context, RtfVisualSpecialCharKind::RtfVisualSpecialCharKind kind );
            // ----------------------------------------------------------------------
            virtual void DoInsertBreak( RtfInterpreterContext* context, RtfVisualBreakKind::RtfVisualBreakKind kind );
            // ----------------------------------------------------------------------
#if 0 //UNIMPLEMENTED currently NOT USED
            virtual void DoInsertImage( IRtfInterpreterContext context,
                                        RtfVisualImageFormat format,
                                        int width, int height, int desiredWidth, int desiredHeight,
                                        int scaleWidthPercent, int scaleHeightPercent,
                                        string imageDataHex
                                      )
#endif
            // ----------------------------------------------------------------------
            virtual void DoEndDocument( RtfInterpreterContext* context );

        private:
            // ----------------------------------------------------------------------
            void WriteLine( QString& message );
            // ----------------------------------------------------------------------
            void EnsureDirectory();
            // ----------------------------------------------------------------------
            void OpenStreamPriv();
            // ----------------------------------------------------------------------
            void CloseStreamPriv();

            // ----------------------------------------------------------------------
            // members

            QString mFileName;
            QFile mLogFile;

#if USE_SETTINGS
            RtfInterpreterLoggerSettings settings;
#endif
            QTextStream* mLogStream;

    }; // class RtfInterpreterListenerFileLogger

} // namespace Itenso.Rtf.Interpreter
// -- EOF -------------------------------------------------------------------
#endif //__RTFINTERPRETERLISTENERFILELOGGER_H   