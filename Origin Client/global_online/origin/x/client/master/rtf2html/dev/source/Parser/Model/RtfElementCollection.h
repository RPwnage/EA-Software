// -- FILE ------------------------------------------------------------------
// name       : RtfElementCollection.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.19
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;

#ifndef RTFELEMENTCOLLECTION_H
#define RTFELEMENTCOLLECTION_H

#include "RtfElement.h"
#include "ReadOnlyBaseCollection.h"

namespace RTF2HTML
{
    // ------------------------------------------------------------------------
    class RtfElementCollection : public ReadOnlyBaseCollection //, public IRtfElementCollection
    {
        public:
            RtfElementCollection();
            ~RtfElementCollection();

            // ----------------------------------------------------------------------
            RtfElement* Get (int index);

            // ----------------------------------------------------------------------
            /*
                    void CopyTo( IRtfElement[] array, int index )
                    {
                        InnerList.CopyTo( array, index );
                    } // CopyTo
            */

            // ----------------------------------------------------------------------
            void Add( RtfElement* item );

            // ----------------------------------------------------------------------
            void Clear();

    }; // class RtfElementCollection

} // namespace Itenso.Rtf.Model
#endif //RTFELEMENTCOLLECTION_H
// -- EOF -------------------------------------------------------------------
