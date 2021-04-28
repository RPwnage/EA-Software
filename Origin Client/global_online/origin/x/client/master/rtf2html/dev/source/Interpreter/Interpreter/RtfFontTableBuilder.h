// -- FILE ------------------------------------------------------------------
// name       : RtfFontTableBuilder.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.21
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using Itenso.Rtf.Model;
//using Itenso.Rtf.Support;

#ifndef __RTFFONTTABLEBUILDER_H
#define __RTFFONTTABLEBUILDER_H

#include "RtfElementVisitorBase.h"
#include "RtfFontCollection.h"
#include "RtfFontBuilder.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfFontTableBuilder : public RtfElementVisitorBase
    {
        public:
            // ----------------------------------------------------------------------
            RtfFontTableBuilder( RtfFontCollection* fontTable, bool ignoreDuplicatedFonts = false, QObject* parent = 0 );

            // ----------------------------------------------------------------------
            bool IgnoreDuplicatedFonts ();

            // ----------------------------------------------------------------------
            void Reset();

            // ----------------------------------------------------------------------
            virtual void DoVisitGroup( RtfGroup* group );

            // ----------------------------------------------------------------------
            void BuildFontFromGroup( RtfGroup* group );

        private:
            // ----------------------------------------------------------------------
            void AddCurrentFont();

            // ----------------------------------------------------------------------
            // members
            RtfFontBuilder* fontBuilder;
            RtfFontCollection* fontTable;
            bool ignoreDuplicatedFonts;

    }; // class RtfFontBuilder

} // namespace Itenso.Rtf.Interpreter
// -- EOF -------------------------------------------------------------------
#endif //__RTFFONTTABLEBUILDER_H