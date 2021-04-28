// -- FILE ------------------------------------------------------------------
// name       : IRtfVisualVisitor.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.26
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
#ifndef __RTF_VISUALVISITOR_H
#define __RTF_VISUALVISITOR_H

#include <qobject.h>

namespace RTF2HTML
{
    class RtfVisualText;
    class RtfVisualBreak;
    class RtfVisualSpecialChar;
    class RtfVisualImage;
}

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfVisualVisitor : public QObject
    {
        public:
            RtfVisualVisitor (QObject* parent = 0):QObject (parent) {};
            virtual ~RtfVisualVisitor() {};

            // ----------------------------------------------------------------------
            virtual void VisitText( RtfVisualText* /*visualText*/ ) {};

            // ----------------------------------------------------------------------
            virtual void VisitBreak( RtfVisualBreak* /*visualBreak*/ ) {};

            // ----------------------------------------------------------------------
            virtual void VisitSpecial( RtfVisualSpecialChar* /*visualSpecialChar*/ ) {};

#if 0 //UNIMPLEMENTED currently NOT USED
            // ----------------------------------------------------------------------
            virtual void VisitImage( RtfVisualImage& visualImage );
#endif

    }; // interface IRtfVisualVisitor

} // namespace Itenso.Rtf
// -- EOF -------------------------------------------------------------------
#endif //__RTF_VISUALVISITOR_H