// -- FILE ------------------------------------------------------------------
// name       : RtfVisualCollection.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.21
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;

#ifndef __RTFVISUALCOLLECTION_H
#define __RTFVISUALCOLLECTION_H

#include "ReadOnlyBaseCollection.h"
#include "RtfVisual.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfVisualCollection : public ReadOnlyBaseCollection
    {
        public:
            RtfVisualCollection (QObject* parent = 0);
            ~RtfVisualCollection ();
            // ----------------------------------------------------------------------
            RtfVisual* Get (int index);

            // ----------------------------------------------------------------------
            void CopyTo( RtfVisual* visual, int index );

            // ----------------------------------------------------------------------
            void Add( RtfVisual* item );
            // ----------------------------------------------------------------------
            void Clear();

    }; // class RtfVisualCollection

} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
#endif //__RTFVISUALCOLLECTION_H