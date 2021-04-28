// -- FILE ------------------------------------------------------------------
// name       : RtfHtmlStyleConverter.cs
// project    : RTF Framelet
// created    : Jani Giannoudis - 2008.07.13
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using System.Drawing;

#include <assert.h>
#include <qcolor.h>
#include "RtfHtmlStyleConverter.h"


namespace RTF2HTML
{
    RtfHtmlStyleConverter::RtfHtmlStyleConverter(QObject* parent)
        : QObject (parent)
    {
    }

    // ----------------------------------------------------------------------
    RtfHtmlStyle* RtfHtmlStyleConverter::TextToHtml( RtfVisualText* visualText )
    {
        if ( visualText == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "visualText" );
        }

        RtfHtmlStyle* htmlStyle = new RtfHtmlStyle(this);

        RtfTextFormat* textFormat = visualText->getFormat();

        // background color
        QColor backgroundColor = textFormat->BackgroundColor()->AsDrawingColor();
        if ( backgroundColor.red() != 0 || backgroundColor.green() != 0 || backgroundColor.blue() != 0 )
        {
            QString bcName = backgroundColor.name().toUpper();
            htmlStyle->setBackgroundColor (bcName );
        }

        // foreground color
        QColor foregroundColor = textFormat->ForegroundColor()->AsDrawingColor();
        if ( foregroundColor.red() != 0 || foregroundColor.green() != 0 || foregroundColor.blue() != 0 )
        {
            QString fgName = foregroundColor.name().toUpper();
            htmlStyle->setForegroundColor (fgName );
        }

        // font
        htmlStyle->setFontFamily (textFormat->Font()->Name());
        if ( textFormat->FontSize() > 0 )
        {
            QString fsz = QString("%0").arg((textFormat->FontSize())/2);
            fsz += "pt";
            htmlStyle->setFontSize( fsz );
        }

        return htmlStyle;
    } // TextToHtml
} // namespace Itenso.Rtf.Converter.Html
// -- EOF -------------------------------------------------------------------
