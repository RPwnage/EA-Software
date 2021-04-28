// -- FILE ------------------------------------------------------------------
// name       : RtfVisualVisitorBase.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.26
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------

#include "RtfVisualVisitorBase.h"
#include "RtfVisualText.h"
#include "RtfVisualBreak.h"
#include "RtfVisualSpecialChar.h"

namespace RTF2HTML
{
    RtfVisualVisitorBase::RtfVisualVisitorBase (QObject* parent)
        : RtfVisualVisitor (parent)
    {
    }

    RtfVisualVisitorBase::~RtfVisualVisitorBase()
    {
    }

    // ----------------------------------------------------------------------
    void RtfVisualVisitorBase::VisitText( RtfVisualText* visualText )
    {
        if ( visualText != NULL )
        {
            DoVisitText( visualText );
        }
    } // VisitText

    // ----------------------------------------------------------------------
    void RtfVisualVisitorBase::DoVisitText( RtfVisualText* visualText )
    {
        Q_UNUSED (visualText);
    } // DoVisitText

    // ----------------------------------------------------------------------
    void RtfVisualVisitorBase::VisitBreak( RtfVisualBreak* visualBreak )
    {
        if ( visualBreak != NULL )
        {
            DoVisitBreak( visualBreak );
        }
    } // VisitBreak

    // ----------------------------------------------------------------------
    void RtfVisualVisitorBase::DoVisitBreak( RtfVisualBreak* visualBreak )
    {
        Q_UNUSED (visualBreak);
    } // DoVisitBreak

    // ----------------------------------------------------------------------
    void RtfVisualVisitorBase::VisitSpecial( RtfVisualSpecialChar* visualSpecialChar )
    {
        if ( visualSpecialChar != NULL )
        {
            DoVisitSpecial( visualSpecialChar );
        }
    } // VisitSpecial

    // ----------------------------------------------------------------------
    void RtfVisualVisitorBase::DoVisitSpecial( RtfVisualSpecialChar* visualSpecialChar )
    {
        Q_UNUSED (visualSpecialChar);
    } // DoVisitSpecial

#if 0 //UNIMPLEMENTED currently NOT USED
    // ----------------------------------------------------------------------
    public void VisitImage( IRtfVisualImage visualImage )
    {
        if ( visualImage != null )
        {
            DoVisitImage( visualImage );
        }
    } // VisitImage

    // ----------------------------------------------------------------------
    protected virtual void DoVisitImage( IRtfVisualImage visualImage )
    {
    } // DoVisitImage
#endif

} // namespace Itenso.Rtf.Support
// -- EOF -------------------------------------------------------------------
