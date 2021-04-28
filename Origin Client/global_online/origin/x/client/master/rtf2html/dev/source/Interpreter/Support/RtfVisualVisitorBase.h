// -- FILE ------------------------------------------------------------------
// name       : RtfVisualVisitorBase.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.26
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
#ifndef __RTFVISUALVISITORBASE_H
#define __RTFVISUALVISITORBASE_H

#include "RtfVisualVisitor.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfVisualVisitorBase : public RtfVisualVisitor
    {
        public:

            RtfVisualVisitorBase (QObject* parent = 0);
            virtual ~RtfVisualVisitorBase();

            // ----------------------------------------------------------------------
            virtual void VisitText( RtfVisualText* visualText );

            // ----------------------------------------------------------------------
            virtual void DoVisitText( RtfVisualText* visualText );

            // ----------------------------------------------------------------------
            virtual void VisitBreak( RtfVisualBreak* visualBreak );

            // ----------------------------------------------------------------------
            virtual void DoVisitBreak( RtfVisualBreak* visualBreak );

            // ----------------------------------------------------------------------
            virtual void VisitSpecial( RtfVisualSpecialChar* visualSpecialChar );

            // ----------------------------------------------------------------------
            virtual void DoVisitSpecial( RtfVisualSpecialChar* visualSpecialChar );

#if 0 //UNIMPLEMENTED currently NOT USED
            // ----------------------------------------------------------------------
            public void VisitImage( IRtfVisualImage visualImage )

            // ----------------------------------------------------------------------
            protected virtual void DoVisitImage( IRtfVisualImage visualImage )
#endif
    }; // class RtfVisualVisitorBase

} // namespace Itenso.Rtf.Support
// -- EOF -------------------------------------------------------------------
#endif //__RTFVISUALVISITORBASE_H