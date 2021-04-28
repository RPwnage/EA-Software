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

#include <assert.h>
#include <qurl.h>
#include "RtfHtmlConverter.h"
#include "RtfVisualText.h"
#include "RtfVisualBreak.h"

namespace RTF2HTML
{

    // ----------------------------------------------------------------------
    const QString RtfHtmlConverter::DefaultHtmlFileExtension = ".html";

    // ----------------------------------------------------------------------
    RtfHtmlConverter::RtfHtmlConverter( RtfDocument* rtfDocument, RtfHtmlConvertSettings* settings, QObject* parent )
        : RtfVisualVisitorBase (parent)
    {
        if ( rtfDocument == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "rtfDocument" );
        }
        if ( settings == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "settings" );
        }

        this->rtfDocument = rtfDocument;
        this->settings = settings;

        isInParagraphNumber = false;

        generatorName = "Rtf2Html Converter";
        nonBreakingSpace = "&nbsp;";
        unsortedListValue = "·";
    } // RtfHtmlConverter

    RtfHtmlConverter::~RtfHtmlConverter()
    {
    }

    // ----------------------------------------------------------------------
    RtfDocument* RtfHtmlConverter::getRtfDocument()
    {
        return rtfDocument;
    } // RtfDocument

    // ----------------------------------------------------------------------
    RtfHtmlConvertSettings* RtfHtmlConverter::Settings()
    {
        return settings;
    } // Settings

    // ----------------------------------------------------------------------
#if 0 //UNIMPLEMENTED, currently NOT USED
    RtfHtmlStyleConverter StyleConverter
    {
        get { return styleConverter; }
        set
        {
            if ( value == null )
            {
                throw new ArgumentNullException( "value" );
            }
            styleConverter = value;
        }
    } // StyleConverter
#endif
    // ----------------------------------------------------------------------
    QHash<RtfVisualSpecialCharKind::RtfVisualSpecialCharKind, QString>& RtfHtmlConverter::SpecialCharacters ()
    {
        return specialCharacters;
    } // SpecialCharacters

    // ----------------------------------------------------------------------
#if 0 //UNIMPLEMENTED, currently NOT USED
    public RtfConvertedImageInfoCollection DocumentImages
    {
        get { return documentImages; }
    } // DocumentImages
#endif
    // ----------------------------------------------------------------------
    HtmlTextWriter* RtfHtmlConverter::Writer()
    {
        return &writer;
    } // Writer

    // ----------------------------------------------------------------------
    RtfHtmlElementPath* RtfHtmlConverter::ElementPath ()
    {
        return &elementPath;
    } // ElementPath

    // ----------------------------------------------------------------------
    bool RtfHtmlConverter::IsInParagraph ()
    {
        return IsInElement( HtmlTextWriterTag::P );
    } // IsInParagraph

    // ----------------------------------------------------------------------
    bool RtfHtmlConverter::IsInList ()
    {
        return IsInElement( HtmlTextWriterTag::Ul ) || IsInElement( HtmlTextWriterTag::Ol );
    } // IsInList

    // ----------------------------------------------------------------------
    bool RtfHtmlConverter::IsInListItem ()
    {
        return IsInElement( HtmlTextWriterTag::Li );
    } // IsInListItem

    // ----------------------------------------------------------------------
    QString& RtfHtmlConverter::Generator ()
    {
        return generatorName;
    } // Generator

    // ----------------------------------------------------------------------
    QString& RtfHtmlConverter::Convert()
    {
        QString html;
        //documentImages.Clear();

        //using ( StringWriter stringWriter = new StringWriter() )
        {
            //  using ( writer = new HtmlTextWriter( stringWriter ) )
            {
                RenderDocumentSection();
                RenderHtmlSection();
            }
            //html = stringWriter.ToString();
            //html = writer.toString();
        }

        if ( elementPath.Count() != 0 )
        {
            assert (0);
            //logger.Error( "unbalanced element structure" );
        }

        return writer.toString();
    } // Convert

    // ----------------------------------------------------------------------
    bool RtfHtmlConverter::IsCurrentElement( HtmlTextWriterTag::Tag tag )
    {
        return elementPath.IsCurrent( tag );
    } // IsCurrentElement

    // ----------------------------------------------------------------------
    bool RtfHtmlConverter::IsInElement( HtmlTextWriterTag::Tag tag )
    {
        return elementPath.Contains( tag );
    } // IsInElement

