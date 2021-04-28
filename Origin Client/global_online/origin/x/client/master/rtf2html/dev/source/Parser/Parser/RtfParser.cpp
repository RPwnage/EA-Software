// -- FILE ------------------------------------------------------------------
// name       : RtfParser.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.20
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using System.Collections;
//using System.Globalization;
//using System.IO;
//using System.Text;
//using Itenso.Rtf.Model;

#include "RtfParser.h"
#include "RtfSpec.h"
#include <assert.h>
#include <string.h>
#include <qtextcodec.h>

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    // ----------------------------------------------------------------------
    RtfParser::RtfParser(QObject* parent)
        :RtfParserBase(parent)
    {
    } // RtfParser

    // ----------------------------------------------------------------------
    RtfParser::RtfParser( RtfParserListenerList& listeners, QObject* parent )
        : RtfParserBase (listeners, parent)
    {
    } // RtfParser

    // ----------------------------------------------------------------------
    void RtfParser::Parse( char* srcBuffer, size_t srcBufSz )
    {
        if ( srcBuffer == NULL )
        {
            assert (0);
        }
        DoParse( srcBuffer, srcBufSz );
    } // Parse

    // ----------------------------------------------------------------------
    void RtfParser::DoParse( char* srcBuffer, size_t srcBufSz )
    {
        //first take the srcBuffer, convert it to unicode buffer and then convert that to unicodeStr
        mUnicodeStr.SetStringFromBuffer (srcBuffer, srcBufSz, "utf-8");
        ResetCharIndex();

        NotifyParseBegin();

//        try
        {
            ParseRtf(); //rtfTextSource->Reader() );
            NotifyParseSuccess();
        }
        /*
                catch ( RtfException e )
                {
                    NotifyParseFail( e );
                    throw;
                }
        */
        NotifyParseEnd();
    } // DoParse

    // ----------------------------------------------------------------------
    void RtfParser::ParseRtf() // std::wifstream* reader )
    {
        curText.unicodeStr()->remove();

        unicodeSkipCountStack.clear();
        codePageStack.clear();

        unicodeSkipCount = 1;
        level = 0;
        tagCountAtLastGroupStart = 0;
        tagCount = 0;
        fontTableStartLevel = -1;

        targetFont.clear();
        expectingThemeFont = false;

        fontToCodePageMapping.clear();
        hexDecodingBuffer.clear();
        UpdateEncoding( RtfSpec::AnsiCodePage );

        int groupCount = 0;
//      const int eof = -1;
        UChar32 nextChar = PeekNextChar ();
        bool backslashAlreadyConsumed = false;
//      while ( nextChar != std::char_traits<wchar_t>::eof() )
        while (nextChar != 0xFFFF)
        {
            int peekChar = 0;
            bool peekCharValid = false;
            switch ( nextChar )
            {
                case '\\':
                    {
                        if ( !backslashAlreadyConsumed )
                        {
                            ReadOneChar(); // must still consume the 'peek'ed char
                        }

                        UChar32 secondChar = PeekNextChar();
                        switch ( secondChar )
                        {
                            case '\\':
                            case '{':
                            case '}':
                                curText.unicodeStr()->append( ReadOneChar () ); // must still consume the 'peek'ed char
                                break;

                            case '\n':
                            case '\r':
                                {
                                    ReadOneChar (); // must still consume the 'peek'ed char
                                    // must be treated as a 'par' tag if preceded by a backslash
                                    // (see RTF spec page 144)
                                    RtfTag* newParTag = new RtfTag( RtfSpec::TagParagraph, this );

                                    HandleTag( newParTag);
                                }
                                break;

                            case '\'':
                                {
                                    ReadOneChar ();// must still consume the 'peek'ed char
                                    UChar32 hex1 = ReadOneChar (); //(char)ReadOneByte( reader );
                                    UChar32 hex2 = ReadOneChar (); //(char)ReadOneByte( reader );
                                    char hexChar1, hexChar2;
                                    if ( !IsHexDigit( hex1, &hexChar1 ) )
                                    {
                                        assert (0);
                                        //throw new RtfHexEncodingException( Strings.InvalidFirstHexDigit( hex1 ) );
                                    }
                                    if ( !IsHexDigit( hex2, &hexChar2 ) )
                                    {
                                        assert (0);
                                        //throw new RtfHexEncodingException( Strings.InvalidSecondHexDigit( hex2 ) );
                                    }

                                    QString hexString = QString("%1%2").arg(hexChar1).arg(hexChar2);
                                    //int decodedByte = Int32.Parse( "" + hex1 + hex2, NumberStyles.HexNumber );
                                    bool ok;
                                    int decodedByte = hexString.toInt (&ok, 16);
                                    hexDecodingBuffer.append ((char)decodedByte );
                                    peekChar = PeekNextChar();
                                    peekCharValid = true;
                                    bool mustFlushHexContent = true;
                                    if ( peekChar == '\\' )
                                    {
                                        ReadOneChar();
                                        backslashAlreadyConsumed = true;
                                        int continuationChar = PeekNextChar();
                                        if ( continuationChar == '\'' )
                                        {
                                            mustFlushHexContent = false;
                                        }
                                    }
                                    if ( mustFlushHexContent )
                                    {
                                        // we may _NOT_ handle hex content in a character-by-character way as
                                        // this results in invalid text for japanese/chinese content ...
                                        // -> we wait until the following content is non-hex and then flush the
                                        //    pending data. ugly but necessary with our decoding model.
                                        DecodeCurrentHexBuffer();
                                    }
                                }
                                break;

                            case '|':
                            case '~':
                            case '-':
                            case '_':
                            case ':':
                            case '*':
                                {
                                    //a bit kludgy yes, but don't want to expend the effort to convert UChar to ASCII
                                    QString tagLabel;
                                    switch (secondChar)
                                    {
                                        case '|':
                                            tagLabel = RtfSpec::TagBar;
                                            break;
                                        case '~':
                                            tagLabel = RtfSpec::TagTilde;
                                            break;

                                        case '-':
                                            tagLabel = RtfSpec::TagHyphen;
                                            break;
                                        case '_':
                                            tagLabel = RtfSpec::TagUnderscore;
                                            break;
                                        case ':':
                                            tagLabel = RtfSpec::TagColon;
                                            break;
                                        case '*':
                                            tagLabel = RtfSpec::TagExtensionDestination;
                                            break;
                                    }
                                    UChar32 tagChar = ReadOneChar(); // must still consume the 'peek'ed char
                                    Q_UNUSED (tagChar);
                                    RtfTag* charTag = new RtfTag (tagLabel, this);
                                    HandleTag( charTag );
                                }
                                break;

                            default:
                                ParseTag( );
                                break;
                        }
                    }
                    break;

                case '\n':
                case '\r':
                    ReadOneChar();// must still consume the 'peek'ed char
                    break;

                case '\t':
                    ReadOneChar(); // must still consume the 'peek'ed char
                    // should be treated as a 'tab' tag (see RTF spec page 144)
                    HandleTag( new RtfTag( RtfSpec::TagTabulator, this ) );
                    break;

                case '{':
                    ReadOneChar(); // must still consume the 'peek'ed char
                    FlushText();
                    NotifyGroupBegin();
                    tagCountAtLastGroupStart = tagCount;
                    unicodeSkipCountStack.push( unicodeSkipCount );
                    codePageStack.push( encoding.CodePage() );
                    level++;
                    break;

                case '}':
                    ReadOneChar(); // must still consume the 'peek'ed char
                    FlushText();
                    if ( level > 0 )
                    {
                        unicodeSkipCount = (int)unicodeSkipCountStack.pop();
                        if ( fontTableStartLevel == level )
                        {
                            fontTableStartLevel = -1;
                            targetFont.clear();
                            expectingThemeFont = false;
                        }
                        UpdateEncoding( (int)codePageStack.pop() );
                        level--;
                        NotifyGroupEnd();
                        groupCount++;
                    }
                    else
                    {
                        assert (0);
                        //throw new RtfBraceNestingException( Strings.ToManyBraces );
                    }
                    break;

                default:
                    curText.unicodeStr()->append( ReadOneChar() ); // must still consume the 'peek'ed char
                    break;
            }
            if ( level == 0 && getIgnoreContentAfterRootGroup() )
            {
                break;
            }
            if ( peekCharValid )
            {
                nextChar = peekChar;
            }
            else
            {
                nextChar = PeekNextChar(); //reader, false );
                backslashAlreadyConsumed = false;
            }
        }
        FlushText();
        //reader.Close();

        if ( level > 0 )
        {
            assert (0);
            //throw new RtfBraceNestingException( Strings.ToFewBraces );
        }
        if ( groupCount == 0 )
        {
            assert (0);//throw new RtfEmptyDocumentException( Strings.NoRtfContent );
        }
        curText.unicodeStr()->remove();
    } // ParseRtf

    // ----------------------------------------------------------------------
    void RtfParser::ParseTag ()
    {
        ICUString tagName;
        ICUString tagValue;

        bool readingName = true;
        bool delimReached = false;

        UChar32 nextChar = PeekNextChar();
        while ( !delimReached )
        {
            if ( readingName && IsASCIILetter( nextChar ) )
            {
                tagName.unicodeStr()->append( ReadOneChar() ); // must still consume the 'peek'ed char
            }
            else if ( IsDigit( nextChar ) || (nextChar == '-' && tagValue.unicodeStr()->length() == 0) )
            {
                readingName = false;
                tagValue.unicodeStr()->append( ReadOneChar() ); // must still consume the 'peek'ed char
            }
            else
            {
                delimReached = true;
                RtfTag* newTag;

                QString qTagNameStr = convertUnicodeToQString (tagName);
                if ( tagValue.unicodeStr()->length() > 0 )
                {
                    QString qTagValueStr = convertUnicodeToQString (tagValue);
                    newTag = new RtfTag( qTagNameStr, qTagValueStr, this);
                }
                else
                {
                    newTag = new RtfTag( qTagNameStr, this );
                }
                bool skippedContent = HandleTag( newTag );
                if ( nextChar == ' ' && !skippedContent )
                {
                    ReadOneChar();// must still consume the 'peek'ed char
                }
            }
            if ( !delimReached )
            {
                nextChar = PeekNextChar();
            }
        }
    } // ParseTag

    // ----------------------------------------------------------------------
    bool RtfParser::HandleTag( RtfTag* tag )
    {
        if ( level == 0 )
        {
            assert (0);
            //throw new RtfStructureException( Strings.TagOnRootLevel( tag.ToString() ) );
        }

        if ( tagCount < 4 )
        {
            // this only handles the initial encoding tag in the header section
            UpdateEncoding( tag );
        }

        QString tagName = tag->Name();
        if (tagName == "leveltemplateid")
        {
            if (tag->ValueAsText() == "-1010900442")
            {
                char tmp;
                tmp = 1;
            }
        }
        // enable the font name detection in case the last tag was introducing
        // a theme font
        bool detectFontName = expectingThemeFont;
        if ( tagCountAtLastGroupStart == tagCount )
        {
            if (tagName == RtfSpec::TagThemeFontLoMajor ||
                    tagName == RtfSpec::TagThemeFontHiMajor ||
                    tagName == RtfSpec::TagThemeFontDbMajor ||
                    tagName == RtfSpec::TagThemeFontBiMajor ||
                    tagName == RtfSpec::TagThemeFontLoMinor ||
                    tagName == RtfSpec::TagThemeFontHiMinor ||
                    tagName == RtfSpec::TagThemeFontDbMinor ||
                    tagName == RtfSpec::TagThemeFontBiMinor)
            {
                // these introduce a new font, but the actual font tag will be
                // the second tag in the group, so we must remember this condition ...
                expectingThemeFont = true;
            }
            // always enable the font name detection also for the first tag in a group
            detectFontName = true;
        }

        if ( detectFontName )
        {
            // first tag in a group:
            if (tagName == RtfSpec::TagFont)
            {
                // in the font-table definition:
                // -> remember the target font for charset mapping
                targetFont = tag->FullName ();
                expectingThemeFont = false; // reset that state now
            }
            else if (tagName == RtfSpec::TagFontTable)
            {
                // -> remember we're in the font-table definition
                fontTableStartLevel = level;
            }
        }

        if ( !targetFont.isEmpty() )
        {
            // within a font-tables font definition: perform charset mapping
            if ( tagName == RtfSpec::TagFontCharset )
            {
                int charSet = tag->ValueAsNumber ();
                int codePage = RtfSpec::GetCodePage( charSet );
                fontToCodePageMapping[ targetFont ] = codePage;
                UpdateEncoding( codePage );
            }
        }
        if ( fontToCodePageMapping.size () > 0 && tagName == RtfSpec::TagFont )
        {
            QHash<QString, int>::const_iterator it = fontToCodePageMapping.find (tag->FullName());
            if (it != fontToCodePageMapping.end()) //found it
            {
                int codePage = fontToCodePageMapping[tag->FullName ()];
                UpdateEncoding( codePage );
            }
        }

        bool skippedContent = false;
        if (tagName == RtfSpec::TagUnicodeCode)
        {
            int unicodeValue = tag->ValueAsNumber ();
            UChar32 unicodeChar = (UChar)unicodeValue;
            curText.unicodeStr()->append( unicodeChar );
            // skip over the indicated number of 'alternative representation' text
            for ( int i = 0; i < unicodeSkipCount; i++ )
            {
                UChar32 nextChar = PeekNextChar();
                switch ( nextChar )
                {
                    case ' ':
                    case '\r':
                    case '\n':
                        ReadOneChar ();
                        skippedContent = true;
                        if ( i == 0 )
                        {
                            // the first whitespace after the tag
                            // -> only a delimiter, doesn't count for skipping ...
                            i--;
                        }
                        break;

                    case '\\':
                        {
                            ReadOneChar ();
                            skippedContent = true;
                            UChar32 secondChar = ReadOneChar ();

                            switch ( secondChar )
                            {
                                case '\'':
                                    // ok, this is a hex-encoded 'byte' -> need to consume both
                                    // hex digits too
                                    ReadOneChar (); // high nibble
                                    ReadOneChar (); // low nibble
                                    break;
                            }
                        }
                        break;

                    case '{':
                    case '}':
                        // don't consume peeked char and abort skipping
                        i = unicodeSkipCount;
                        break;

                    default:
                        {
                            ReadOneChar (); // consume peeked char
                            skippedContent = true;
                        }
                        break;
                }
            }
        }
        else if (tagName == RtfSpec::TagUnicodeSkipCount)
        {
            int newSkipCount = tag->ValueAsNumber();
            if ( newSkipCount < 0 || newSkipCount > 10 )
            {
                assert (0);
//                      throw new RtfUnicodeEncodingException( Strings.InvalidUnicodeSkipCount( tag.ToString() ) );
            }
            unicodeSkipCount = newSkipCount;
        }
        else
        {
            FlushText();
            NotifyTagFound( tag );
        }

        tagCount++;

        return skippedContent;
    } // HandleTag

    // ----------------------------------------------------------------------
    void RtfParser::UpdateEncoding( RtfTag* tag )
    {
        if (tag->Name() == RtfSpec::TagEncodingAnsi)
        {
            UpdateEncoding( RtfSpec::AnsiCodePage );
        }
        else if (tag->Name() == RtfSpec::TagEncodingMac)
        {
            UpdateEncoding( 10000 );
        }
        else if (tag->Name() == RtfSpec::TagEncodingPc)
        {
            UpdateEncoding( 437 );
        }
        else if (tag->Name() == RtfSpec::TagEncodingPca)
        {
            UpdateEncoding( 850 );
        }
        else if (tag->Name() == RtfSpec::TagEncodingAnsiCodePage)
        {
            UpdateEncoding( tag->ValueAsNumber() );
        }
    } // UpdateEncoding

    // ----------------------------------------------------------------------
    void RtfParser::UpdateEncoding( int codePage )
    {
        if ( codePage != encoding.CodePage() )
        {
            switch ( codePage )
            {
                case RtfSpec::AnsiCodePage:
                case RtfSpec::SymbolFakeCodePage: // hack to handle a windows legacy ...
                    encoding.SetEncoding (RtfSpec::AnsiCodePage);
                    break;

                default:
                    encoding.SetEncoding (codePage);
                    break;
            }
        }
    } // UpdateEncoding

    // ----------------------------------------------------------------------
    bool RtfParser::IsASCIILetter( UChar32 character )
    {
        //convert to QChar first and then test
        QChar qc (character);
        return (qc.isLetter());
    } // IsASCIILetter

    // ----------------------------------------------------------------------
    bool RtfParser::IsHexDigit( UChar32 character, char* hexChar )
    {
        QChar qc (character);
        char ascii = qc.toLatin1();

        *hexChar = ascii;
        return (ascii >= '0' && ascii <= '9') ||
               (ascii >= 'a' && ascii <= 'f') ||
               (ascii >= 'A' && ascii <= 'F');
    } // IsHexDigit

    // ----------------------------------------------------------------------
    bool RtfParser::IsDigit( UChar32 character )
    {
        QChar qc(character);
        return (qc.isDigit());
    } // IsDigit

    // ----------------------------------------------------------------------
