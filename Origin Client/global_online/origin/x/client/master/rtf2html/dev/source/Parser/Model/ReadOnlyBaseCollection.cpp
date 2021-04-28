// -- FILE ------------------------------------------------------------------
// name       : ReadOnlyBaseCollection.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.20
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System.Collections;
//using Itenso.Sys;
//using Itenso.Sys.Collection;

#include <assert.h>
#include "ReadOnlyBaseCollection.h"

namespace RTF2HTML
{
    // ------------------------------------------------------------------------
    ReadOnlyBaseCollection::ReadOnlyBaseCollection(QObject* parent)
        : ObjectBase (parent)
    {
        objName = "ReadOnlyBaseCollection";
    }

    ReadOnlyBaseCollection::~ReadOnlyBaseCollection()
    {
    }

    int ReadOnlyBaseCollection::Count()
    {
        return mInnerList.size();
    }

    bool ReadOnlyBaseCollection::Equals (ObjectBase* obj)
    {
        ReadOnlyBaseCollection* collection = static_cast<ReadOnlyBaseCollection*>(obj);

        if ( this == collection )
        {
            return true;
        }

        if ( obj == NULL || (GetType() !=  collection->GetType() ))
        {
            return false;
        }

        return IsEqual( obj );
    } // Equals

    // ----------------------------------------------------------------------
    QString& ReadOnlyBaseCollection::ToString()
    {
        assert (0);
        return toStr;
        //CollectionTool.ToString doesn't seem to be called so ignore for now
        //return CollectionTool.ToString( this );
    } // ToString

    // ----------------------------------------------------------------------
    bool ReadOnlyBaseCollection::IsEqual( ObjectBase* obj )
    {
        Q_UNUSED (obj);
        //collectionTool never seems to be called so just assume true for now
        assert (0);
        return true;
        //return CollectionTool.AreEqual( this, obj );
    } // IsEqual

} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
