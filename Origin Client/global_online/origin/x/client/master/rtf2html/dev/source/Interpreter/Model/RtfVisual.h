// -- FILE ------------------------------------------------------------------
// name       : RtfVisual.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.21
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using Itenso.Sys;

#ifndef __RTFVISUAL_H
#define __RTFVISUAL_H

#include "ObjectBase.h"
#include "RtfVisualVisitor.h"

namespace RTF2HTML
{
    namespace RtfVisualKind
    {
        enum RtfVisualKind
        {
            Text,
            Break,
            Special
            //      ,Image
        }; // enum RtfVisualKind
    }

    class RtfVisual : public ObjectBase
    {
        public:

            // ----------------------------------------------------------------------
            RtfVisual( RtfVisualKind::RtfVisualKind kind, QObject* parent);

            // ----------------------------------------------------------------------
            RtfVisualKind::RtfVisualKind Kind ();

            // ----------------------------------------------------------------------

            virtual bool Equals (RTF2HTML::ObjectBase* obj);
            virtual bool IsEqual (RTF2HTML::ObjectBase* obj );

            virtual void Visit( RtfVisualVisitor* visitor );
            virtual void DoVisit( RtfVisualVisitor* visitor );

            // ----------------------------------------------------------------------
            // members
        private:
            RtfVisualKind::RtfVisualKind kind;

    }; // class RtfVisual

} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
#endif //__RTFVISUAL_H