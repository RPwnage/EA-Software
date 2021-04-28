// -- FILE ------------------------------------------------------------------
// name       : RtfParserListenerFileLogger.cs
// project    : RTF Framelet
// created    : Jani Giannoudis - 2008.06.03
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using System.IO;
//using System.Text;
//using System.Globalization;

#ifndef __RTFPARSERLISTENERFILELOGGER_H
#define __RTFPARSERLISTENERFILELOGGER_H

#include <qfile.h>
#include <qtextstream.h>
#include "RtfParserListenerBase.h"

//define USE_SETTINGS

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfParserListenerFileLogger : public RtfParserListenerBase
    {
        public:

            // ----------------------------------------------------------------------
            RtfParserListenerFileLogger( QString fileName, QObject* parent = 0 );
            // ----------------------------------------------------------------------
            virtual ~RtfParserListenerFileLogger();


            // ----------------------------------------------------------------------
#if USE_SETTINGS
            RtfParserListenerFileLogger( string fileName, RtfParserLoggerSettings settings );
#endif

            // ----------------------------------------------------------------------
            QString& FileName();

#if USE_SETTINGS
            // ----------------------------------------------------------------------
            RtfParserLoggerSettings* Settings ();
#endif

            // ----------------------------------------------------------------------
            void CloseStream ();

        protected:
            // ----------------------------------------------------------------------
            virtual void DoParseBegin();

            // ----------------------------------------------------------------------
            virtual void DoGroupBegin();

            // ----------------------------------------------------------------------
            virtual void DoTagFound( RtfTag* tag );

            // ----------------------------------------------------------------------
            virtual void DoTextFound( RtfText* text );

            // ----------------------------------------------------------------------
            virtual void DoGroupEnd();

            // ----------------------------------------------------------------------
            virtual void DoParseSuccess();

            // ----------------------------------------------------------------------
            virtual void DoParseFail( RtfException* reason );

            // ----------------------------------------------------------------------
            virtual void DoParseEnd();

        private:
            // ----------------------------------------------------------------------
            void WriteLine( QString& msg, int codePage = -1 );

            // ----------------------------------------------------------------------
            QString Indent( QString& msg);

            // ----------------------------------------------------------------------
            void EnsureDirectory();

            // ----------------------------------------------------------------------
            void OpenStreamPriv();

            // ----------------------------------------------------------------------
            void CloseStreamPriv();
            // ----------------------------------------------------------------------
            void RtfParserListenerFileLogger::newLogStream(int codePage);

            // ----------------------------------------------------------------------
            // members
            QString mFileName;
#if USE_SETTINGS
            RtfParserLoggerSettings settings;
#endif
            QFile mLogFile;

            //need to delete after each flush if we want to set encoding properly
            //http://stackoverflow.com/questions/26803384/qt-5-encoding-problems-utf-8-windows-1250-windows-1251
            QTextStream* mLogStream;

            int mCodePage;


    }; // class RtfParserListenerFileLogger

} // namespace Itenso.Rtf.Parser
#endif //__RTFPARSERLISTENERFILELOGGER_H
// -- EOF -------------------------------------------------------------------
