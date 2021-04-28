// -- FILE ------------------------------------------------------------------
// name       : IRtfTextFormatCollection.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.21
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System.Collections;

#ifndef __RTFTEXTFORMATCOLLECTION_H
#define __RTFTEXTFORMATCOLLECTION_H

#include "ReadOnlyBaseCollection.h"
#include "RtfTextFormat.h"


namespace RTF2HTML
{
    // ------------------------------------------------------------------------
    class RtfTextFormatCollection : public ReadOnlyBaseCollection
    {
        public:
            RtfTextFormatCollection (QObject* parent = 0);
            ~RtfTextFormatCollection();

            // ----------------------------------------------------------------------
            RtfTextFormat* Get (int index);

            // ----------------------------------------------------------------------
            bool Contains( RtfTextFormat* format );

            // ----------------------------------------------------------------------
            int IndexOf( RtfTextFormat* format );

            // ----------------------------------------------------------------------
            void CopyTo( RtfTextFormat* format, int index );

            // ----------------------------------------------------------------------
            void Add( RtfTextFormat* item );
            // ----------------------------------------------------------------------
            void Clear();

    }; // interface IRtfTextFormatCollection

} // namespace Itenso.Rtf
// -- EOF -------------------------------------------------------------------
#endif //__RTFTEXTFORMATCOLLECTION_H