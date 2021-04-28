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

#include "RtfParserListenerFileLogger.h"
#include <qfile.h>
#include <qtextcodec.h>
#include "RtfSpec.h"

namespace RTF2HTML
{
    namespace settings
    {
        QString ParseBeginText =               "ParseBegin";
        QString ParseEndText =                 "ParseEnd";
        QString ParseGroupBeginText =          "GroupBegin";
        QString ParseGroupEndText =            "GroupEnd";
        QString ParseTagText =                 "Tag";
        QString ParseTextText =                "Text";
        QString TextOverflowText =             "OverflowText";
        QString ParseSuccessText =             "ParseSuccess";
        QString ParseFailKnownReasonText =     "ParseFail";
        QString ParseFailUnknownReasonText =   "ParseFailUnknown";
    }
    // ----------------------------------------------------------------------
//      public const string DefaultLogFileExtension = ".parser.log";

    // ----------------------------------------------------------------------
    RtfParserListenerFileLogger::RtfParserListenerFileLogger( QString fileName, QObject* parent )
        : RtfParserListenerBase (parent)
        , mFileName (fileName) //this( fileName, new RtfParserLoggerSettings() )
        , mLogStream (NULL)
    {

    } // RtfParserListenerFileLogger

    RtfParserListenerFileLogger::~RtfParserListenerFileLogger()
    {
        if (mLogStream)
        {
            delete mLogStream;
        }
    }

#if USE_SETTINGS
    // ----------------------------------------------------------------------
    public RtfParserListenerFileLogger( string fileName, RtfParserLoggerSettings settings )
    {
        if ( fileName == null )
        {
            throw new ArgumentNullException( "fileName" );
        }
        if ( settings == null )
        {
            throw new ArgumentNullException( "settings" );
        }

        this.fileName = fileName;
        this.settings = settings;
    } // RtfParserListenerFileLogger
#endif

    // ----------------------------------------------------------------------
    QString& RtfParserListenerFileLogger::FileName ()
    {
        return mFileName;
    } // FileName

#if USE_SETTINGS
    // ----------------------------------------------------------------------
    public RtfParserLoggerSettings Settings
    {
        get { return settings; }
    } // Settings
#endif
    // ----------------------------------------------------------------------
    void RtfParserListenerFileLogger::CloseStream ()
    {
        CloseStreamPriv();
    }

    // ----------------------------------------------------------------------
    void RtfParserListenerFileLogger::DoParseBegin()
    {
        EnsureDirectory();
        OpenStreamPriv();

#if USE_SETTINGS
        if ( settings.Enabled && !string.IsNullOrEmpty( settings.ParseBeginText ) )
        {
            WriteLine( settings.ParseBeginText );
        }
#else
        WriteLine(settings::ParseBeginText );
#endif
    } // DoParseBegin

    // ----------------------------------------------------------------------
    void RtfParserListenerFileLogger::DoGroupBegin()
    {
#if USE_SETTINGS
        if ( settings.Enabled && !string.IsNullOrEmpty( settings.ParseGroupBeginText ) )
        {
            WriteLine( settings.ParseGroupBeginText );
        }
#else
        WriteLine( settings::ParseGroupBeginText );
#endif
    } // DoGroupBegin

    // ----------------------------------------------------------------------
    void RtfParserListenerFileLogger::DoTagFound( RtfTag* tag )
    {
#if USE_SETTINGS
        if ( settings.Enabled && !string.IsNullOrEmpty( settings.ParseTagText ) )
        {
            WriteLine( string.Format(
                           CultureInfo.InvariantCulture,
                           settings.ParseTagText,
                           tag ) );
        }
#else
        if (tag->Name() == "insrsid" && tag->HasValue() && tag->ValueAsNumber() == 4153944)
        {
            char tmp;
            tmp = 'a';
        }

        QString msg = settings::ParseTagText + ": " + tag->ToString();
        WriteLine (msg, 1252);
#endif
    } // DoTagFound

    // ----------------------------------------------------------------------
    void RtfParserListenerFileLogger::DoTextFound( RtfText* text )
    {
        const int MAX_LENGTH = 80;
        QString textOverFlowText = "...";
#if USE_SETTINGS
        if ( settings.Enabled && !string.IsNullOrEmpty( settings.ParseTextText ) )
        {
            string msg = text.Text;
            if ( msg.Length > settings.TextMaxLength && !string.IsNullOrEmpty( settings.TextOverflowText ) )
            {
                msg = msg.Substring( 0, msg.Length - settings.TextOverflowText.Length ) + settings.TextOverflowText;
            }
            WriteLine( string.Format(
                           CultureInfo.InvariantCulture,
                           settings.ParseTextText,
                           msg ) );
        }
#else
        QString msg = text->Text();
        if (msg.length() > MAX_LENGTH)
        {
            msg = msg.left (msg.length() - textOverFlowText.length()) + textOverFlowText;
        }

        msg = msg.prepend(settings::ParseTextText + ": ");
        //get the codepage associated with this text
        WriteLine (msg, text->codePage());
#endif
    } // DoTextFound

