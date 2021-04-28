// -- FILE ------------------------------------------------------------------
// name       : RtfDocumentPropertyCollection.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.23
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;

#include <assert.h>
#include "RtfDocumentPropertyCollection.h"

namespace RTF2HTML
{
    RtfDocumentPropertyCollection::RtfDocumentPropertyCollection (QObject* parent)
        :ReadOnlyBaseCollection (parent)
    {
    }

    RtfDocumentPropertyCollection::~RtfDocumentPropertyCollection()
    {
    }

    // ----------------------------------------------------------------------
    RtfDocumentProperty* RtfDocumentPropertyCollection::Get (int index)
    {
        return (RtfDocumentProperty*)InnerList()[ index ];
    } // this[ int ]

    // ----------------------------------------------------------------------
    RtfDocumentProperty*  RtfDocumentPropertyCollection::Get (QString& name)
    {
        if ( !name.isEmpty() )
        {
            std::vector<RTF2HTML::ObjectBase*>::iterator it;

            for (it = InnerList().begin(); it != InnerList().end(); it++)
            {
                RtfDocumentProperty* prop = (RtfDocumentProperty*)&(it);
                if (prop->Name() == name)
                {
                    return prop;
                }
            }
        }
        return NULL;
    } // this[ string ]

    // ----------------------------------------------------------------------
    void RtfDocumentPropertyCollection::CopyTo( RtfDocumentProperty* prop, int index )
    {
        Q_UNUSED (prop);
        Q_UNUSED (index);
        assert (0);
        //InnerList.CopyTo( array, index );
    } // CopyTo

    // ----------------------------------------------------------------------
    void RtfDocumentPropertyCollection::Add( RtfDocumentProperty* item )
    {
        if ( item == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "item" );
        }
        InnerList().push_back( item );
    } // Add

    // ----------------------------------------------------------------------
    void RtfDocumentPropertyCollection::Clear()
    {
        InnerList().clear();
    } // Clear


} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
