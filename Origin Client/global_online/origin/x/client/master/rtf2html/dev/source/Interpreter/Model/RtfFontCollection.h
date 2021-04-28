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

#ifndef __RTFFONTCOLLECTION_H
#define __RTFFONTCOLLECTION_H

#include <qstring.h>
#include <qhash.h>
#include "ReadOnlyBaseCollection.h"
#include "RtfFont.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfFontCollection : public ReadOnlyBaseCollection
    {
        public:
            RtfFontCollection (QObject* parent = 0);
            ~RtfFontCollection ();

            // ----------------------------------------------------------------------
            bool ContainsFontWithId( QString fontId );

            // ----------------------------------------------------------------------
            RtfFont* Get(int index);

            // ----------------------------------------------------------------------
            RtfFont* Get (QString id);

            // ----------------------------------------------------------------------
            void CopyTo( RtfFont& font, int index );

            // ----------------------------------------------------------------------
            void Add( RtfFont* item );

            // ----------------------------------------------------------------------
            void Clear();

            // ----------------------------------------------------------------------
            // members
        private:
            QHash<QString, RtfFont*> fontByIdMap;

    }; // class RtfFontCollection

} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
#endif //__RTFFONTCOLLECTION_H