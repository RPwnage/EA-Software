// -- FILE ------------------------------------------------------------------
// name       : RtfElementCollection.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.19
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;

#include <assert.h>
#include "RtfElementCollection.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    RtfElementCollection::RtfElementCollection()
    {
    }

    // ------------------------------------------------------------------------
    RtfElementCollection::~RtfElementCollection()
    {
    }

    // ----------------------------------------------------------------------
    RtfElement* RtfElementCollection::Get(int index)
    {
        return (static_cast<RtfElement*>(InnerList()[index]));
    } // this[ int ]

    /*
        // ----------------------------------------------------------------------
        void CopyTo( IRtfElement[] array, int index )
        {
            InnerList.CopyTo( array, index );
        } // CopyTo
    */
    // ----------------------------------------------------------------------
    void RtfElementCollection::Add( RtfElement* item )
    {
        //if ( item == NULL )
        //{
        //          assert (0);
        //}
        InnerList().push_back(static_cast<ObjectBase*>(item));
    } // Add

    // ----------------------------------------------------------------------
    void RtfElementCollection::Clear()
    {
        InnerList().clear();
    } // Clear

} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
