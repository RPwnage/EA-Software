// -- FILE ------------------------------------------------------------------
// name       : RtfVisualBreak.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.22
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using Itenso.Sys;

#include "RtfVisualBreak.h"

namespace RTF2HTML
{
    // ----------------------------------------------------------------------
    RtfVisualBreak::RtfVisualBreak( RtfVisualBreakKind::RtfVisualBreakKind breakKind, QObject* parent )
        : RtfVisual( RtfVisualKind::Break, parent )
    {
        this->breakKind = breakKind;
        setBreakKindString();
    } // RtfVisualBreak

    // ----------------------------------------------------------------------
    RtfVisualBreakKind::RtfVisualBreakKind RtfVisualBreak::BreakKind ()
    {
        return breakKind;
    } // BreakKind

    // ----------------------------------------------------------------------
    QString& RtfVisualBreak::ToString()
    {
        return toStr;
    } // ToString

    // ----------------------------------------------------------------------
    void RtfVisualBreak::setBreakKindString()
    {
        switch (breakKind)
        {
            case RtfVisualBreakKind::Line:
                toStr = "Line";
                break;
            case RtfVisualBreakKind::Page:
                toStr = "Page";
                break;
            case RtfVisualBreakKind::Paragraph:
                toStr = "Paragraph";
                break;
            case RtfVisualBreakKind::Section:
                toStr = "Section";
                break;
        }
    }

    // ----------------------------------------------------------------------
    void RtfVisualBreak::DoVisit( RtfVisualVisitor* visitor )
    {
        visitor->VisitBreak( this );
    } // DoVisit

    // ----------------------------------------------------------------------
    bool RtfVisualBreak::IsEqual( ObjectBase* obj )
    {
        RtfVisualBreak* vb = static_cast<RtfVisualBreak*>(obj);
        return (
                   RtfVisual::IsEqual( obj ) &&
                   breakKind == vb->breakKind);
    } // IsEqual

#if 0 //UNIMPLEMENTED currently NOT USED
    // ----------------------------------------------------------------------
    protected override int ComputeHashCode()
    {
        return HashTool.AddHashCode( base.ComputeHashCode(), breakKind );
    } // ComputeHashCode
#endif
} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
