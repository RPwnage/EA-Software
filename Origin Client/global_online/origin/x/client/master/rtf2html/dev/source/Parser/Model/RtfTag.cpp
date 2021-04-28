// -- FILE ------------------------------------------------------------------
// name       : RtfTag.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.19
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using System.Globalization;
//using Itenso.Sys;

#include <assert.h>
#include <stdexcept>
#include "RtfTag.h"

namespace RTF2HTML
{
    // ----------------------------------------------------------------------
    RtfTag::RtfTag( QString name, QObject* parent )
        : RtfElement( RtfElementKind::Tag, parent )
    {
        if ( name.isEmpty() )
        {
            assert (0);
        }
        fullName = name;
        this->name = name;
        valueAsText.clear();
        valueAsNumber = -1;
    } // RtfTag

    // ----------------------------------------------------------------------
    RtfTag::RtfTag( QString name, QString value, QObject* parent )
        : RtfElement( RtfElementKind::Tag, parent )
    {
        if ( name.isEmpty() )
        {
            assert (0);
        }
        if ( value.isEmpty() )
        {
            assert (0);
        }

        fullName = name + value;
        this->name = name;
        valueAsText = value;
        try
        {
            valueAsNumber = value.toInt();
        }
        catch (const std::invalid_argument& ia)
        {
            Q_UNUSED (ia);
            assert (0);
            valueAsNumber = -1;
        }
    } // RtfTag
    // ----------------------------------------------------------------------
    RtfTag::~RtfTag()
    {
    }
    // ----------------------------------------------------------------------
    QString& RtfTag::FullName ()
    {
        return fullName;
    } // FullName

    // ----------------------------------------------------------------------
    QString& RtfTag::Name ()
    {
        return name;
    } // Name

    // ----------------------------------------------------------------------
    bool RtfTag::HasValue ()
    {
        return !(valueAsText.isEmpty());
    } // HasValue

    // ----------------------------------------------------------------------
    QString& RtfTag::ValueAsText ()
    {
        return valueAsText;
    } // ValueAsText

    // ----------------------------------------------------------------------
    int RtfTag::ValueAsNumber()
    {
        return valueAsNumber;
    } // ValueAsNumber

    // ----------------------------------------------------------------------
    QString& RtfTag::ToString()
    {
        tagStr = "\\" + fullName;
        return tagStr;
    } // ToString

    // ----------------------------------------------------------------------
    void RtfTag::DoVisit( RtfElementVisitor* visitor )
    {
        visitor->VisitTag( this );
    } // DoVisit

    // ----------------------------------------------------------------------
    bool RtfTag::IsEqual( ObjectBase* obj )
    {
        Q_UNUSED (obj);
        //doesn't seem to be called so for now, ignore
        assert (0);
        return true;
        /*
                RtfTag compare = obj as RtfTag; // guaranteed to be non-null
                return compare != null && base.IsEqual( obj ) &&
                    fullName.Equals( compare.fullName );
        */
    } // IsEqual

    /*
        // ----------------------------------------------------------------------
        protected override int ComputeHashCode()
        {
            int hash = base.ComputeHashCode();
            hash = HashTool.AddHashCode( hash, fullName );
            return hash;
        } // ComputeHashCode
    */

} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
