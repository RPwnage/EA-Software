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

#include <assert.h>
#include "RtfInterpreterListenerFileLogger.h"

namespace RTF2HTML
{
    namespace InterpreterSettings
    {
        QString BeginDocumentText = "BeginDocument";
        QString EndDocumentText = "EndDocument";
        QString TextFormatText = "InsertText";
        QString TextOverflowText = "OverflowText";
        QString SpecialCharFormatText = "InsertChar";
        QString BreakFormatText = "InsertBreak";
        QString ImageFormatText = "InsertImage";
    }

    // ----------------------------------------------------------------------
    //public const string DefaultLogFileExtension = ".interpreter.log";

    // ----------------------------------------------------------------------
    RtfInterpreterListenerFileLogger::RtfInterpreterListenerFileLogger( QString fileName, QObject* parent )
        : RtfInterpreterListenerBase (parent)
        , mFileName (fileName) //this( fileName, new RtfParserLoggerSettings() )
        , mLogStream (NULL)
    {
    } // RtfInterpreterListenerFileLogger

    RtfInterpreterListenerFileLogger::~RtfInterpreterListenerFileLogger()
    {
        if (mLogStream)
        {
            delete mLogStream;
        }
    }

#if USE_SETTINGS
    // ----------------------------------------------------------------------
    public RtfInterpreterListenerFileLogger( string fileName, RtfInterpreterLoggerSettings settings )
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
    } // RtfInterpreterListenerFileLogger
#endif
    // ----------------------------------------------------------------------
    QString& RtfInterpreterListenerFileLogger::FileName ()
    {
        return mFileName;
    } // FileName

#if USE_SETTINGS
    // ----------------------------------------------------------------------
    public RtfInterpreterLoggerSettings Settings
    {
        get { return settings; }
    } // Settings
#endif

    // ----------------------------------------------------------------------
    void RtfInterpreterListenerFileLogger::CloseStream()
    {
        CloseStreamPriv();
    } // Dispose

    // ----------------------------------------------------------------------
    void RtfInterpreterListenerFileLogger::DoBeginDocument( RtfInterpreterContext* context )
    {
        Q_UNUSED (context);
        EnsureDirectory();
        OpenStreamPriv();

#if USE_SETTINGS
        if ( settings.Enabled && !string.IsNullOrEmpty( settings.BeginDocumentText ) )
        {
            WriteLine( settings.BeginDocumentText );
        }
#else
        WriteLine( InterpreterSettings::BeginDocumentText );
#endif
    } // DoBeginDocument

    // ----------------------------------------------------------------------
    void RtfInterpreterListenerFileLogger::DoInsertText( RtfInterpreterContext* context, QString& text )
    {
#if USE_SETTINGS
        if ( settings.Enabled && !string.IsNullOrEmpty( settings.TextFormatText ) )
        {
            string msg = text;
            if ( msg.Length > settings.TextMaxLength && !string.IsNullOrEmpty( settings.TextOverflowText ) )
            {
                msg = msg.Substring( 0, msg.Length - settings.TextOverflowText.Length ) + settings.TextOverflowText;
            }
            WriteLine( string.Format(
                           CultureInfo.InvariantCulture,
                           settings.TextFormatText,
                           msg,
                           context.CurrentTextFormat ) );
        }
#else
        const int MAX_LENGTH = 80;
        QString textOverFlowText = "...";

        QString msg = text;
        if (msg.length() > MAX_LENGTH)
        {
            msg = msg.left (msg.length() - textOverFlowText.length()) + textOverFlowText;
        }

        RtfTextFormat* formatText = context->CurrentTextFormat();
        QString textFormatStr = formatText->ToString();

        msg = msg + "\'";
        msg = msg.prepend(InterpreterSettings::TextFormatText + ": \'");
        msg += " with format [" + textFormatStr + "]";

        //get the codepage associated with this text
        WriteLine (msg);
#endif
    } // DoInsertText

    // ----------------------------------------------------------------------
    void RtfInterpreterListenerFileLogger::DoInsertSpecialChar( RtfInterpreterContext* context, RtfVisualSpecialCharKind::RtfVisualSpecialCharKind kind )
    {
        Q_UNUSED (context);
#if USE_SETTINGS
        if ( settings.Enabled && !string.IsNullOrEmpty( settings.SpecialCharFormatText ) )
        {
            WriteLine( string.Format(
                           CultureInfo.InvariantCulture,
                           settings.SpecialCharFormatText,
                           kind ) );
        }
#else
        QString msg;
        RtfVisualSpecialChar sc (kind);
        msg = InterpreterSettings::SpecialCharFormatText + ": " + sc.ToString();
        WriteLine (msg);
#endif
    } // DoInsertSpecialChar

    // ----------------------------------------------------------------------
    void RtfInterpreterListenerFileLogger::DoInsertBreak( RtfInterpreterContext* context, RtfVisualBreakKind::RtfVisualBreakKind kind )
    {
        Q_UNUSED (context);
#if USE_SETTINGS
        if ( settings.Enabled && !string.IsNullOrEmpty( settings.BreakFormatText ) )
        {
            WriteLine( string.Format(
                           CultureInfo.InvariantCulture,
                           settings.BreakFormatText,
                           kind ) );
        }
#else
        QString msg;
        RtfVisualBreak vb (kind);
        msg = InterpreterSettings::BreakFormatText + ": " + vb.ToString();
        WriteLine (msg);
#endif
    } // DoInsertBreak

