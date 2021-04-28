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

#include <assert.h>
#include "RtfUserPropertyBuilder.h"
#include "RtfGroup.h"
#include "RtfTag.h"
#include "RtfSpec.h"

namespace RTF2HTML
{

    // ----------------------------------------------------------------------
    RtfUserPropertyBuilder::RtfUserPropertyBuilder( RtfDocumentPropertyCollection* collectedProperties, QObject* parent )
        : RtfElementVisitorBase ( RtfElementVisitorOrder::NonRecursive, parent )
    {
        // we iterate over our children ourselves -> hence non-recursive
        if ( collectedProperties == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "collectedProperties" );
        }
        this->collectedProperties = collectedProperties;
        textBuilder = new RtfTextBuilder(this);
    } // RtfUserPropertyBuilder

    RtfUserPropertyBuilder::~RtfUserPropertyBuilder()
    {
    }

    // ----------------------------------------------------------------------
    RtfDocumentProperty* RtfUserPropertyBuilder::CreateProperty()
    {
        return new RtfDocumentProperty( propertyTypeCode, propertyName, staticValue, linkValue, this );
    } // CreateProperty

    // ----------------------------------------------------------------------
    void RtfUserPropertyBuilder::Reset()
    {
        propertyTypeCode = 0;
        propertyName.clear();
        staticValue.clear();
        linkValue.clear();
    } // Reset

    // ----------------------------------------------------------------------
    void RtfUserPropertyBuilder::DoVisitGroup( RtfGroup* group )
    {
        QString dest = group->Destination();

        if (dest == RtfSpec::TagUserProperties)
        {
            VisitGroupChildren( group );
        }
        else if (dest.isEmpty())
        {
            Reset();
            VisitGroupChildren( group );
            collectedProperties->Add( CreateProperty() );
        }
        else if (dest == RtfSpec::TagUserPropertyName)
        {
            textBuilder->Reset();
            textBuilder->VisitGroup( group );
            propertyName = textBuilder->CombinedText();
        }
        else if (dest == RtfSpec::TagUserPropertyValue)
        {
            textBuilder->Reset();
            textBuilder->VisitGroup( group );
            staticValue = textBuilder->CombinedText();
        }
        else if (dest == RtfSpec::TagUserPropertyLink)
        {
            textBuilder->Reset();
            textBuilder->VisitGroup( group );
            linkValue = textBuilder->CombinedText();
        }
    } // DoVisitGroup

    // ----------------------------------------------------------------------
    void RtfUserPropertyBuilder::DoVisitTag( RtfTag* tag )
    {
        if (tag->Name() == RtfSpec::TagUserPropertyType)
        {
            propertyTypeCode = tag->ValueAsNumber();
        }
    } // DoVisitTag

} // namespace Itenso.Rtf.Interpreter
// -- EOF -------------------------------------------------------------------
