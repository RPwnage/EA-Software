// -- FILE ------------------------------------------------------------------
// name       : RtfHtmlConvertSettings.cs
// project    : RTF Framelet
// created    : Jani Giannoudis - 2008.06.02
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using System.Collections.Specialized;
//using Itenso.Rtf.Converter.Image;

#include "RtfHtmlConvertSettings.h"


namespace RTF2HTML
{
    QString RtfHtmlConvertSettings::DefaultDocumentHeader("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//DE\" \"http://www.w3.org/TR/html4/loose.dtd\">");
    QString RtfHtmlConvertSettings::DefaultDocumentCharacterSet ("UTF-8");
    QString RtfHtmlConvertSettings::DefaultSpecialCharRepresentation("");


    // ----------------------------------------------------------------------
    // ----------------------------------------------------------------------
    RtfHtmlConvertSettings::RtfHtmlConvertSettings(QObject* parent)
        : QObject (parent)
        , ignoreBGcolor (false)
    {
        this->convertScope = RtfHtmlConvertScope::All;

        documentHeader = DefaultDocumentHeader;
        characterSet = DefaultDocumentCharacterSet;
    } // RtfHtmlConvertSettings

    // ----------------------------------------------------------------------
    RtfHtmlConvertScope::RtfHtmlConvertScope RtfHtmlConvertSettings::ConvertScope ()
    {
        return convertScope;
    }
    // ----------------------------------------------------------------------
    bool RtfHtmlConvertSettings::HasStyles ()
    {
        return false;
    } // HasStyles

#if 0 //UNIMPLEMENTED, currently NOT USED
    // ----------------------------------------------------------------------
    public RtfHtmlCssStyleCollection Styles
    {
        get { return styles ?? ( styles = new RtfHtmlCssStyleCollection() ); }
    } // Styles

    // ----------------------------------------------------------------------
    public bool HasStyleSheetLinks
    {
        get { return styleSheetLinks != null && styleSheetLinks.Count > 0; }
    } // HasStyleSheetLinks

    // ----------------------------------------------------------------------
    public StringCollection StyleSheetLinks
    {
        get { return styleSheetLinks ?? ( styleSheetLinks = new StringCollection() ); }
    } // StyleSheetLinks
#endif
    // ----------------------------------------------------------------------
    QString& RtfHtmlConvertSettings::DocumentHeader()
    {
        return documentHeader;
    } // DocumentHeader

    // ----------------------------------------------------------------------
    QString& RtfHtmlConvertSettings::getTitle ()
    {
        return title;
    }
    void RtfHtmlConvertSettings::setTitle (QString& title)
    {
        this->title = title;
    }
    // ----------------------------------------------------------------------
    QString& RtfHtmlConvertSettings::CharacterSet ()
    {
        return characterSet;
    } // CharacterSet

#if 0 //UNIMPLEMENTED, currently NOT USED
    // ----------------------------------------------------------------------
    public string VisualHyperlinkPattern { get; set; }
#endif
    // ----------------------------------------------------------------------
    QString& RtfHtmlConvertSettings::SpecialCharsRepresentation ()
    {
        return DefaultSpecialCharRepresentation;
    }

    // ----------------------------------------------------------------------
    bool RtfHtmlConvertSettings::IsShowHiddenText ()
    {
        return false;
    }

#if 0 //UNIMPLEMENTED, currently NOT USED
    // ----------------------------------------------------------------------
    public bool ConvertVisualHyperlinks { get; set; }
#endif
    // ----------------------------------------------------------------------
    bool RtfHtmlConvertSettings::UseNonBreakingSpaces ()
    {
        return false;
    }

#if 0 //UNIMPLEMENTED, currently NOT USED
    // ----------------------------------------------------------------------
    public string ImagesPath { get; set; }

    // ----------------------------------------------------------------------
    public string GetImageUrl( int index, RtfVisualImageFormat rtfVisualImageFormat )
    {
        string imageFileName = imageAdapter.ResolveFileName( index, rtfVisualImageFormat );
        return imageFileName.Replace( '\\', '/' );
    } // GetImageUrl
#endif

} // namespace Itenso.Rtf.Converter.Html
// -- EOF -------------------------------------------------------------------
