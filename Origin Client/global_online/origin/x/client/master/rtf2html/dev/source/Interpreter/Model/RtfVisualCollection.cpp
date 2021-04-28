// -- FILE ------------------------------------------------------------------
// name       : RtfVisualCollection.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.21
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;

#include <assert.h>
#include "RtfVisualCollection.h"

namespace RTF2HTML
{

    RtfVisualCollection::RtfVisualCollection (QObject* parent)
        : ReadOnlyBaseCollection (parent)
    {
    }

    RtfVisualCollection::~RtfVisualCollection ()
    {
    }

    // ----------------------------------------------------------------------
    RtfVisual* RtfVisualCollection::Get (int index)
    {
        return (RtfVisual*)InnerList()[ index ];
    } // this[ int ]

    // ----------------------------------------------------------------------
    void RtfVisualCollection::CopyTo( RtfVisual* visual, int index )
    {
        Q_UNUSED (visual);
        Q_UNUSED (index);
        assert (0);
        //InnerList.CopyTo( array, index );
    } // CopyTo

    // ----------------------------------------------------------------------
    void RtfVisualCollection::Add( RtfVisual* item )
    {
        if ( item == NULL)
        {
            assert (0);
            //throw new ArgumentNullException( "item" );
        }
        InnerList().push_back( item );
    } // Add

    // ----------------------------------------------------------------------
    void RtfVisualCollection::Clear()
    {
        InnerList().clear();
    } // Clear

} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
