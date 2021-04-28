// -- FILE ------------------------------------------------------------------
// name       : RtfFontCollection.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.20
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using System.Collections;

#include "assert.h"
#include "RtfFontCollection.h"

namespace RTF2HTML
{
    RtfFontCollection::RtfFontCollection (QObject* parent)
        : ReadOnlyBaseCollection (parent)
    {
    }

    RtfFontCollection::~RtfFontCollection()
    {
    }
    // ----------------------------------------------------------------------
    bool RtfFontCollection::ContainsFontWithId( QString fontId )
    {
        QHash<QString, RtfFont*>::const_iterator it = fontByIdMap.find (fontId);
        return (it != fontByIdMap.end());
    } // ContainsFontWithId

    // ----------------------------------------------------------------------
    RtfFont* RtfFontCollection::Get (int index)
    {
        return (static_cast<RtfFont*>(InnerList()[index]));
    } // this[ int ]

    // ----------------------------------------------------------------------
    RtfFont* RtfFontCollection::Get (QString id)
    {
        QHash<QString, RtfFont*>::const_iterator it = fontByIdMap.find (id);
        if (it != fontByIdMap.end())
        {
            return (RtfFont*)(it.value());
        }
        return NULL;
    } // this[ string ]

    // ----------------------------------------------------------------------
    void RtfFontCollection::CopyTo( RtfFont& font, int index )
    {
        Q_UNUSED (font);
        Q_UNUSED (index);
        assert (0);
        //InnerList.CopyTo( array, index );
    } // CopyTo

    // ----------------------------------------------------------------------
    void RtfFontCollection::Add( RtfFont* item )
    {
        if ( item == NULL)
        {
            assert (0);
            //throw new ArgumentNullException( "item" );
        }
        InnerList().push_back ( item );
        if (!ContainsFontWithId (item->Id()))
        {
            fontByIdMap [item->Id()] = item;
        }
    } // Add

    // ----------------------------------------------------------------------
    void RtfFontCollection::Clear()
    {
        InnerList().clear();
        fontByIdMap.clear();
    } // Clear

    // ----------------------------------------------------------------------
    // members
//      private readonly Hashtable fontByIdMap = new Hashtable();


} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
