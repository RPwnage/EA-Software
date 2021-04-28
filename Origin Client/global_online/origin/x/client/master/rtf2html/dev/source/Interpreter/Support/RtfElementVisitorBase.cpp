// -- FILE ------------------------------------------------------------------
// name       : RtfElementVisitorBase.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.19
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------

#include <qobject.h>

#include "RtfElementVisitorBase.h"
#include "RtfGroup.h"
#include "RtfTag.h"
#include "RtfText.h"
#include "RtfElementCollection.h"

namespace RTF2HTML
{

    // ----------------------------------------------------------------------
    RtfElementVisitorBase::RtfElementVisitorBase( RtfElementVisitorOrder::RtfElementVisitorOrder order, QObject* parent )
        : RtfElementVisitor (parent)
    {
        this->order = order;
    } // RtfElementVisitorBase

    // ----------------------------------------------------------------------
    void RtfElementVisitorBase::VisitTag( RtfTag* tag )
    {
        if ( tag != NULL )
        {
            DoVisitTag( tag );
        }
    } // VisitTag

    // ----------------------------------------------------------------------
    void RtfElementVisitorBase::DoVisitTag( RtfTag* tag )
    {
        Q_UNUSED (tag);
    } // DoVisitTag

    // ----------------------------------------------------------------------
    void RtfElementVisitorBase::VisitGroup( RtfGroup* group )
    {
        if ( group != NULL)
        {
            if ( order == RtfElementVisitorOrder::DepthFirst )
            {
                VisitGroupChildren( group );
            }
            DoVisitGroup( group );
            if ( order == RtfElementVisitorOrder::BreadthFirst )
            {
                VisitGroupChildren( group );
            }
        }
    } // VisitGroup

    // ----------------------------------------------------------------------
    void RtfElementVisitorBase::DoVisitGroup( RtfGroup* group )
    {
        Q_UNUSED (group);
    } // DoVisitGroup

    // ----------------------------------------------------------------------
    void RtfElementVisitorBase::VisitGroupChildren( RtfGroup* group )
    {
        RtfElementCollection& contents = group->Contents();
        int count = contents.Count();
        for (int i = 0; i < count; i++)
        {
            RtfElement* child = contents.Get(i);
            child->Visit (this);
        }
    } // VisitGroupChildren

    // ----------------------------------------------------------------------
    void RtfElementVisitorBase::VisitText( RtfText* text )
    {
        if ( text != NULL )
        {
            DoVisitText( text );
        }
    } // VisitText

    // ----------------------------------------------------------------------
    void RtfElementVisitorBase::DoVisitText( RtfText* text )
    {
        Q_UNUSED (text);
    } // DoVisitText

} // namespace Itenso.Rtf.Support
// -- EOF -------------------------------------------------------------------
