// -- FILE ------------------------------------------------------------------
// name       : RtfVisualText.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.22
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using Itenso.Sys;

#include <assert.h>
#include "RtfVisualText.h"

namespace RTF2HTML
{
    // ----------------------------------------------------------------------
    RtfVisualText::RtfVisualText( QString& text, RtfTextFormat* format, QObject* parent )
        : RtfVisual (RtfVisualKind::Text, parent)
    {
        if ( text.isEmpty() )
        {
            assert (0);
//          throw new ArgumentNullException( "text" );
        }
        if ( format == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "format" );
        }
        this->text = text;
        this->format = format;

        toStr = "'" + text + "'";

    } // RtfVisualText

    // ----------------------------------------------------------------------
    void RtfVisualText::DoVisit( RtfVisualVisitor* visitor )
    {
        visitor->VisitText( this );
    } // DoVisit

    // ----------------------------------------------------------------------
    QString RtfVisualText::getText ()
    {
        return text;
    } // Text

    // ----------------------------------------------------------------------
    RtfTextFormat* RtfVisualText::getFormat ()
    {
        return format;
    }
    void RtfVisualText::setFormat (RtfTextFormat* format)
    {
        if ( format == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "value" );
        }
        this->format = format;
    } // Format

    // ----------------------------------------------------------------------
    bool RtfVisualText::IsEqual( ObjectBase* obj )
    {
        RtfVisualText* vt = static_cast<RtfVisualText*>(obj);

        return
            (vt != NULL &&
             RtfVisual::IsEqual( vt ) &&
             text == vt->text &&
             format == vt->format);
    } // IsEqual

#if 0 //UNIMPLEMENTED currently NOT USED
    // ----------------------------------------------------------------------
    protected override int ComputeHashCode()
    {
        int hash = base.ComputeHashCode();
        hash = HashTool.AddHashCode( hash, text );
        hash = HashTool.AddHashCode( hash, format );
        return hash;
    } // ComputeHashCode
#endif
    // ----------------------------------------------------------------------
    QString& RtfVisualText::ToString()
    {
        return toStr;
    } // ToString

} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
