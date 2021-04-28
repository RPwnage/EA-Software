// -- FILE ------------------------------------------------------------------
// name       : RtfElement.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.19
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using Itenso.Sys;

#include <stddef.h>
#include <assert.h>

#include "ObjectBase.h"
#include "rtfElement.h"

using namespace RTF2HTML;

namespace RTF2HTML
{
    // ------------------------------------------------------------------------
    RtfElement::RtfElement(QObject* parent)
        : ObjectBase (parent)
    {
        objName = "RTFElement";
    }
    // ------------------------------------------------------------------------
    RtfElement::RtfElement( RtfElementKind::RtfElementKind kind, QObject* parent )
        : ObjectBase (parent)
        , mKind(kind)
    {
        objName = "RTFElement";
    } // RtfElement
    // ------------------------------------------------------------------------
    RtfElement::~RtfElement()
    {
    }

    // ----------------------------------------------------------------------
    void RtfElement::Visit( RtfElementVisitor* visitor )
    {
        if ( visitor == NULL )
        {
            assert (0);
        }
        DoVisit( visitor );
    } // Visit

    // ----------------------------------------------------------------------
    bool RtfElement::Equals( RTF2HTML::ObjectBase*  obj )
    {
        RtfElement* element = static_cast<RtfElement*>(obj);

        if ( this == element )
        {
            return true;
        }

        if ( obj == NULL || (GetType() != element->GetType()))
        {
            return false;
        }

        return IsEqual( obj );
    } // Equals
    // ----------------------------------------------------------------------
    QString& RtfElement::GetType ()
    {
        return RtfElement::objName;
    }

    QString& RtfElement::ToString()
    {
        return GetType();
    }

    // ----------------------------------------------------------------------
    void RtfElement::DoVisit( RtfElementVisitor* visitor )
    {
        Q_UNUSED (visitor);
    }

    // ----------------------------------------------------------------------
    bool RtfElement::IsEqual (RTF2HTML::ObjectBase* obj )
    {
        Q_UNUSED (obj);
        return true;
    }


} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
