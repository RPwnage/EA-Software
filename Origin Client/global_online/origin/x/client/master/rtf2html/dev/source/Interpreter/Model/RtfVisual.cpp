// -- FILE ------------------------------------------------------------------
// name       : RtfVisual.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.21
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using Itenso.Sys;

#include <assert.h>
#include "RtfVisual.h"

namespace RTF2HTML
{
    // ----------------------------------------------------------------------
    RtfVisual::RtfVisual( RtfVisualKind::RtfVisualKind kind, QObject* parent )
        : ObjectBase (parent)
    {
        this->kind = kind;
    } // RtfVisual

    // ----------------------------------------------------------------------
    RtfVisualKind::RtfVisualKind RtfVisual::Kind ()
    {
        return kind;
    } // Kind

    // ----------------------------------------------------------------------
    void RtfVisual::Visit( RtfVisualVisitor* visitor )
    {
        if ( visitor == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "visitor" );
        }
        DoVisit( visitor );
    } // Visit

    // ----------------------------------------------------------------------
    bool RtfVisual::Equals( ObjectBase* obj )
    {
        RtfVisual* v = static_cast<RtfVisual*>(obj);
        if ( v == this )
        {
            return true;
        }

        if ( v == NULL || GetType() != obj->GetType() )
        {
            return false;
        }

        return IsEqual( obj );
    } // Equals

#if 0 //UNIMPLEMENTED currently NOT USED
    // ----------------------------------------------------------------------
    public sealed override int GetHashCode()
    {
        return HashTool.AddHashCode( GetType().GetHashCode(), ComputeHashCode() );
    } // GetHashCode
#endif
    // ----------------------------------------------------------------------
    void RtfVisual::DoVisit( RtfVisualVisitor* visitor )
    {
        Q_UNUSED (visitor);
    }

    // ----------------------------------------------------------------------
    bool RtfVisual::IsEqual( ObjectBase* obj )
    {
        Q_UNUSED (obj);
        return true;
    } // IsEqual

#if 0 //UNIMPLEMENTED currently NOT USED
    // ----------------------------------------------------------------------
    protected virtual int ComputeHashCode()
    {
        return 0x0f00ba11;
    } // ComputeHashCode
#endif

} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
