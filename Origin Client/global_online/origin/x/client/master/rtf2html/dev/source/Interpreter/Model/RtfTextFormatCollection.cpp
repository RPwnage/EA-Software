// -- FILE ------------------------------------------------------------------
// name       : RtfTextFormatCollection.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.21
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;

#include <assert.h>
#include "RtfTextFormatCollection.h"

namespace RTF2HTML
{
    RtfTextFormatCollection::RtfTextFormatCollection (QObject* parent)
        : ReadOnlyBaseCollection (parent)
    {
    }

    RtfTextFormatCollection::~RtfTextFormatCollection()
    {
    }

    // ------------------------------------------------------------------------
    // ----------------------------------------------------------------------
    RtfTextFormat* RtfTextFormatCollection::Get (int index)
    {
        return (static_cast<RtfTextFormat*>(InnerList()[index]));
    } // this[ int ]

    // ----------------------------------------------------------------------
    bool RtfTextFormatCollection::Contains( RtfTextFormat* format )
    {
        std::vector<RTF2HTML::ObjectBase*>::iterator it;

        for (it = InnerList().begin(); it != InnerList().end(); it++)
        {
            if ((*it) == (ObjectBase*)format)
                return true;
        }

        return false;
    } // Contains

    // ----------------------------------------------------------------------
    int RtfTextFormatCollection::IndexOf( RtfTextFormat* format )
    {
        if ( format != NULL )
        {
            // PERFORMANCE: most probably we should maintain a hashmap for fast searching ...
            int count = Count();
            for ( int i = 0; i < count; i++ )
            {
                if ( format->Equals( InnerList()[ i ] ) )
                {
                    return i;
                }
            }
        }
        return -1;
    } // IndexOf

    // ----------------------------------------------------------------------
    void RtfTextFormatCollection::CopyTo( RtfTextFormat* format, int index )
    {
        Q_UNUSED (format);
        Q_UNUSED (index);
        assert (0);
//      InnerList.CopyTo( array, index );
    } // CopyTo

    // ----------------------------------------------------------------------
    void RtfTextFormatCollection::Add( RtfTextFormat* item )
    {
        if ( item == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "item" );
        }
        InnerList().push_back( item );
    } // Add

    // ----------------------------------------------------------------------
    void RtfTextFormatCollection::Clear()
    {
        InnerList().clear();
    } // Clear


} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