//  #region TagRendering

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderBeginTag( HtmlTextWriterTag::Tag tag )
    {
        writer.RenderBeginTag( tag );
        elementPath.Push( tag );
    } // RenderBeginTag

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderEndTag()
    {
        RenderEndTag( false );
    } // RenderEndTag

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderEndTag( bool lineBreak )
    {
        writer.RenderEndTag();
        if ( lineBreak )
        {
            writer.WriteLine();
        }
        elementPath.Pop();
    } // RenderEndTag

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderTitleTag()
    {
        RenderBeginTag( HtmlTextWriterTag::Title );
    } // RenderTitleTag

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderMetaTag()
    {
        RenderBeginTag( HtmlTextWriterTag::Meta );
    } // RenderMetaTag

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderHtmlTag()
    {
        RenderBeginTag( HtmlTextWriterTag::Html );
    } // RenderHtmlTag

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderLinkTag()
    {
        RenderBeginTag( HtmlTextWriterTag::Link );
    } // RenderLinkTag

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderHeadTag()
    {
        RenderBeginTag( HtmlTextWriterTag::Head );
    } // RenderHeadTag

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderBodyTag()
    {
        RenderBeginTag( HtmlTextWriterTag::Body );
    } // RenderBodyTag

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderLineBreak()
    {
        writer.WriteBreak();
        writer.WriteLine();
    } // RenderLineBreak

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderATag()
    {
        RenderBeginTag( HtmlTextWriterTag::A );
    } // RenderATag

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderPTag()
    {
        RenderBeginTag( HtmlTextWriterTag::P );
    } // RenderPTag

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderBTag()
    {
        RenderBeginTag( HtmlTextWriterTag::B );
    } // RenderBTag

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderITag()
    {
        RenderBeginTag( HtmlTextWriterTag::I );
    } // RenderITag

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderUTag()
    {
        RenderBeginTag( HtmlTextWriterTag::U );
    } // RenderUTag

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderSTag()
    {
        RenderBeginTag( HtmlTextWriterTag::S );
    } // RenderSTag

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderSubTag()
    {
        RenderBeginTag( HtmlTextWriterTag::Sub );
    } // RenderSubTag

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderSupTag()
    {
        RenderBeginTag( HtmlTextWriterTag::Sup );
    } // RenderSupTag

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderSpanTag()
    {
        RenderBeginTag( HtmlTextWriterTag::Span );
    } // RenderSpanTag

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderUlTag()
    {
        RenderBeginTag( HtmlTextWriterTag::Ul );
    } // RenderUlTag

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderOlTag()
    {
        RenderBeginTag( HtmlTextWriterTag::Ol );
    } // RenderOlTag

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderLiTag()
    {
        RenderBeginTag( HtmlTextWriterTag::Li );
    } // RenderLiTag

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderImgTag()
    {
        RenderBeginTag( HtmlTextWriterTag::Img );
    } // RenderImgTag

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderStyleTag()
    {
        RenderBeginTag( HtmlTextWriterTag::Style );
    } // RenderStyleTag

    //#endregion // TagRendering

    //#region HtmlStructure

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderDocumentHeader()
    {
        if ( settings->DocumentHeader().isEmpty() )
        {
            return;
        }

        writer.WriteLine( settings->DocumentHeader() );
    } // RenderDocumentHeader

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderMetaContentType()
    {
        writer.AddAttribute( "http-equiv", "content-type" );

        QString content = "text/html";
        if ( !settings->CharacterSet().isEmpty() )
        {
            content += "; charset=" + settings->CharacterSet();
        }
        writer.AddAttribute( HtmlTextWriterAttribute::Content, content );
        RenderMetaTag();
        RenderEndTag();
    } // RenderMetaContentType

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderMetaGenerator()
    {
        QString generator = Generator();
        if ( generator.isEmpty() )
        {
            return;
        }

        writer.WriteLine();
        writer.AddAttribute( HtmlTextWriterAttribute::Name, "generator" );
        writer.AddAttribute( HtmlTextWriterAttribute::Content, generator );
        RenderMetaTag();
        RenderEndTag();
    } // RenderMetaGenerator

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderLinkStyleSheets()
    {
        return;
#if 0 //UNIMPLEMENTED, currently NOT USED
        if ( !settings.HasStyleSheetLinks )
        {
            return;
        }

        foreach ( string styleSheetLink in settings.StyleSheetLinks )
        {
            if ( string.IsNullOrEmpty( styleSheetLink ) )
            {
                continue;
            }

            Writer.WriteLine();
            Writer.AddAttribute( HtmlTextWriterAttribute.Href, styleSheetLink );
            Writer.AddAttribute( HtmlTextWriterAttribute.Type, "text/css" );
            Writer.AddAttribute( HtmlTextWriterAttribute.Rel, "stylesheet" );
            RenderLinkTag();
            RenderEndTag();
        }
#endif
    } // RenderLinkStyleSheets

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderHeadAttributes()
    {
        RenderMetaContentType();
        RenderMetaGenerator();
        RenderLinkStyleSheets();
    } // RenderHeadAttributes

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderTitle()
    {
        if ( settings->getTitle().isEmpty() )
        {
            return;
        }

        writer.WriteLine();
        RenderTitleTag();
        writer.Write( settings->getTitle() );
        RenderEndTag();
    } // RenderTitle

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderStyles()
    {
        if ( !settings->HasStyles() )
        {
            return;
        }

        assert (0);
#if 0 //UNIMPLEMENTED, currently NOT USED
        Writer.WriteLine();
        RenderStyleTag();

        bool firstStyle = true;
        foreach ( IRtfHtmlCssStyle cssStyle in settings.Styles )
        {
            if ( cssStyle.Properties.Count == 0 )
            {
                continue;
            }

            if ( !firstStyle )
            {
                Writer.WriteLine();
            }
            Writer.WriteLine( cssStyle.SelectorName );
            Writer.WriteLine( "{" );
            for ( int i = 0; i < cssStyle.Properties.Count; i++ )
            {
                Writer.WriteLine( string.Format(
                                      CultureInfo.InvariantCulture,
                                      "  {0}: {1};",
                                      cssStyle.Properties.Keys[ i ],
                                      cssStyle.Properties[ i ] ) );
            }
            Writer.Write( "}" );
            firstStyle = false;
        }

        RenderEndTag();
#endif
    } // RenderStyles

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderRtfContent()
    {
        RtfVisualCollection* vc = rtfDocument->VisualContent();
        int numVisuals = vc->Count();

        for (int i = 0; i < numVisuals; i++)
        {
            RtfVisual* visual = vc->Get (i);
            visual->Visit( this );
        }
        EnsureClosedList();
    } // RenderRtfContent

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::BeginParagraph()
    {
        if ( IsInParagraph () )
        {
            return;
        }
        RenderPTag();
    } // BeginParagraph

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::EndParagraph()
    {
        if ( !IsInParagraph () )
        {
            return;
        }
        RenderEndTag( true );
    } // EndParagraph

    // ----------------------------------------------------------------------
    bool RtfHtmlConverter::OnEnterVisual( RtfVisual* rtfVisual )
    {
        Q_UNUSED (rtfVisual);
        return true;
    } // OnEnterVisual

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::OnLeaveVisual( RtfVisual* rtfVisual )
    {
        Q_UNUSED (rtfVisual);
    } // OnLeaveVisual

    //#endregion // HtmlStructure

    //#region HtmlFormat

    // ----------------------------------------------------------------------
    RtfHtmlStyle* RtfHtmlConverter::GetHtmlStyle( RtfVisual* rtfVisual )
    {
        RtfHtmlStyle* htmlStyle = NULL;

        switch ( rtfVisual->Kind() )
        {
            case RtfVisualKind::Text:
                {
                    htmlStyle = styleConverter.TextToHtml( (RtfVisualText*)rtfVisual );
                }
                break;
        }

        return htmlStyle;
    } // GetHtmlStyle

    // ----------------------------------------------------------------------
    QString RtfHtmlConverter::FormatHtmlText( QString& rtfText )
    {
        QString htmlText = rtfText.toHtmlEscaped();

        // replace all spaces to non-breaking spaces
        if ( settings->UseNonBreakingSpaces() )
        {
            htmlText = htmlText.replace( " ", nonBreakingSpace );
        }

        return htmlText;
    } // FormatHtmlText

    //#endregion // HtmlFormat

    //#region RtfVisuals

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::DoVisitText( RtfVisualText* visualText )
    {
        if ( !EnterVisual( visualText ) )
        {
            return;
        }

        // suppress hidden text
        if ( visualText->getFormat()->IsHidden() && settings->IsShowHiddenText() == false )
        {
            return;
        }

        RtfTextFormat* textFormat = visualText->getFormat();
        switch ( textFormat->Alignment() )
        {
            case RtfTextAlignment::Left:
                //Writer.AddStyleAttribute( HtmlTextWriterStyle::TextAlign, "left" );
                break;
            case RtfTextAlignment::Center:
                writer.AddStyleAttribute( HtmlTextWriterStyle::TextAlign, "center" );
                break;
            case RtfTextAlignment::Right:
                writer.AddStyleAttribute( HtmlTextWriterStyle::TextAlign, "right" );
                break;
            case RtfTextAlignment::Justify:
                writer.AddStyleAttribute( HtmlTextWriterStyle::TextAlign, "justify" );
                break;
        }

        if ( !IsInListItem() )
        {
            BeginParagraph();
        }

        // format tags
        if ( textFormat->IsBold () )
        {
            RenderBTag();
        }
        if ( textFormat->IsItalic () )
        {
            RenderITag();
        }
        if ( textFormat->IsUnderline () )
        {
            RenderUTag();
        }
        if ( textFormat->IsStrikeThrough ())
        {
            RenderSTag();
        }

        // span with style
        RtfHtmlStyle* htmlStyle = GetHtmlStyle( visualText ); //"new"s htmlStyle

        if ( !htmlStyle->IsEmpty() )
        {
            if ( !htmlStyle->getForegroundColor().isEmpty() )
            {
                writer.AddStyleAttribute( HtmlTextWriterStyle::Color, htmlStyle->getForegroundColor() );
            }
            if ( (settings != NULL) && !settings->ignoreBackgroundColor() && !htmlStyle->getBackgroundColor().isEmpty() )
            {
                writer.AddStyleAttribute( HtmlTextWriterStyle::BackgroundColor, htmlStyle->getBackgroundColor() );
            }
            if (  !htmlStyle->getFontFamily().isEmpty() )
            {
                writer.AddStyleAttribute( HtmlTextWriterStyle::FontFamily, htmlStyle->getFontFamily() );
            }
            if ( !htmlStyle->getFontSize().isEmpty() )
            {
                writer.AddStyleAttribute( HtmlTextWriterStyle::FontSize, htmlStyle->getFontSize() );
            }

            RenderSpanTag();
        }

        // visual hyperlink
        bool isHyperlink = false;
//      if ( 1) //settings->ConvertVisualHyperlinks () )
        {
            QString link = visualText->getText();
            QString href = ConvertVisualHyperlink( link );
            if ( !href.isEmpty() )
            {
                isHyperlink = true;
                writer.AddAttribute( HtmlTextWriterAttribute::Href, href );
                RenderATag();
            }
        }

        // subscript and superscript
        if ( textFormat->SuperScript() < 0 )
        {
            RenderSubTag();
        }
        else if ( textFormat->SuperScript() > 0 )
        {
            RenderSupTag();
        }

        QString text = visualText->getText();
        if (isHyperlink)
        {
            //text is combination of HYPERLINK "http://www.origin.com | www.origin.com
            //need to extract out the visual portion of text
            int index = text.indexOf("|");
            if (index > 0)
            {
                QStringList hyperlinkArray = text.split('|');
                text = hyperlinkArray [1];
                text = text.trimmed();
            }
        }

        QString htmlText = FormatHtmlText( text );
        writer.Write( htmlText );

        // subscript and superscript
        if ( textFormat->SuperScript() < 0 )
        {
            RenderEndTag(); // sub
        }
        else if ( textFormat->SuperScript() > 0 )
        {
            RenderEndTag(); // sup
        }

        // visual hyperlink
        if ( isHyperlink )
        {
            RenderEndTag(); // a
        }

        // span with style
        if ( !htmlStyle->IsEmpty() )
        {
            RenderEndTag();
        }

        // format tags
        if ( textFormat->IsStrikeThrough() )
        {
            RenderEndTag(); // s
        }
        if ( textFormat->IsUnderline() )
        {
            RenderEndTag(); // u
        }
        if ( textFormat->IsItalic() )
        {
            RenderEndTag(); // i
        }
        if ( textFormat->IsBold() )
        {
            RenderEndTag(); // b
        }

        LeaveVisual( visualText );
    } // DoVisitText

    // ----------------------------------------------------------------------
