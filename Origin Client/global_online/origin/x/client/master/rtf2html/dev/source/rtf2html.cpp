
#include <assert.h>
#include "rtf2html\rtf2html.h"

#include "RtfGroup.h"
#include "RtfParserListenerFileLogger.h"
#include "RtfParserListenerStructureBuilder.h"
#include "RtfParser.h"

#include "RtfDocument.h"
#include "RtfInterpreterListenerDocumentBuilder.h"
#include "RtfInterpreterListenerFileLogger.h"
#include "RtfInterpreter.h"

#include "RtfHtmlConvertSettings.h"
#include "RtfHtmlConverter.h"


#define LOG_PARSER 0
#define LOG_INTERPRETER 0

namespace RTF2HTML
{
    Rtf2Html::Rtf2Html (bool ignoreBGcolor, QObject* parent)
        : QObject (parent)
        , docBuilder (NULL)
        , structureBuilder (NULL)
        , htmlConvertSettings(NULL)
        , htmlConvert (NULL)
        , interpreter(NULL)
        , parser (NULL)
        , mIgnoreBGcolor (ignoreBGcolor)
    {
    }

    Rtf2Html::~Rtf2Html()
    {
    }

    QString& Rtf2Html::ConvertRtf (QString rtfFileName)
    {
        QString fileName (rtfFileName);

        //strip the .rtf ending
        int extensionNdx = fileName.lastIndexOf (".rtf", -1, Qt::CaseInsensitive);
        if (extensionNdx == -1) //not found
        {
            srcFileName = fileName;
        }
        else
        {
            srcFileName = fileName.left (extensionNdx);
        }

        //parse the file first
        RtfGroup* rtfStructure = ParseRtf(fileName);

        //now interpret it
        RtfDocument* rtfDocument = InterpretRtf (rtfStructure);

        //convert will actually write it to member html
        ConvertHtml (rtfDocument);
        return html;
    }

    QString* Rtf2Html::ConvertHtml (RtfDocument* rtfDocument)
    {
        htmlConvertSettings = new RtfHtmlConvertSettings (this);
        QString srcFileNameOnly = srcFileName;
        //strip out the path
        int index = srcFileNameOnly.lastIndexOf ("\\");
        if (index > 0)
        {
            srcFileNameOnly = srcFileNameOnly.right (srcFileNameOnly.length() - (index+1));
        }
        htmlConvertSettings->setTitle (srcFileNameOnly);
        htmlConvertSettings->setIgnoreBackgroundColor( mIgnoreBGcolor );  // we want it to just appear on the dialog background

        htmlConvert = new RtfHtmlConverter (rtfDocument, htmlConvertSettings, this);

        html = htmlConvert->Convert();
        return &html;
    }

    RtfDocument* Rtf2Html::InterpretRtf (RtfGroup* rtfStructure)
    {
        docBuilder = new RtfInterpreterListenerDocumentBuilder(this);
#if LOG_INTERPRETER
        RtfInterpreterListenerFileLogger* interpreterLogger = new RtfInterpreterListenerFileLogger("interpreterLog.txt");
#endif
        RtfInterpreterListenerList lList;
        lList.push_back ((RtfInterpreterListener*)docBuilder);
#if LOG_INTERPRETER
        lList.push_back ((RtfInterpreterListener*)interpreterLogger);
#endif

        interpreter = new RtfInterpreter( lList, this );
        interpreter->Interpret (rtfStructure);

#if LOG_INTERPRETER
        if (interpreterLogger)
        {
            delete interpreterLogger;
        }
#endif
        return docBuilder->Document();
    }

    RtfGroup* Rtf2Html::ParseRtf (QString rtfFileName)
    {
        FILE* fp;
        char* utf8Buffer;

#if LOG_PARSER
        RtfParserListenerFileLogger* parserLogger = new RtfParserListenerFileLogger("parserLog.txt");
#endif
        structureBuilder = new RtfParserListenerStructureBuilder(this);

        RtfParserListenerList lList;
        lList.push_back ((RtfParserListener*)structureBuilder);
#if LOG_PARSER
        lList.push_back ((RtfParserListener*)parserLogger);
#endif
        parser = new RtfParser(lList, this);

        parser->setIgnoreContentAfterRootGroup(true); // support WordPad documents

        //read in the rtf file (assume encded as utf-8) and convert it to unicode buffer
        //and leave the strings as unicode so that it can be converted to a different code page easily.
        fp = fopen(rtfFileName.toLatin1(), "rb");
        if (fp == NULL)
        {
            assert (0);
            return NULL;
        }

        long  fileLen;
        fseek (fp, 0, SEEK_END);
        fileLen = ftell (fp);
        fseek (fp, 0, SEEK_SET);

        //allocate a buffer
        utf8Buffer = (char*)malloc (fileLen* sizeof(char));
        int numRead = fread (utf8Buffer, sizeof (char), fileLen, fp);
        if (numRead != fileLen)
        {
            assert (0);
        }
        fclose (fp);

        parser->Parse (utf8Buffer, numRead);

        RtfGroup* rtfStructure = structureBuilder->StructureRoot();

        if (utf8Buffer)
        {
            free (utf8Buffer);
        }

#if LOG_PARSER
        if (parserLogger)
        {
            free (parserLogger);
        }
#endif
        return rtfStructure;
    }

    int Rtf2Html::SaveHtml ()
    {
        QFile htmlFile;
        QTextStream* htmlStream;
        int err = 0;

        htmlFile.setFileName(srcFileName + "_qt.html");
        if (htmlFile.open(QIODevice::WriteOnly | QIODevice::Text) == true)
        {
            htmlStream = new QTextStream(&htmlFile);
            htmlStream->setCodec ("UTF-8");

            *htmlStream << html;

            if (htmlFile.handle() >= 0)
            {
                htmlStream->flush();
                htmlFile.close();

                delete htmlStream;
            }
        }
        else
        {
            err = -1;
        }
        return err;
    }

} //namespace