#if 0 //UNIMPLEMENTED currently NOT USED
    private static int ReadOneByte( TextReader reader )
    {
        int byteValue = reader.Read();
        if ( byteValue == -1 )
        {
            throw new RtfUnicodeEncodingException( Strings.UnexpectedEndOfFile );
        }
        return byteValue;
    } // ReadOneByte
#endif
    // ----------------------------------------------------------------------
    UChar32 RtfParser::ReadOneChar()
    {
        UChar32 character = mUnicodeStr.unicodeStr()->char32At(mCurrCharIndex);
        mCurrCharIndex++;
        if (character == 0xFFFF)
        {
            assert (0);
        }
        /*
                    // NOTE: the handling of multi-byte encodings is probably not the most
                    // efficient here ...

                    bool completed = false;
                    int byteIndex = 0;
                    while ( !completed )
                    {
                        byteDecodingBuffer[ byteIndex ] = (byte)ReadOneByte( reader );
                        byteIndex++;
                        int usedBytes;
                        int usedChars;
                        byteToCharDecoder.Convert(
                            byteDecodingBuffer, 0, byteIndex,
                            charDecodingBuffer, 0, 1,
                            true,
                            out usedBytes,
                            out usedChars,
                            out completed );
                        if ( completed && ( usedBytes != byteIndex || usedChars != 1 ) )
                        {
                            throw new RtfMultiByteEncodingException( Strings.InvalidMultiByteEncoding(
                            byteDecodingBuffer, byteIndex, encoding ) );
                        }
                    }
                    char character = charDecodingBuffer[ 0 ];
        */
        return character;
    } // ReadOneChar

    // ----------------------------------------------------------------------
    void RtfParser::DecodeCurrentHexBuffer()
    {
        long pendingByteCount = hexDecodingBuffer.size();
        if ( pendingByteCount > 0 )
        {
            char* pendingBytes = hexDecodingBuffer.data();
            //byte[] pendingBytes = hexDecodingBuffer.ToArray();

            ICUString tmpStr;

            //use Qt to decode the buffer since it is more robust than doing it thru ICU
            //ICU failed when decoding russian eula that contained shift-JIS character encoding
            int codePage = encoding.CodePage();
            char* qtCodePageName = RtfSpec::GetCodePageName (codePage);
            QTextCodec* codec = QTextCodec::codecForName(qtCodePageName);
            QString decodeStr = codec->toUnicode (pendingBytes, pendingByteCount);

            //copy one char at a time since we don't have a good way of converting from QString to ICUString
            for (int i = 0; i < decodeStr.length(); i++)
            {
                UChar uniChar = (decodeStr.at(i)).unicode();
                tmpStr.unicodeStr()->append (uniChar);
            }
            curText.unicodeStr()->append (*(tmpStr.unicodeStr()));

            /*
                            size_t outSz;
                            std::vector<char> outBuf;

                            encoding.setBuffer (pendingBytes, pendingByteCount);

                            encoding.convertBuffer(1, //int verbose,
                                     &outSz,
                                     &outBuf);
            */

//                curText.Append( convertedChars, 0, usedChars );

            /*
                            int startIndex = 0;
                            bool completed = false;
            //              while ( !completed && startIndex < pendingBytes.Length )
                            while ( !completed && startIndex < pendingByteCount )
                            {


                                int usedBytes;
                                int usedChars;
                                byteToCharDecoder.Convert(
                                    pendingBytes, startIndex, pendingBytes.Length - startIndex,
                                    convertedChars, 0, convertedChars.Length,
                                    true,
                                    out usedBytes,
                                    out usedChars,
                                    out completed );
                                curText.Append( convertedChars, 0, usedChars );
                                startIndex += usedChars;
                            }
            */

            hexDecodingBuffer.clear();
        }
    } // DecodeCurrentHexBuffer

    // ----------------------------------------------------------------------
    UChar32 RtfParser::PeekNextChar() // bool mandatory )
    {
        UChar32 character = mUnicodeStr.unicodeStr()->char32At(mCurrCharIndex);
        if (character == 0xFFFF)
        {
            //throw new RtfMultiByteEncodingException( Strings.EndOfFileInvalidCharacter );
            assert (0);
        }
        return character;
    } // PeekNextChar

    // ----------------------------------------------------------------------
    void RtfParser::FlushText()
    {
        if ( curText.unicodeStr()->length() > 0 )
        {
            if ( level == 0 )
            {
                assert (0);
                //throw new RtfStructureException( Strings.TextOnRootLevel( curText.ToString() ) );
            }

            QString qCurTextStr = convertUnicodeToQString (curText);
            int currCodePage = encoding.CodePage();
            NotifyTextFound( new RtfText( qCurTextStr, currCodePage, this) );
            curText.unicodeStr()->remove();
        }
    } // FlushText


    void RtfParser::ResetCharIndex()
    {
        mCurrCharIndex = 0;
    }

    QString RtfParser::convertUnicodeToQString (ICUString& uniStr)
    {
#if 1
        int unilen = uniStr.unicodeStr()->length();
        QChar* qcharArray = (QChar*)malloc(unilen * sizeof(QChar));
        int qndx = 0;
        for (int i = 0; i < unilen; i++)
        {
            qcharArray [qndx] = QChar(uniStr.unicodeStr()->char32At (i));
            qndx++;
        }


#if 1
        //setting it directly from qcharArray allows the '\00 in unicode char to be preserved
        QString qStr (qcharArray, qndx);
        free (qcharArray);
#else
        //if we have the case where the unicode '\00 char exists, it considers it
        //a string truncator so we can't conver to utf8 and back
        //so this method below doesn't work


        QTextCodec* utf8codec = QTextCodec::codecForName("utf-8");
        QByteArray utf8Buf = utf8codec->fromUnicode (qcharArray, qndx);

        free (qcharArray);
        QString qStr(utf8Buf);
#endif
#else
        //a bit kludgy, but convert from unicode to utf8 string using ICU libs
        //then convert to Qstring from utf8 using Qt libs
        std::string utf8Str;

        uniStr.unicodeStr()->toUTF8String (utf8Str);
        int utf8len = utf8Str.length();

        QString qStr2;
        qStr2 = qStr2.fromUtf8(utf8Str.data());
#endif
        return qStr;
    }
    /*
            // ----------------------------------------------------------------------
            // members
            private int level;
            private int tagCountAtLastGroupStart;
            private int tagCount;
            private int fontTableStartLevel;
            private string targetFont;
            private bool expectingThemeFont;
            private readonly Hashtable fontToCodePageMapping = new Hashtable();
            private Encoding encoding;
            private Decoder byteToCharDecoder;
            private readonly MemoryStream hexDecodingBuffer = new MemoryStream();
            private readonly byte[] byteDecodingBuffer = new byte[ 8 ]; // >0 for multi-byte encodings
            private readonly char[] charDecodingBuffer = new char[ 1 ];
    */
} // class RtfParser

//} // namespace Itenso.Rtf.Parser
// -- EOF -------------------------------------------------------------------
