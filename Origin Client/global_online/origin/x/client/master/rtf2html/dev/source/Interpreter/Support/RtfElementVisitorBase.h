// -- FILE ------------------------------------------------------------------
// name       : RtfElementVisitorBase.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.19
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
#ifndef __RTFELEMENTVISITORBASE_H
#define __RTFELEMENTVISITORBASE_H

#include "RtfElementVisitor.h"

namespace RTF2HTML
{
    namespace RtfElementVisitorOrder
    {
        enum RtfElementVisitorOrder
        {
            NonRecursive,
            DepthFirst,
            BreadthFirst
        }; // enum RtfElementVisitorOrder
    }

    // ------------------------------------------------------------------------
    class RtfElementVisitorBase : public RtfElementVisitor
    {
        public:
            // ----------------------------------------------------------------------
            RtfElementVisitorBase( RtfElementVisitorOrder::RtfElementVisitorOrder order, QObject* parent = 0 );

            // ----------------------------------------------------------------------
            virtual void VisitTag( RtfTag* tag );

            // ----------------------------------------------------------------------
            virtual void DoVisitTag( RtfTag* tag );

            // ----------------------------------------------------------------------
            virtual void VisitGroup( RtfGroup* group );

            // ----------------------------------------------------------------------
            virtual void DoVisitGroup( RtfGroup* group );

            // ----------------------------------------------------------------------
            void VisitGroupChildren( RtfGroup* group );

            // ----------------------------------------------------------------------
            virtual void VisitText( RtfText* text );

            // ----------------------------------------------------------------------
            virtual void DoVisitText( RtfText* text );

            // ----------------------------------------------------------------------
            // members
        private:
            RtfElementVisitorOrder::RtfElementVisitorOrder order;

    }; // class RtfElementVisitorBase

} // namespace Itenso.Rtf.Support
// -- EOF -------------------------------------------------------------------
#endif //__RTFELEMENTVISITORBASE_H