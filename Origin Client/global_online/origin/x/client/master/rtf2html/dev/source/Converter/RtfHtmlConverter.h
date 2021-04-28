// -- FILE ------------------------------------------------------------------
// name       : RtfHtmlConverter.cs
// project    : RTF Framelet
// created    : Jani Giannoudis - 2008.06.02
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using System.IO;
//using System.Text.RegularExpressions;
//using System.Web;
//using System.Web.UI;
//using System.Drawing;
//using System.Globalization;
//using Itenso.Sys.Logging;
//using Itenso.Rtf.Support;
//using Itenso.Rtf.Converter.Image;

#ifndef __RTFHTMLCONVERTER_H
#define __RTFHTMLCONVERTER_H

#include <qstring.h>

#include "RtfDocument.h"
#include "RtfVisualVisitorBase.h"
#include "RtfHtmlConvertSettings.h"
#include "RtfVisual.h"
#include "RtfHtmlElementPath.h"
#include "RtfHtmlStyle.h"
#include "HtmlTextWriter.h"
#include "RtfVisualSpecialChar.h"
#include "RtfHtmlStyleConverter.h"


namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfHtmlConverter : public RtfVisualVisitorBase
    {
        public:

            static const QString DefaultHtmlFileExtension;

            // ----------------------------------------------------------------------
            RtfHtmlConverter( RtfDocument* rtfDocument, RtfHtmlConvertSettings* settings, QObject* parent = 0 );
            virtual ~RtfHtmlConverter();

            // ----------------------------------------------------------------------
            RtfDocument* getRtfDocument ();

            // ----------------------------------------------------------------------
            RtfHtmlConvertSettings* Settings ();

#if 0   //UNIMPLEMENTED, currently NOT USED
            // ----------------------------------------------------------------------
            public IRtfHtmlStyleConverter StyleConverter
#endif
            // ----------------------------------------------------------------------
            QHash<RtfVisualSpecialCharKind::RtfVisualSpecialCharKind, QString>& SpecialCharacters ();

#if 0 //UNIMPLEMENTED, currently NOT USED
            // ----------------------------------------------------------------------
            RtfConvertedImageInfoCollection DocumentImages
#endif
            // ----------------------------------------------------------------------
            HtmlTextWriter* Writer ();

            // ----------------------------------------------------------------------
            RtfHtmlElementPath* ElementPath ();

            // ----------------------------------------------------------------------
            bool IsInParagraph ();

            // ----------------------------------------------------------------------
            bool IsInList ();

            // ----------------------------------------------------------------------
            bool IsInListItem ();

            // ----------------------------------------------------------------------
            QString& Generator ();

            // ----------------------------------------------------------------------
            QString& Convert();
            // ----------------------------------------------------------------------
            bool IsCurrentElement( HtmlTextWriterTag::Tag tag );

            // ----------------------------------------------------------------------
            bool IsInElement( HtmlTextWriterTag::Tag tag );

            //#region TagRendering

            // ----------------------------------------------------------------------
            void RenderBeginTag( HtmlTextWriterTag::Tag tag );

            // ----------------------------------------------------------------------
            void RenderEndTag();

            // ----------------------------------------------------------------------
            void RenderEndTag( bool lineBreak );

            // ----------------------------------------------------------------------
            void RenderTitleTag();

            // ----------------------------------------------------------------------
            void RenderMetaTag();

            // ----------------------------------------------------------------------
            void RenderHtmlTag();
            // ----------------------------------------------------------------------
            void RenderLinkTag();

            // ----------------------------------------------------------------------
            void RenderHeadTag();

            // ----------------------------------------------------------------------
            void RenderBodyTag();

            // ----------------------------------------------------------------------
            void RenderLineBreak();

            // ----------------------------------------------------------------------
            void RenderATag();
            // ----------------------------------------------------------------------
            void RenderPTag();

            // ----------------------------------------------------------------------
            void RenderBTag();

            // ----------------------------------------------------------------------
            void RenderITag();

            // ----------------------------------------------------------------------
            void RenderUTag();

            // ----------------------------------------------------------------------
            void RenderSTag();
            // ----------------------------------------------------------------------
            void RenderSubTag();

            // ----------------------------------------------------------------------
            void RenderSupTag();

            // ----------------------------------------------------------------------
            void RenderSpanTag();

            // ----------------------------------------------------------------------
            void RenderUlTag();

            // ----------------------------------------------------------------------
            void RenderOlTag();
            // ----------------------------------------------------------------------
            void RenderLiTag();

            // ----------------------------------------------------------------------
            void RenderImgTag();

            // ----------------------------------------------------------------------
            void RenderStyleTag();

            //#endregion // TagRendering

            //#region HtmlStructure

            // ----------------------------------------------------------------------
            void RenderDocumentHeader();
            // ----------------------------------------------------------------------
            void RenderMetaContentType();
            // ----------------------------------------------------------------------
            void RenderMetaGenerator();
            // ----------------------------------------------------------------------
            void RenderLinkStyleSheets();
            // ----------------------------------------------------------------------
            void RenderHeadAttributes();
            // ----------------------------------------------------------------------
            void RenderTitle();

            // ----------------------------------------------------------------------
            void RenderStyles();

            // ----------------------------------------------------------------------
            void RenderRtfContent();

            // ----------------------------------------------------------------------
            void BeginParagraph();
            // ----------------------------------------------------------------------
            void EndParagraph();

            // ----------------------------------------------------------------------
            bool OnEnterVisual( RtfVisual* rtfVisual );

            // ----------------------------------------------------------------------
            void OnLeaveVisual( RtfVisual* rtfVisual );

            //#endregion // HtmlStructure

            //#region HtmlFormat

            // ----------------------------------------------------------------------
            RtfHtmlStyle* GetHtmlStyle( RtfVisual* rtfVisual );

            // ----------------------------------------------------------------------
            QString FormatHtmlText( QString& rtfText );

            //#endregion // HtmlFormat

            //#region RtfVisuals

            // ----------------------------------------------------------------------
            virtual void DoVisitText( RtfVisualText* visualText );

#if 0 //UNIMPLEMENTED, currently NOT USED
            // ----------------------------------------------------------------------
            void DoVisitImage( IRtfVisualImage visualImage )
#endif

            // ----------------------------------------------------------------------
            void DoVisitSpecial( RtfVisualSpecialChar* visualSpecialChar );

            // ----------------------------------------------------------------------
            void DoVisitBreak( RtfVisualBreak* visualBreak );

            //#endregion // RtfVisuals

        private:

            // ----------------------------------------------------------------------
            QString ConvertVisualHyperlink( QString& text );

            // ----------------------------------------------------------------------
            void RenderDocumentSection();

            // ----------------------------------------------------------------------
            void RenderHtmlSection();

            // ----------------------------------------------------------------------
            void RenderHeadSection();

            // ----------------------------------------------------------------------
            void RenderBodySection();

            // ----------------------------------------------------------------------
            bool EnterVisual( RtfVisual* rtfVisual );

            // ----------------------------------------------------------------------
            void LeaveVisual( RtfVisual* rtfVisual );

            // ----------------------------------------------------------------------
            // returns true if visual is in list
            bool EnsureOpenList( RtfVisual* rtfVisual );

            // ----------------------------------------------------------------------
            void EnsureClosedList();
            // ----------------------------------------------------------------------
            void EnsureClosedList( RtfVisual* rtfVisual );

            // ----------------------------------------------------------------------
            // members
//UNUSED
//          RtfConvertedImageInfoCollection documentImages = new RtfConvertedImageInfoCollection();
            RtfHtmlElementPath elementPath;
            RtfDocument* rtfDocument;
            RtfHtmlConvertSettings* settings;
            RtfHtmlStyleConverter styleConverter;
            QHash<RtfVisualSpecialCharKind::RtfVisualSpecialCharKind, QString> specialCharacters;
            HtmlTextWriter writer;
            RtfVisual* lastVisual;

            bool isInParagraphNumber;
            QString generatorName;
            QString nonBreakingSpace;
            QString unsortedListValue;

            QString html;
//unused
//      private Regex hyperlinkRegEx;

//      private static readonly ILogger logger = Logger.GetLogger( typeof( RtfHtmlConverter ) );

    }; // class RtfHtmlConverter

} // namespace Itenso.Rtf.Converter.Html
// -- EOF -------------------------------------------------------------------
#endif //__RTFHTMLCONVERTER_H