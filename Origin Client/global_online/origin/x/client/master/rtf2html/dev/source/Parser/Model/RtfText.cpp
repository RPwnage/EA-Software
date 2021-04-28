// -- FILE ------------------------------------------------------------------
// name       : RtfText.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.19
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using Itenso.Sys;

#include <assert.h>
#include "RtfText.h"


namespace RTF2HTML
{
    // ------------------------------------------------------------------------
    // ----------------------------------------------------------------------
    RtfText::RtfText( QString text, int codePage, QObject* parent )
        :RtfElement( RtfElementKind::Text, parent )
    {
        if ( text.isEmpty() )
        {
            assert (0);
        }
        this->text = text;
        this->textCodePage = codePage;
    } // RtfText
    // ----------------------------------------------------------------------
    RtfText::~RtfText()
    {
    }

    // ----------------------------------------------------------------------
    QString& RtfText::Text()
    {
        return text;
    } // Text


    // ----------------------------------------------------------------------
    QString& RtfText::ToString()
    {
        return text;
    } // Text

    // ----------------------------------------------------------------------
    // ----------------------------------------------------------------------
    void RtfText::DoVisit( RtfElementVisitor* visitor )
    {
        visitor->VisitText( this );
    } // DoVisit

    // ----------------------------------------------------------------------
    bool RtfText::IsEqual( ObjectBase* obj )
    {
        //doesn't seem to be called so for now, ignore
        Q_UNUSED (obj);
        assert (0);
        return true;
        /*
                RtfText compare = obj as RtfText; // guaranteed to be non-null
                return compare != null && base.IsEqual( obj ) &&
                    text.Equals( compare.text );
        */
    } // IsEqual

    /*
        // ----------------------------------------------------------------------
        protected override int ComputeHashCode()
        {
            return HashTool.AddHashCode( base.ComputeHashCode(), text );
        } // ComputeHashCode
    */

} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
