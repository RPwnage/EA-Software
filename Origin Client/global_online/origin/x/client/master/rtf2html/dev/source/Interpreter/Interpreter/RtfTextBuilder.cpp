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

#include "RtfTextBuilder.h"
#include "RtfText.h"

namespace RTF2HTML
{

    // ----------------------------------------------------------------------
    RtfTextBuilder::RtfTextBuilder(QObject* parent)
        :RtfElementVisitorBase ( RtfElementVisitorOrder::DepthFirst, parent )
    {
        Reset();
    } // RtfTextBuilder

    // ----------------------------------------------------------------------
    QString& RtfTextBuilder::CombinedText ()
    {
        return buffer;
    } // CombinedText

    // ----------------------------------------------------------------------
    void RtfTextBuilder::Reset()
    {
        buffer.clear();
    } // Reset

    // ----------------------------------------------------------------------
    void RtfTextBuilder::DoVisitText( RtfText* text )
    {
        buffer += text->Text();
    } // DoVisitText

} // namespace Itenso.Rtf.Interpreter
// -- EOF -------------------------------------------------------------------
