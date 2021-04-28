// -- FILE ------------------------------------------------------------------
// name       : IRtfDocumentProperty.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.23
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
#ifndef __RTFDOCUMENTPROPERTY_H
#define __RTFDOCUMENTPROPERTY_H

#include <qstring.h>
#include "ObjectBase.h"

namespace RTF2HTML
{
    namespace RtfPropertyKind
    {
        enum RtfPropertyKind
        {
            Unknown,
            IntegerNumber,
            RealNumber,
            Date,
            Boolean,
            Text
        }; // enum RtfPropertyKind
    }

    // ------------------------------------------------------------------------
    class RtfDocumentProperty: public ObjectBase
    {
        public:

            RtfDocumentProperty( int propertyKindCode, QString name, QString staticValue, QObject* parent = 0 );
            RtfDocumentProperty( int propertyKindCode, QString name, QString staticValue, QString linkValue, QObject* parent = 0 );

            void Init (int propertyKindCode, QString name, QString staticValue, QString linkValue);
            // ----------------------------------------------------------------------
            virtual bool Equals (RTF2HTML::ObjectBase* obj);
            virtual QString& ToString();
            virtual bool IsEqual (RTF2HTML::ObjectBase* obj );

            // ----------------------------------------------------------------------
            int PropertyKindCode ();

            // ----------------------------------------------------------------------
            RtfPropertyKind::RtfPropertyKind PropertyKind ();

            // ----------------------------------------------------------------------
            QString& Name ();

            // ----------------------------------------------------------------------
            QString& StaticValue ();

            // ----------------------------------------------------------------------
            QString& LinkValue ();

        private:
            int propertyKindCode;
            RtfPropertyKind::RtfPropertyKind propertyKind;
            QString name;
            QString staticValue;
            QString linkValue;

            QString toStr;

            // ----------------------------------------------------------------------
    }; // interface IRtfDocumentProperty

} // namespace Itenso.Rtf
// -- EOF -------------------------------------------------------------------
#endif //__RTFDOCUMENTPROPERTY_H