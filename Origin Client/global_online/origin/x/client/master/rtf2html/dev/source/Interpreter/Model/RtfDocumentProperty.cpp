// -- FILE ------------------------------------------------------------------
// name       : RtfDocumentProperty.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.23
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using System.Text;
//using Itenso.Sys;

#include <assert.h>
#include <qtextstream.h>
#include "RtfDocumentProperty.h"
#include "RtfSpec.h"

namespace RTF2HTML
{
    // ----------------------------------------------------------------------
    RtfDocumentProperty::RtfDocumentProperty ( int propertyKindCode, QString name, QString staticValue, QObject* parent )
        : ObjectBase (parent)
    {
        Init(propertyKindCode, name, staticValue, "");
    } // RtfDocumentProperty

    // ----------------------------------------------------------------------
    RtfDocumentProperty::RtfDocumentProperty( int propertyKindCode, QString name, QString staticValue, QString linkValue, QObject* parent )
        : ObjectBase (parent)
    {
        Init (propertyKindCode, name, staticValue, linkValue);
    } // RtfDocumentProperty

    void RtfDocumentProperty::Init (int propertyKindCode, QString name, QString staticValue, QString linkValue)
    {
        if ( name.isEmpty() )
        {
            assert (0);
            //throw new ArgumentNullException( "name" );
        }
        if ( staticValue.isEmpty() )
        {
            assert (0);
            //throw new ArgumentNullException( "staticValue" );
        }
        this->propertyKindCode = propertyKindCode;
        switch ( propertyKindCode )
        {
            case RtfSpec::PropertyTypeInteger:
                propertyKind = RtfPropertyKind::IntegerNumber;
                break;
            case RtfSpec::PropertyTypeRealNumber:
                propertyKind = RtfPropertyKind::RealNumber;
                break;
            case RtfSpec::PropertyTypeDate:
                propertyKind = RtfPropertyKind::Date;
                break;
            case RtfSpec::PropertyTypeBoolean:
                propertyKind = RtfPropertyKind::Boolean;
                break;
            case RtfSpec::PropertyTypeText:
                propertyKind = RtfPropertyKind::Text;
                break;
            default:
                propertyKind = RtfPropertyKind::Unknown;
                break;
        }
        this->name = name;
        this->staticValue = staticValue;
        this->linkValue = linkValue;
    }

    // ----------------------------------------------------------------------
    int RtfDocumentProperty::PropertyKindCode ()
    {
        return propertyKindCode;
    } // PropertyKindCode

    // ----------------------------------------------------------------------
    RtfPropertyKind::RtfPropertyKind RtfDocumentProperty::PropertyKind ()
    {
        return propertyKind;
    } // PropertyKind

    // ----------------------------------------------------------------------
    QString& RtfDocumentProperty::Name ()
    {
        return name;
    } // Name

    // ----------------------------------------------------------------------
    QString& RtfDocumentProperty::StaticValue ()
    {
        return staticValue;
    } // StaticValue

    // ----------------------------------------------------------------------
    QString& RtfDocumentProperty::LinkValue ()
    {
        return linkValue;
    } // LinkValue

    // ----------------------------------------------------------------------
    bool RtfDocumentProperty::Equals( ObjectBase* obj )
    {
        RtfDocumentProperty* dp = static_cast<RtfDocumentProperty*>(obj);

        if ( dp == this )
        {
            return true;
        }

        if ( obj == NULL || GetType() != obj->GetType() )
        {
            return false;
        }

        return IsEqual( obj );
    } // Equals

    // ----------------------------------------------------------------------
    bool RtfDocumentProperty::IsEqual( ObjectBase* obj )
    {
        RtfDocumentProperty* dp = static_cast<RtfDocumentProperty*>(obj);
        return (dp != NULL &&
                propertyKindCode == dp->propertyKindCode &&
                propertyKind == dp->propertyKind &&
                name == dp->name &&
                staticValue == dp->staticValue &&
                linkValue == dp->linkValue);
    } // IsEqual

#if 0 //UNIMPLEMENTED currently NOT USED
    // ----------------------------------------------------------------------
    public override int GetHashCode()
    {
        return HashTool.AddHashCode( GetType().GetHashCode(), ComputeHashCode() );
    } // GetHashCode

    // ----------------------------------------------------------------------
    private int ComputeHashCode()
    {
        int hash = propertyKindCode;
        hash = HashTool.AddHashCode( hash, propertyKind );
        hash = HashTool.AddHashCode( hash, name );
        hash = HashTool.AddHashCode( hash, staticValue );
        hash = HashTool.AddHashCode( hash, linkValue );
        return hash;
    } // ComputeHashCode
#endif
    // ----------------------------------------------------------------------
    QString& RtfDocumentProperty::ToString()
    {
        QTextStream toStream (&toStr);

        toStream << name;

        if ( !staticValue.isEmpty() )
        {
            toStream << "=";
            toStream << staticValue;
        }
        if ( !linkValue.isEmpty() )
        {
            toStream << "@";
            toStream << linkValue;
        }
        return toStr;
    } // ToString

} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
