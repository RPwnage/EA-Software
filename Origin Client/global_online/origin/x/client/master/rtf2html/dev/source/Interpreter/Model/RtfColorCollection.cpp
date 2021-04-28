// -- FILE ------------------------------------------------------------------
// name       : RtfColorCollection.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.20
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;

#include <assert.h>
#include "RtfColorCollection.h"

namespace RTF2HTML
{
    RtfColorCollection::RtfColorCollection (QObject* parent)
        :ReadOnlyBaseCollection (parent)
    {
    }

    RtfColorCollection::~RtfColorCollection()
    {
    }

    // ----------------------------------------------------------------------
    RtfColor* RtfColorCollection::Get (int index)
    {
        return (static_cast<RtfColor*>(InnerList()[index]));
    } // this[ int ]

    // ----------------------------------------------------------------------
    void RtfColorCollection::CopyTo( RtfColor* color, int index )
    {
        Q_UNUSED (color);
        Q_UNUSED (index);
        assert (0);
        //InnerList.CopyTo( array, index );
    } // CopyTo

    // ----------------------------------------------------------------------
    void RtfColorCollection::Add( RtfColor* item )
    {
        if ( item == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "item" );
        }
        InnerList().push_back( item );
    } // Add

    // ----------------------------------------------------------------------
    void RtfColorCollection::Clear()
    {
        InnerList().clear();
    } // Clear


} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