#if 0 //UNIMPLEMENTED currently NOT USED
    // ----------------------------------------------------------------------
    protected override void DoInsertImage( IRtfInterpreterContext context,
                                           RtfVisualImageFormat format,
                                           int width, int height, int desiredWidth, int desiredHeight,
                                           int scaleWidthPercent, int scaleHeightPercent,
                                           string imageDataHex
                                         )
    {
        if ( settings.Enabled && !string.IsNullOrEmpty( settings.ImageFormatText ) )
        {
            WriteLine( string.Format(
                           CultureInfo.InvariantCulture,
                           settings.ImageFormatText,
                           format,
                           width,
                           height,
                           desiredWidth,
                           desiredHeight,
                           scaleWidthPercent,
                           scaleHeightPercent,
                           imageDataHex,
                           (imageDataHex.Length / 2) ) );
        }
    } // DoInsertImage
#endif

    // ----------------------------------------------------------------------
    void RtfInterpreterListenerFileLogger::DoEndDocument( RtfInterpreterContext* context )
    {
        Q_UNUSED (context);
#if USE_SETTINGS
        if ( settings.Enabled && !string.IsNullOrEmpty( settings.EndDocumentText ) )
        {
            WriteLine( settings.EndDocumentText );
        }
#else
        WriteLine (InterpreterSettings::EndDocumentText);
#endif
        CloseStream();
    } // DoEndDocument

    // ----------------------------------------------------------------------
    void RtfInterpreterListenerFileLogger::WriteLine( QString& message )
    {
        if (mLogFile.handle() >= 0)
        {
            *mLogStream << message;
            *mLogStream << "\n";
            mLogStream->flush();
        }
    } // WriteLine

    // ----------------------------------------------------------------------
    void RtfInterpreterListenerFileLogger::EnsureDirectory()
    {
#if 0 //UNIMPLEMENTED currently NOT USED
        FileInfo fi = new FileInfo( fileName );
        if ( !string.IsNullOrEmpty( fi.DirectoryName ) && !Directory.Exists( fi.DirectoryName ) )
        {
            Directory.CreateDirectory( fi.DirectoryName );
        }
#endif
    } // EnsureDirectory

    // ----------------------------------------------------------------------
    void RtfInterpreterListenerFileLogger::OpenStreamPriv()
    {
        //  Open and initialize log file
        mLogFile.setFileName(mFileName);
        if (mLogFile.open(QIODevice::WriteOnly | QIODevice::Text) == true)
        {
            mLogStream = new QTextStream(&mLogFile);
            mLogStream->setCodec ("UTF-8");
        }
    } // OpenStream

    // ----------------------------------------------------------------------
    void RtfInterpreterListenerFileLogger::CloseStreamPriv()
    {
        if (mLogFile.handle() >= 0)
        {
            mLogStream->flush();
            mLogFile.close();

            delete mLogStream;
        }
        mLogStream = NULL;
    } // OpenStream

} // namespace Itenso.Rtf.Interpreter
// -- EOF -------------------------------------------------------------------