#if 0 //UNIMPLEMENTED, currently NOT USED
    override void DoVisitImage( IRtfVisualImage visualImage )
    {
        if ( !EnterVisual( visualImage ) )
        {
            return;
        }

        switch ( visualImage.Alignment )
        {
            case RtfTextAlignment.Left:
                //Writer.AddStyleAttribute( HtmlTextWriterStyle.TextAlign, "left" );
                break;
            case RtfTextAlignment.Center:
                Writer.AddStyleAttribute( HtmlTextWriterStyle.TextAlign, "center" );
                break;
            case RtfTextAlignment.Right:
                Writer.AddStyleAttribute( HtmlTextWriterStyle.TextAlign, "right" );
                break;
            case RtfTextAlignment.Justify:
                Writer.AddStyleAttribute( HtmlTextWriterStyle.TextAlign, "justify" );
                break;
        }

        BeginParagraph();

        int imageIndex = documentImages.Count + 1;
        string fileName = settings.GetImageUrl( imageIndex, visualImage.Format );
        int width = settings.ImageAdapter.CalcImageWidth( visualImage.Format, visualImage.Width,
                    visualImage.DesiredWidth, visualImage.ScaleWidthPercent );
        int height = settings.ImageAdapter.CalcImageHeight( visualImage.Format, visualImage.Height,
                     visualImage.DesiredHeight, visualImage.ScaleHeightPercent );

        Writer.AddAttribute( HtmlTextWriterAttribute.Width, width.ToString( CultureInfo.InvariantCulture ) );
        Writer.AddAttribute( HtmlTextWriterAttribute.Height, height.ToString( CultureInfo.InvariantCulture ) );
        string htmlFileName = HttpUtility.HtmlEncode( fileName );
        Writer.AddAttribute( HtmlTextWriterAttribute.Src, htmlFileName, false );
        RenderImgTag();
        RenderEndTag();

        documentImages.Add( new RtfConvertedImageInfo(
                                htmlFileName,
                                settings.ImageAdapter.TargetFormat,
                                new Size( width, height ) ) );

        LeaveVisual( visualImage );
    } // DoVisitImage
