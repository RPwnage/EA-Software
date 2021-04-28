// -- FILE ------------------------------------------------------------------
// name       : RtfHtmlElementPath.cs
// project    : RTF Framelet
// created    : Jani Giannoudis - 2008.06.09
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System.Text;
//using System.Collections;
//using System.Web.UI;

#ifndef __RTFHTMLELEMENTPATH_H
#define __RTFHTMLELEMENTPATH_H

#include <qstring.h>
#include <qstack.h>

#include "ObjectBase.h"
#include "HtmlTextWriter.h"

namespace RTF2HTML
{
    // ------------------------------------------------------------------------
    class RtfHtmlElementPath : public ObjectBase
    {
        public:
            RtfHtmlElementPath();

            // ----------------------------------------------------------------------
            int Count ();

            // ----------------------------------------------------------------------
            HtmlTextWriterTag::Tag Current ();

            // ----------------------------------------------------------------------
            bool IsCurrent( HtmlTextWriterTag::Tag tag );

            // ----------------------------------------------------------------------
            bool Contains( HtmlTextWriterTag::Tag tag );

            // ----------------------------------------------------------------------
            void Push( HtmlTextWriterTag::Tag tag );

            // ----------------------------------------------------------------------
            void Pop();

            // ----------------------------------------------------------------------
            virtual QString& ToString();

            // ----------------------------------------------------------------------
            // members
        private:
            QStack <HtmlTextWriterTag::Tag> elements;

            QString toStr;

    }; // class RtfHtmlElementPath

} // namespace Itenso.Rtf.Converter.Html
// -- EOF -------------------------------------------------------------------
#endif //__RTFHTMLELEMENTPATH_H