// -- FILE ------------------------------------------------------------------
// name       : RtfSpec.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.20
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System.Text;

#ifndef RTFSPEC_H
#define RTFSPEC_H
#include <qstring.h>

namespace RTF2HTML
{
    namespace RtfSpec
    {
        // --- rtf general ----
        const QString TagRtf = "rtf";
        const int RtfVersion1 = 1;

        const QString TagGenerator = "generator";
        const QString TagViewKind = "viewkind";

        // --- encoding ----
        const QString TagEncodingAnsi = "ansi";
        const QString TagEncodingMac = "mac";
        const QString TagEncodingPc = "pc";
        const QString TagEncodingPca = "pca";
        const QString TagEncodingAnsiCodePage = "ansicpg";
        const int AnsiCodePage = 1252;
        const int SymbolFakeCodePage = 42; // a windows legacy hack ...

//      public static readonly Encoding AnsiEncoding = Encoding.GetEncoding( AnsiCodePage );

        const QString TagUnicodeSkipCount = "uc";
        const QString TagUnicodeCode = "u";
        const QString TagUnicodeAlternativeChoices = "upr";
        const QString TagUnicodeAlternativeUnicode = "ud";

        // --- font ----
        const QString TagFontTable = "fonttbl";
        const QString TagDefaultFont = "deff";
        const QString TagFont = "f";
        const QString TagFontKindNil = "fnil";
        const QString TagFontKindRoman = "froman";
        const QString TagFontKindSwiss = "fswiss";
        const QString TagFontKindModern = "fmodern";
        const QString TagFontKindScript = "fscript";
        const QString TagFontKindDecor = "fdecor";
        const QString TagFontKindTech = "ftech";
        const QString TagFontKindBidi = "fbidi";
        const QString TagFontCharset = "fcharset";
        const QString TagFontPitch = "fprq";
        const QString TagFontSize = "fs";
        const QString TagFontDown = "dn";
        const QString TagFontUp = "up";
        const QString TagFontSubscript = "sub";
        const QString TagFontSuperscript = "super";
        const QString TagFontNoSuperSub = "nosupersub";

        const QString TagThemeFontLoMajor = "flomajor"; // these are 'theme' fonts
        const QString TagThemeFontHiMajor = "fhimajor"; // used in new font tables
        const QString TagThemeFontDbMajor = "fdbmajor";
        const QString TagThemeFontBiMajor = "fbimajor";
        const QString TagThemeFontLoMinor = "flominor";
        const QString TagThemeFontHiMinor = "fhiminor";
        const QString TagThemeFontDbMinor = "fdbminor";
        const QString TagThemeFontBiMinor = "fbiminor";

        const int DefaultFontSize = 24;

        const QString TagCodePage = "cpg";

        // --- color ----
        const QString TagColorTable = "colortbl";
        const QString TagColorRed = "red";
        const QString TagColorGreen = "green";
        const QString TagColorBlue = "blue";
        const QString TagColorForeground = "cf";
        const QString TagColorBackground = "cb";
        const QString TagColorBackgroundWord = "chcbpat";
        const QString TagColorHighlight = "highlight";

        // --- header/footer ----
        const QString TagHeader = "header";
        const QString TagHeaderFirst = "headerf";
        const QString TagHeaderLeft = "headerl";
        const QString TagHeaderRight = "headerr";
        const QString TagFooter = "footer";
        const QString TagFooterFirst = "footerf";
        const QString TagFooterLeft = "footerl";
        const QString TagFooterRight = "footerr";
        const QString TagFootnote = "footnote";

        // --- character ----
        const QString TagDelimiter = ";";
        const QString TagExtensionDestination = "*";
        const QString TagTilde = "~";
        const QString TagHyphen = "-";
        const QString TagUnderscore = "_";

        // --- special character ----
        const QString TagPage = "page";
        const QString TagSection = "sect";
        const QString TagParagraph = "par";
        const QString TagLine = "line";
        const QString TagTabulator = "tab";
        const QString TagEmDash = "emdash";
        const QString TagEnDash = "endash";
        const QString TagEmSpace = "emspace";
        const QString TagEnSpace = "enspace";
        const QString TagQmSpace = "qmspace";
        const QString TagBulltet = "bullet";
        const QString TagLeftSingleQuote = "lquote";
        const QString TagRightSingleQuote = "rquote";
        const QString TagLeftDoubleQuote = "ldblquote";
        const QString TagRightDoubleQuote = "rdblquote";

