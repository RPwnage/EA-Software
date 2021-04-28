// -- FILE ------------------------------------------------------------------
// name       : RtfColorTableBuilder.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.21
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using Itenso.Rtf.Model;
//using Itenso.Rtf.Support;
#include <assert.h>

#include "RtfColorTableBuilder.h"
#include "RtfGroup.h"
#include "RtfTag.h"
#include "RtfText.h"
#include "RtfSpec.h"

namespace RTF2HTML
{
    // ----------------------------------------------------------------------
    RtfColorTableBuilder::RtfColorTableBuilder( RtfColorCollection* colorTable, QObject* parent )
        : RtfElementVisitorBase( RtfElementVisitorOrder::NonRecursive, parent )
    {
        // we iterate over our children ourselves -> hence non-recursive
        if ( colorTable == NULL)
        {
            assert (0);
            //throw new ArgumentNullException( "colorTable" );
        }
        this->colorTable = colorTable;
        curRed = 0;
        curGreen = 0;
        curBlue = 0;
    } // RtfColorTableBuilder

    // ----------------------------------------------------------------------
    void RtfColorTableBuilder::Reset()
    {
        colorTable->Clear();
        curRed = 0;
        curGreen = 0;
        curBlue = 0;
    } // Reset

    // ----------------------------------------------------------------------
    void RtfColorTableBuilder::DoVisitGroup( RtfGroup* group )
    {
        if ( RtfSpec::TagColorTable == group->Destination())
        {
            VisitGroupChildren( group );
        }
    } // DoVisitGroup

    // ----------------------------------------------------------------------
    void RtfColorTableBuilder::DoVisitTag( RtfTag* tag )
    {
        QString tagName = tag->Name();
        if ( tagName == RtfSpec::TagColorRed )
        {
            curRed = tag->ValueAsNumber();
        }
        else if ( tagName == RtfSpec::TagColorGreen )
        {
            curGreen = tag->ValueAsNumber();
        }
        else if ( tagName == RtfSpec::TagColorBlue )
        {
            curBlue = tag->ValueAsNumber();
        }
    } // DoVisitTag

    // ----------------------------------------------------------------------
    void RtfColorTableBuilder::DoVisitText( RtfText* text )
    {
        if ( RtfSpec::TagDelimiter == text->Text() )
        {
            colorTable->Add( new RtfColor( curRed, curGreen, curBlue, this ) );
            curRed = 0;
            curGreen = 0;
            curBlue = 0;
        }
        else
        {
            assert (0);
            //throw new RtfColorTableFormatException( Strings.ColorTableUnsupportedText( text.Text ) );
        }
    } // DoVisitText

} // namespace Itenso.Rtf.Interpreter
// -- EOF -------------------------------------------------------------------
