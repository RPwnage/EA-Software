// -- FILE ------------------------------------------------------------------
// name       : IRtfColorCollection.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.21
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System.Collections;

#ifndef __RTFCOLORCOLLECTION_H
#define __RTFCOLORCOLLECTION_H

#include "ReadOnlyBaseCollection.h"
#include "RtfColor.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfColorCollection : public ReadOnlyBaseCollection
    {
        public:
            RtfColorCollection (QObject* parent = 0);
            ~RtfColorCollection();

            // ----------------------------------------------------------------------
            RtfColor* Get (int index);

            // ----------------------------------------------------------------------
            void CopyTo ( RtfColor* color, int index);

            // ----------------------------------------------------------------------
            void Add( RtfColor* item );

            // ----------------------------------------------------------------------
            void Clear();

    }; // interface IRtfColorCollection

} // namespace Itenso.Rtf
// -- EOF -------------------------------------------------------------------
#endif //__RTFCOLORCOLLECTION_H