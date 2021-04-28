// -- FILE ------------------------------------------------------------------
// name       : IRtfDocumentPropertyCollection.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.23
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System.Collections;

#ifndef __RTFDOCUMENTPROPERTYCOLLECTION_H
#define __RTFDOCUMENTPROPERTYCOLLECTION_H

#include "ReadOnlyBaseCollection.h"
#include "RtfDocumentProperty.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfDocumentPropertyCollection : public ReadOnlyBaseCollection
    {
        public:
            RtfDocumentPropertyCollection (QObject* parent = 0);
            ~RtfDocumentPropertyCollection ();

            // ----------------------------------------------------------------------
            RtfDocumentProperty* Get (int index);

            // ----------------------------------------------------------------------
            RtfDocumentProperty* Get (QString& name);

            // ----------------------------------------------------------------------
            void CopyTo( RtfDocumentProperty* prop, int index );

            // ----------------------------------------------------------------------
            void Add( RtfDocumentProperty* item );

            // ----------------------------------------------------------------------
            void Clear();

    }; // interface IRtfDocumentPropertyCollection

} // namespace Itenso.Rtf
// -- EOF -------------------------------------------------------------------
#endif //__RTFDOCUMENTPROPERTYCOLLECTION_H