// -- FILE ------------------------------------------------------------------
// name       : IRtfElementVisitor.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.19
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
#ifndef __RTFIRTFELEMENTVISITOR_H
#define __RTFIRTFELEMENTVISITOR_H

#include <qobject.h>

namespace RTF2HTML
{
    class RtfTag;
    class RtfGroup;
    class RtfText;
}

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfElementVisitor: public QObject
    {
        public:

            RtfElementVisitor(QObject* parent = 0) :QObject(parent) {};

            // ----------------------------------------------------------------------
            virtual void VisitTag( RTF2HTML::RtfTag* /*tag*/ ) {};

            // ----------------------------------------------------------------------
            virtual void VisitGroup( RTF2HTML::RtfGroup* /*group */ ) {};

            // ----------------------------------------------------------------------
            virtual void VisitText( RTF2HTML::RtfText* /*text*/ ) {};

    }; // interface IRtfElementVisitor

} // namespace Itenso.Rtf
#endif //__RTFIRTFELEMENTVISITOR_H
// -- EOF -------------------------------------------------------------------
