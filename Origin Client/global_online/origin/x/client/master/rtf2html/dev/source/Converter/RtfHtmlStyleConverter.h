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

#ifndef __RTFHTMLSTYLECONVERTER_H
#define __RTFHTMLSTYLECONVERTER_H

#include "RtfVisualText.h"
#include "RtfHtmlStyle.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfHtmlStyleConverter : public QObject
    {
        public:
            RtfHtmlStyleConverter(QObject* parent = 0);

            // ----------------------------------------------------------------------
            RtfHtmlStyle* TextToHtml( RtfVisualText* visualText );

    }; // class RtfHtmlStyleConverter

} // namespace Itenso.Rtf.Converter.Html
// -- EOF -------------------------------------------------------------------
#endif //__RTFHTMLSTYLECONVERTER_H