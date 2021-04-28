// -- FILE ------------------------------------------------------------------
// name       : RtfColorTableBuilder.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.21
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using Itenso.Rtf.Model;
//using Itenso.Rtf.Support;

#ifndef __RTFCOLORTABLEBUILDER_H
#define __RTFCOLORTABLEBUILDER_H

#include "RtfElementVisitorBase.h"
#include "RtfColorCollection.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfColorTableBuilder : public RtfElementVisitorBase
    {
        public:

            // ----------------------------------------------------------------------
            RtfColorTableBuilder( RtfColorCollection* colorTable, QObject* parent = 0 );

            // ----------------------------------------------------------------------
            void Reset();

            // ----------------------------------------------------------------------
            virtual void DoVisitGroup( RtfGroup* group );

            // ----------------------------------------------------------------------
            virtual void DoVisitTag( RtfTag* tag );

            // ----------------------------------------------------------------------
            virtual void DoVisitText( RtfText* text );

        private:
            // ----------------------------------------------------------------------
            // members
            RtfColorCollection* colorTable;

            int curRed;
            int curGreen;
            int curBlue;

    }; // class RtfColorBuilder

} // namespace Itenso.Rtf.Interpreter
// -- EOF -------------------------------------------------------------------
#endif //__RTFCOLORTABLEBUILDER_H