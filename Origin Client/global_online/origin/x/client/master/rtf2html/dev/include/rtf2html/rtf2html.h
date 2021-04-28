// RTF2HMTL converter
// code ported from RtfConverter : http://www.codeproject.com/KB/recipes/RtfConverter/RtfConverter_v1.7.0.zip
//// -- FILE ------------------------------------------------------------------
// name       : Program.cs
// project    : RTF Framelet
// created    : Jani Giannoudis - 2008.05.30
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
// Licensed under: http://www.codeproject.com/info/cpol10.aspx

#ifndef __RTF2HTML_H
#define __RTF2HTML_H

#include <assert.h>
#include <qobject.h>
#include <qstring.h>

namespace RTF2HTML
{
    //forward declare
    class RtfGroup;
    class RtfDocument;
    class RtfInterpreterListenerDocumentBuilder;
    class RtfParserListenerStructureBuilder;
    class RtfHtmlConvertSettings;
    class RtfHtmlConverter;
    class RtfInterpreter;
    class RtfParser;


    class Q_DECL_EXPORT Rtf2Html : public QObject
    {
        public:
            Rtf2Html (bool ignoreBGcolor = false, QObject *parent = 0);
            ~Rtf2Html ();

            ///
            /// \brief converts RTF file to HTML - main function of this lib, calls all three functions below
            /// \param rtfFilename name of the RTF file
            /// \return file converted to HTML format and returned as QString
            /// 
            QString& ConvertRtf (QString rtfFileName);

            ///
            /// \brief Parses the RTF file into RTF data elements
            /// \param rtfFilename - name of the RTF file to parse
            /// \return RtfGroup* - root of the RTF data element tree
            ///
            RtfGroup* ParseRtf (QString rtfFileName);

            ///
            /// \brief Interprets the RTF data element and prepares internal data structure to be used by Convert
            /// \param RtfGroup* - root of the RTF data element tree
            /// \return RtfDocument* - interpreted document tree containing combined elements (has text+font+styling, e.g.)
            ///
            RtfDocument* InterpretRtf (RtfGroup *rtfStructure);

            ///
            /// \brief Traverses the interpreted document tree and converts it to HTML format
            /// \param RtfDocument* - root of the interpreted document tree
            /// \return QString* - returns the HTML content as a QString
            ///
            QString* ConvertHtml (RtfDocument *rtfDocument);

            ///
            /// \brief saves to file the contents of member variable html, file name is original filename passed to ConvertRtf but appended with _qt.html
            /// \return 0 if successful, -1 if error
            ///
            int SaveHtml ();

        private:
            ///
            /// \brief filename without extension
            ///
            QString srcFileName;

            RtfInterpreterListenerDocumentBuilder* docBuilder;
            RtfParserListenerStructureBuilder* structureBuilder;
            RtfHtmlConvertSettings* htmlConvertSettings;
            RtfHtmlConverter* htmlConvert;
            RtfInterpreter* interpreter;
            RtfParser* parser;

            ///
            /// \brief holds the converted html content
            ///
            QString html;

            bool mIgnoreBGcolor;
    };
}

#endif //__RTF2HTML_H