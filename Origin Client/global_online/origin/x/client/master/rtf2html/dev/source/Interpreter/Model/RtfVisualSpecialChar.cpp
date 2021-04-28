// -- FILE ------------------------------------------------------------------
// name       : RtfVisualSpecialChar.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.22
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using Itenso.Sys;

#include "RtfVisualSpecialChar.h"

namespace RTF2HTML
{
    // ----------------------------------------------------------------------
    RtfVisualSpecialChar::RtfVisualSpecialChar( RtfVisualSpecialCharKind::RtfVisualSpecialCharKind charKind, QObject* parent )
        :RtfVisual(RtfVisualKind::Special, parent)
    {
        this->charKind = charKind;

        QString charKindStr [] =
        {
            "Tabulator",
            "ParagraphNumberBegin",
            "ParagraphNumberEnd",
            "NonBreakingSpace",
            "EmDash",
            "EnDash",
            "EmSpace",
            "EnSpace",
            "QmSpace",
            "Bullet",
            "LeftSingleQuote",
            "RightSingleQuote",
            "LeftDoubleQuote",
            "RightDoubleQuote",
            "OptionalHyphen",
            "NonBreakingHyphen"
        };

        toStr = charKindStr [(unsigned int)charKind];

    } // RtfVisualSpecialChar

    // ----------------------------------------------------------------------
    void RtfVisualSpecialChar::DoVisit( RtfVisualVisitor* visitor )
    {
        visitor->VisitSpecial( this );
    } // DoVisit

    // ----------------------------------------------------------------------
    RtfVisualSpecialCharKind::RtfVisualSpecialCharKind RtfVisualSpecialChar::CharKind ()
    {
        return charKind;
    } // CharKind

    // ----------------------------------------------------------------------
    bool RtfVisualSpecialChar::IsEqual( ObjectBase* obj )
    {
        RtfVisualSpecialChar* vsc = static_cast<RtfVisualSpecialChar*>(obj);
        return (
                   vsc != NULL &&
                   RtfVisual::IsEqual( vsc ) &&
                   charKind == vsc->charKind);
    } // IsEqual

#if 0 //UNIMPLEMENTED currently NOT USED
    // ----------------------------------------------------------------------
    protected override int ComputeHashCode()
    {
        return HashTool.AddHashCode( base.ComputeHashCode(), charKind );
    } // ComputeHashCode
#endif
    // ----------------------------------------------------------------------
    QString& RtfVisualSpecialChar::ToString()
    {
        return toStr;
    } // ToString

} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
