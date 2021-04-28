// -- FILE ------------------------------------------------------------------
// name       : RtfVisualBreak.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.22
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using Itenso.Sys;

#ifndef __RTFVISUALBREAK_H
#define __RTFVISUALBREAK_H

#include "RtfVisual.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    namespace RtfVisualBreakKind
    {
        enum RtfVisualBreakKind
        {
            Line,
            Page,
            Paragraph,
            Section
        }; // enum RtfVisualBreakKind
    }

    // ------------------------------------------------------------------------
    class RtfVisualBreak : public RtfVisual
    {
        public:

            // ----------------------------------------------------------------------
            RtfVisualBreak( RtfVisualBreakKind::RtfVisualBreakKind breakKind, QObject* parent = 0 );

            // ----------------------------------------------------------------------
            RtfVisualBreakKind::RtfVisualBreakKind BreakKind ();

            // ----------------------------------------------------------------------
            virtual QString& ToString();
            virtual bool IsEqual (RTF2HTML::ObjectBase* obj );

            // ----------------------------------------------------------------------
            virtual void DoVisit( RtfVisualVisitor* visitor );

            // ----------------------------------------------------------------------
            // members
        private:
            void setBreakKindString();

            RtfVisualBreakKind::RtfVisualBreakKind breakKind;

            QString toStr;

    }; // class RtfVisualBreak

} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
#endif //__RTFVISUALBREAK_H