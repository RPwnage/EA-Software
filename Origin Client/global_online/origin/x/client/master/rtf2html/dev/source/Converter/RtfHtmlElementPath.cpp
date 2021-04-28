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

#include <assert.h>
#include "RtfHtmlElementPath.h"

namespace RTF2HTML
{
    RtfHtmlElementPath::RtfHtmlElementPath()
    {
    }

    // ----------------------------------------------------------------------
    int RtfHtmlElementPath::Count ()
    {
        return elements.size();
    } // Count

    // ----------------------------------------------------------------------
    HtmlTextWriterTag::Tag RtfHtmlElementPath::Current ()
    {
        if (elements.size())
        {
            return elements.top();
        }

        return HtmlTextWriterTag::Unknown;
    } // Current

    // ----------------------------------------------------------------------
    bool RtfHtmlElementPath::IsCurrent( HtmlTextWriterTag::Tag tag )
    {
        return Current() == tag;
    } // IsCurrent

    // ----------------------------------------------------------------------
    bool RtfHtmlElementPath::Contains( HtmlTextWriterTag::Tag tag )
    {
        return elements.contains( tag );
    } // Contains

    // ----------------------------------------------------------------------
    void RtfHtmlElementPath::Push( HtmlTextWriterTag::Tag tag )
    {
        elements.push( tag );
    } // Push

    // ----------------------------------------------------------------------
    void RtfHtmlElementPath::Pop()
    {
        elements.pop();
    } // Pop

    // ----------------------------------------------------------------------
    QString& RtfHtmlElementPath::ToString()
    {
        //doesn't seem to be used so ignore implementing for now
        assert (0);
        return toStr;
#if 0 //UNIMPLEMENTED, currently NOT USED
        if ( elements.size() == 0 )
        {
            return ObjectBase.ToString();
        }

        bool first = true;

        int numElements = elements.size();
        for (int i = 0; i < numElements; i++)
        {
            if ( !first )
            {
                toStr.prepend (" > ");
            }
            toStr.prepend (sb.Insert( 0, element.ToString() );
                           first = false;
        }

        return sb.ToString();
#endif
    } // ToString

} // namespace Itenso.Rtf.Converter.Html
// -- EOF -------------------------------------------------------------------