    // ----------------------------------------------------------------------
    void RtfParserListenerFileLogger::DoGroupEnd()
    {
#if USE_SETTINGS
        if ( settings.Enabled && !string.IsNullOrEmpty( settings.ParseGroupEndText ) )
        {
            WriteLine( settings.ParseGroupEndText );
        }
#else
        WriteLine( settings::ParseGroupEndText );
#endif
    } // DoGroupEnd

    // ----------------------------------------------------------------------
    void RtfParserListenerFileLogger::DoParseSuccess()
    {
#if USE_SETTINGS
        if ( settings.Enabled && !string.IsNullOrEmpty( settings.ParseSuccessText ) )
        {
            WriteLine( settings.ParseSuccessText );
        }
#else
        WriteLine( settings::ParseSuccessText );
#endif
    } // DoParseSuccess

    // ----------------------------------------------------------------------
    void RtfParserListenerFileLogger::DoParseFail( RtfException* reason )
    {
#if USE_SETTINGS
        if ( settings.Enabled )
        {
            if ( reason != null )
            {
                if ( !string.IsNullOrEmpty( settings.ParseFailKnownReasonText ) )
                {
                    WriteLine( string.Format(
                                   CultureInfo.InvariantCulture,
                                   settings.ParseFailKnownReasonText,
                                   reason.Message ) );
                }
            }
            else
            {
                if ( !string.IsNullOrEmpty( settings.ParseFailUnknownReasonText ) )
                {
                    WriteLine( settings.ParseFailUnknownReasonText );
                }
            }
        }
#else
        if (reason != NULL)
        {
            WriteLine (settings::ParseFailKnownReasonText);
        }
        else
        {
            WriteLine (settings::ParseFailUnknownReasonText);
        }
#endif
    } // DoParseFail

    // ----------------------------------------------------------------------
    void RtfParserListenerFileLogger::DoParseEnd()
    {
#if USE_SETTINGS
        if ( settings.Enabled && !string.IsNullOrEmpty( settings.ParseEndText ) )
        {
            WriteLine( settings.ParseEndText );
        }
#else
        WriteLine( settings::ParseEndText );
#endif
        CloseStream();
    } // DoParseEnd

    // ----------------------------------------------------------------------
    void RtfParserListenerFileLogger::WriteLine( QString& msg, int codePage)
    {
        if (mLogFile.handle() >= 0)
        {
            QString logText = Indent( msg );
            Q_UNUSED (codePage);

            /*
                            //need to switch codecs
                            if (codePage >= 0 && codePage != mCodePage)
                            {
                                if (!(codePage == 0 && mCodePage == 1252 ||
                                    codePage == 1252 && mCodePage == 0))
                                {
            if (codePage == 950)
            {
                char tmp = 1;
            }
                                    newLogStream(codePage);
                                }
                            }
            */

            *mLogStream << logText;
            *mLogStream << "\n";
            mLogStream->flush();
        }
    } // WriteLine

    // ----------------------------------------------------------------------
    QString RtfParserListenerFileLogger::Indent( QString& msg )
    {
        QString indentedStr;

        if (!msg.isEmpty())
        {
            for (int i = 0; i < Level(); i++)
            {
                indentedStr += " ";
            }
            indentedStr += msg;
        }
        return indentedStr;
    } // Indent

    // ----------------------------------------------------------------------
    void RtfParserListenerFileLogger::EnsureDirectory()
    {
        /*
                    FileInfo fi = new FileInfo( fileName );
                    if ( !string.IsNullOrEmpty( fi.DirectoryName ) && !Directory.Exists( fi.DirectoryName ) )
                    {
                        Directory.CreateDirectory( fi.DirectoryName );
                    }
        */
    } // EnsureDirectory

    // ----------------------------------------------------------------------
    void RtfParserListenerFileLogger::OpenStreamPriv()
    {
        //  Open and initialize log file
        mLogFile.setFileName(mFileName);
        if (mLogFile.open(QIODevice::WriteOnly | QIODevice::Text) == true)
        {
            newLogStream(0);
        }
        //TODO: need to add error checking here
    } // OpenStream

    // ----------------------------------------------------------------------
    void RtfParserListenerFileLogger::CloseStreamPriv()
    {
        if (mLogFile.handle() >= 0)
        {
            mLogStream->flush();
            mLogFile.close();

            delete mLogStream;
        }
        mLogStream = NULL;
    } // OpenStream

    void RtfParserListenerFileLogger::newLogStream(int codePage)
    {
        if (mLogStream)
        {
            delete (mLogStream);
        }
        mLogStream = new QTextStream(&mLogFile);

        mCodePage = codePage;   //keep track of current codepage
        mLogStream->setCodec ("UTF-8");
    }
} // namespace Itenso.Rtf.Parser
// -- EOF -------------------------------------------------------------------
