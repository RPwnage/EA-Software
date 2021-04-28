// -- FILE ------------------------------------------------------------------
// name       : RtfTextBuilder.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.23
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System.Text;
//using Itenso.Rtf.Support;

#ifndef __RTFTEXTBUILDER_H
#define __RTFTEXTBUILDER_H

#include <qstring.h>
#include "RtfElementVisitorBase.h"

namespace RTF2HTML
{
    // ------------------------------------------------------------------------
    class RtfTextBuilder : public RtfElementVisitorBase
    {
        public:

            // ----------------------------------------------------------------------
            RtfTextBuilder(QObject* parent = 0);

            // ----------------------------------------------------------------------
            QString& CombinedText ();

            // ----------------------------------------------------------------------
            void Reset();

            // ----------------------------------------------------------------------
            virtual void DoVisitText( RtfText* text );

        private:
            // ----------------------------------------------------------------------
            // members
            QString buffer;

    }; // class RtfTextBuilder

} // namespace Itenso.Rtf.Interpreter
// -- EOF -------------------------------------------------------------------
#endif //__RTFTEXTBUILDER_H