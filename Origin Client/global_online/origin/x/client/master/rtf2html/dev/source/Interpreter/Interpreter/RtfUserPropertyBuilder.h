// -- FILE ------------------------------------------------------------------
// name       : RtfUserPropertyBuilder.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.23
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using Itenso.Rtf.Model;
//using Itenso.Rtf.Support;

#ifndef __RTFUSERPROPERTYBUILDER_H
#define __RTFUSERPROPERTYBUILDER_H

#include <qstring.h>

#include "RtfElementVisitorBase.h"
#include "RtfDocumentPropertyCollection.h"
#include "RtfTextBuilder.h"


namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfUserPropertyBuilder : public RtfElementVisitorBase
    {
        public:

            // ----------------------------------------------------------------------
            RtfUserPropertyBuilder( RtfDocumentPropertyCollection* collectedProperties, QObject* parent = 0 );
            virtual ~RtfUserPropertyBuilder();

            // ----------------------------------------------------------------------
            RtfDocumentProperty* CreateProperty();

            // ----------------------------------------------------------------------
            void Reset();

            // ----------------------------------------------------------------------
            virtual void DoVisitGroup( RtfGroup* group );

            // ----------------------------------------------------------------------
            virtual void DoVisitTag( RtfTag* tag );

        private:
            // ----------------------------------------------------------------------
            // members
            RtfDocumentPropertyCollection* collectedProperties;
            RtfTextBuilder* textBuilder;
            int propertyTypeCode;
            QString propertyName;
            QString staticValue;
            QString linkValue;

    }; // class RtfUserPropertyBuilder

} // namespace Itenso.Rtf.Interpreter
// -- EOF -------------------------------------------------------------------
#endif //__RTFUSERPROPERTYBUILDER_H