        // --- format ----
        const QString TagPlain = "plain";
        const QString TagParagraphDefaults = "pard";
        const QString TagSectionDefaults = "sectd";

        const QString TagBold = "b";
        const QString TagItalic = "i";
        const QString TagUnderLine = "ul";
        const QString TagUnderLineNone = "ulnone";
        const QString TagStrikeThrough = "strike";
        const QString TagHidden = "v";
        const QString TagAlignLeft = "ql";
        const QString TagAlignCenter = "qc";
        const QString TagAlignRight = "qr";
        const QString TagAlignJustify = "qj";

        const QString TagStyleSheet = "stylesheet";

        // --- info ----
        const QString TagInfo = "info";
        const QString TagInfoVersion = "version";
        const QString TagInfoRevision = "vern";
        const QString TagInfoNumberOfPages = "nofpages";
        const QString TagInfoNumberOfWords = "nofwords";
        const QString TagInfoNumberOfChars = "nofchars";
        const QString TagInfoId = "id";
        const QString TagInfoTitle = "title";
        const QString TagInfoSubject = "subject";
        const QString TagInfoAuthor = "author";
        const QString TagInfoManager = "manager";
        const QString TagInfoCompany = "company";
        const QString TagInfoOperator = "operator";
        const QString TagInfoCategory = "category";
        const QString TagInfoKeywords = "keywords";
        const QString TagInfoComment = "comment";
        const QString TagInfoDocumentComment = "doccomm";
        const QString TagInfoHyperLinkBase = "hlinkbase";
        const QString TagInfoCreationTime = "creatim";
        const QString TagInfoRevisionTime = "revtim";
        const QString TagInfoPrintTime = "printim";
        const QString TagInfoBackupTime = "buptim";
        const QString TagInfoYear = "yr";
        const QString TagInfoMonth = "mo";
        const QString TagInfoDay = "dy";
        const QString TagInfoHour = "hr";
        const QString TagInfoMinute = "min";
        const QString TagInfoSecond = "sec";
        const QString TagInfoEditingTimeMinutes = "edmins";

        // --- user properties ----
        const QString TagUserProperties = "userprops";
        const QString TagUserPropertyType = "proptype";
        const QString TagUserPropertyName = "propname";
        const QString TagUserPropertyValue = "staticval";
        const QString TagUserPropertyLink = "linkval";

        // this table is from the RTF specification 1.9.1, page 40
        const int PropertyTypeInteger = 3;
        const int PropertyTypeRealNumber = 5;
        const int PropertyTypeDate = 64;
        const int PropertyTypeBoolean = 11;
        const int PropertyTypeText = 30;

        // --- picture ----
        const QString TagPicture = "pict";
        const QString TagPictureWrapper = "shppict";
        const QString TagPictureWrapperAlternative = "nonshppict";
        const QString TagPictureFormatEmf = "emfblip";
        const QString TagPictureFormatPng = "pngblip";
        const QString TagPictureFormatJpg = "jpegblip";
        const QString TagPictureFormatPict = "macpict";
        const QString TagPictureFormatOs2Metafile = "pmmetafile";
        const QString TagPictureFormatWmf = "wmetafile";
        const QString TagPictureFormatWinDib = "dibitmap";
        const QString TagPictureFormatWinBmp = "wbitmap";
        const QString TagPictureWidth = "picw";
        const QString TagPictureHeight = "pich";
        const QString TagPictureWidthGoal = "picwgoal";
        const QString TagPictureHeightGoal = "pichgoal";
        const QString TagPictureWidthScale = "picscalex";
        const QString TagPictureHeightScale = "picscaley";

        // --- bullets/numbering ----
        const QString TagParagraphNumberText = "pntext";
        const QString TagListNumberText = "listtext";

        //symbols
        const QString TagBar = "|";
        const QString TagColon = ":";

        // ----------------------------------------------------------------------
        int GetCodePage( int charSet );

        char* GetCodePageName (int codePage);

    } // class RtfSpec

} // namespace Itenso.Rtf
#endif //RTFSPEC_H
// -- EOF -------------------------------------------------------------------
