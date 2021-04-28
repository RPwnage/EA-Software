// -- FILE ------------------------------------------------------------------
// name       : IRtfParser.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.19
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using System.IO;

#ifndef __RTFPARSER_H
#define __RTFPARSER_H

#include <qstack.h>
#include <qhash.h>

#include "RtfParserListener.h"
#include "RtfParserBase.h"
#include "RtfSource.h"
#include "icuconverter.h"
#include "icustring.h"

namespace RTF2HTML
{
    // ------------------------------------------------------------------------
    class RtfParser: public RtfParserBase
    {
        public:
            RtfParser(QObject* parent = 0);
            // ----------------------------------------------------------------------
            RtfParser( RtfParserListenerList& listeners, QObject* parent = 0 );

            // ----------------------------------------------------------------------
            /// <summary>
            /// Parses the given RTF text that is read from the given source.
            /// </summary>
            /// <param name="rtfTextSource">the source with RTF text to parse</param>
            /// <exception cref="RtfException">in case of invalid RTF syntax</exception>
            /// <exception cref="IOException">in case of an IO error</exception>
            /// <exception cref="ArgumentNullException">in case of a null argument</exception>
            void Parse( char* srcBuffer, size_t srcBufSz );

            void DoParse( char* srcBuffer, size_t srcBufSz );

        private:
            void ParseRtf(); // std::wifstream* reader );
            bool HandleTag (RtfTag* tag);
            void ParseTag();
            void DecodeCurrentHexBuffer();


            void UpdateEncoding( RtfTag* tag );
            void UpdateEncoding( int codePage );

            //character retrieval from ICUString
            UChar32 PeekNextChar ();
            UChar32 ReadOneChar ();
            void ResetCharIndex ();

            //accumulation of temporary text
            void FlushText ();

            //utils
            bool IsASCIILetter( UChar32 character );
            bool IsHexDigit( UChar32 character, char* hexChar );
            bool IsDigit( UChar32 character );

            QString convertUnicodeToQString (ICUString& uniStr);


            QStack<int> unicodeSkipCountStack;
            int unicodeSkipCount;

            QStack<int> codePageStack;

            int level;
            int tagCountAtLastGroupStart;
            int tagCount;
            int fontTableStartLevel;

            QString targetFont;
            bool expectingThemeFont;

            QHash<QString, int> fontToCodePageMapping;

            QByteArray hexDecodingBuffer;

            ICUConverter encoding;
            ICUString mUnicodeStr;   //actually the unicode version of the src buffer
            int32_t mUnicodeStrLen;
            int32_t mCurrCharIndex;   //keep track of where we are in the string for use by peek/getch

            ICUString curText;
    }; // interface IRtfParser
} // namespace Itenso.Rtf
#endif //__RTFPARSER_H
// -- EOF -------------------------------------------------------------------