#endif
    // ----------------------------------------------------------------------
    void RtfHtmlConverter::DoVisitSpecial( RtfVisualSpecialChar* visualSpecialChar )
    {
        if ( !EnterVisual( visualSpecialChar ) )
        {
            return;
        }

        switch ( visualSpecialChar->CharKind() )
        {
            case RtfVisualSpecialCharKind::ParagraphNumberBegin:
                isInParagraphNumber = true;
                break;

            case RtfVisualSpecialCharKind::ParagraphNumberEnd:
                isInParagraphNumber = false;
                break;

            default:
                if ( SpecialCharacters().contains ( visualSpecialChar->CharKind() ) )
                {
                    writer.Write( SpecialCharacters()[ visualSpecialChar->CharKind() ] );
                }
                break;
        }

        LeaveVisual( visualSpecialChar );
    } // DoVisitSpecial

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::DoVisitBreak( RtfVisualBreak* visualBreak )
    {
        if ( !EnterVisual( visualBreak ) )
        {
            return;
        }

        switch ( visualBreak->BreakKind() )
        {
            case RtfVisualBreakKind::Line:
                RenderLineBreak();
                break;
            case RtfVisualBreakKind::Page:
                break;
            case RtfVisualBreakKind::Paragraph:
                if ( IsInParagraph() )
                {
                    EndParagraph(); // close paragraph
                }
                else if ( IsInListItem() )
                {
                    EndParagraph();
                    RenderEndTag( true ); // close list item
                }
                else
                {
                    BeginParagraph();
                    writer.Write( nonBreakingSpace );
                    EndParagraph();
                }
                break;
            case RtfVisualBreakKind::Section:
                break;
        }

        LeaveVisual( visualBreak );
    } // DoVisitBreak

    //#endregion // RtfVisuals

    // ----------------------------------------------------------------------
    QString RtfHtmlConverter::ConvertVisualHyperlink( QString& text )
    {
        if ( text.isEmpty() )
        {
            return QString();
        }

        if (text.indexOf("HYPERLINK") == 0)
        {
            //text is split with HYPERLINK "http://www.origin.com" |"www.origin.com"
            //so extract out the actual hyperlink part
            int divider = text.indexOf("|");
            if (divider > 0)
            {
                QStringList hyperlinkArray = text.split('|');

                //now remove the HYPERLINK at the beginning
                QString hyperlink = hyperlinkArray [0];
                hyperlink = hyperlink.right(hyperlink.length() - 9);
                hyperlink = hyperlink.trimmed();
                if (hyperlink.startsWith('"'))
                {
                    hyperlink = hyperlink.right (hyperlink.length()-1);
                }
                if (hyperlink.endsWith('"'))
                {
                    hyperlink = hyperlink.left (hyperlink.length()-1);
                }
                return (hyperlink);
            }
            else
            {
                return QString();
            }
        }
        else
        {
            return QString();
        }
        /*
                if ( hyperlinkRegEx == null )
                {
                    if ( string.IsNullOrEmpty( settings.VisualHyperlinkPattern ) )
                    {
                        return null;
                    }
                    hyperlinkRegEx = new Regex( settings.VisualHyperlinkPattern );
                }


                return hyperlinkRegEx.IsMatch( text ) ? text : null;
        */
    } // ConvertVisualHyperlink

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderDocumentSection()
    {
        if ( ( settings->ConvertScope() & RtfHtmlConvertScope::Document ) != RtfHtmlConvertScope::Document )
        {
            return;
        }

        RenderDocumentHeader();
    } // RenderDocumentSection

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderHtmlSection()
    {
        if ( ( settings->ConvertScope() & RtfHtmlConvertScope::Html ) == RtfHtmlConvertScope::Html )
        {
            RenderHtmlTag();
        }

        RenderHeadSection();
        RenderBodySection();

        if ( ( settings->ConvertScope() & RtfHtmlConvertScope::Html ) == RtfHtmlConvertScope::Html )
        {
            RenderEndTag( true );
        }
    } // RenderHtmlSection

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderHeadSection()
    {
        if ( ( settings->ConvertScope() & RtfHtmlConvertScope::Head ) != RtfHtmlConvertScope::Head )
        {
            return;
        }

        RenderHeadTag();
        RenderHeadAttributes();
        RenderTitle();
        RenderStyles();
        RenderEndTag( true );
    } // RenderHeadSection

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::RenderBodySection()
    {
        if ( ( settings->ConvertScope() & RtfHtmlConvertScope::Body ) == RtfHtmlConvertScope::Body )
        {
            RenderBodyTag();
        }

        if ( ( settings->ConvertScope() & RtfHtmlConvertScope::Content ) == RtfHtmlConvertScope::Content )
        {
            RenderRtfContent();
        }

        if ( ( settings->ConvertScope() & RtfHtmlConvertScope::Body ) == RtfHtmlConvertScope::Body )
        {
            RenderEndTag();
        }
    } // RenderBodySection

    // ----------------------------------------------------------------------
    bool RtfHtmlConverter::EnterVisual( RtfVisual* rtfVisual )
    {
        bool openList = EnsureOpenList( rtfVisual );
        if ( openList )
        {
            return false;
        }

        EnsureClosedList( rtfVisual );
        return OnEnterVisual( rtfVisual );
    } // EnterVisual

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::LeaveVisual( RtfVisual* rtfVisual )
    {
        OnLeaveVisual( rtfVisual );
        lastVisual = rtfVisual;
    } // LeaveVisual

    // ----------------------------------------------------------------------
    // returns true if visual is in list
    bool RtfHtmlConverter::EnsureOpenList( RtfVisual* rtfVisual )
    {
        RtfVisualText* visualText = static_cast<RtfVisualText*>(rtfVisual);
        if ( visualText == NULL || !isInParagraphNumber )
        {
            return false;
        }

        if ( !IsInList() )
        {
            bool unsortedList = (unsortedListValue == visualText->getText());
            if ( unsortedList )
            {
                RenderUlTag(); // unsorted list
            }
            else
            {
                RenderOlTag(); // ordered list
            }
        }

        RenderLiTag();

        return true;
    } // EnsureOpenList

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::EnsureClosedList()
    {
        if ( lastVisual == NULL )
        {
            return;
        }
        EnsureClosedList( lastVisual );
    } // EnsureClosedList

    // ----------------------------------------------------------------------
    void RtfHtmlConverter::EnsureClosedList( RtfVisual* rtfVisual )
    {
        if ( !IsInList() )
        {
            return; // not in list
        }

        RtfVisualBreak* previousParagraph = static_cast<RtfVisualBreak*>(lastVisual);
        if ( previousParagraph == NULL || previousParagraph->BreakKind() != RtfVisualBreakKind::Paragraph )
        {
            return; // is not following to a pragraph
        }

        RtfVisualSpecialChar* specialChar = static_cast<RtfVisualSpecialChar*>(rtfVisual);
        if ( specialChar == NULL ||
                specialChar->CharKind() != RtfVisualSpecialCharKind::ParagraphNumberBegin )
        {
            RenderEndTag( true ); // close ul/ol list
        }
    } // EnsureClosedList
} // namespace Itenso.Rtf.Converter.Html
// -- EOF -------------------------------------------------------------------
