// -- FILE ------------------------------------------------------------------
// name       : RtfFontTableBuilder.cs
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
#include "RtfFontTableBuilder.h"
#include "RtfFontBuilder.h"
#include "RtfGroup.h"
#include "RtfSpec.h"

namespace RTF2HTML
{
    // ----------------------------------------------------------------------
    RtfFontTableBuilder::RtfFontTableBuilder( RtfFontCollection* fontTable, bool ignoreDuplicatedFonts, QObject* parent )
        : RtfElementVisitorBase( RtfElementVisitorOrder::NonRecursive, parent )
    {
        fontBuilder = new RtfFontBuilder(this);
        // we iterate over our children ourselves -> hence non-recursive
        if ( fontTable == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "fontTable" );
        }

        this->fontTable = fontTable;
        this->ignoreDuplicatedFonts = ignoreDuplicatedFonts;
    } // RtfFontTableBuilder

    // ----------------------------------------------------------------------
    bool RtfFontTableBuilder::IgnoreDuplicatedFonts()
    {
        return ignoreDuplicatedFonts;
    } // IgnoreDuplicatedFonts

    // ----------------------------------------------------------------------
    void RtfFontTableBuilder::Reset()
    {
        fontTable->Clear();
    } // Reset

    // ----------------------------------------------------------------------
    void RtfFontTableBuilder::DoVisitGroup( RtfGroup* group )
    {
        QString dest = group->Destination();

        if (dest == RtfSpec::TagFont ||
                dest == RtfSpec::TagThemeFontLoMajor ||
                dest == RtfSpec::TagThemeFontHiMajor ||
                dest == RtfSpec::TagThemeFontDbMajor ||
                dest == RtfSpec::TagThemeFontBiMajor ||
                dest == RtfSpec::TagThemeFontLoMinor ||
                dest == RtfSpec::TagThemeFontHiMinor ||
                dest == RtfSpec::TagThemeFontDbMinor ||
                dest == RtfSpec::TagThemeFontBiMinor)
        {
            BuildFontFromGroup( group );
        }
        else if (dest == RtfSpec::TagFontTable)
        {
            if ( group->Contents().Count() > 1 )
            {
                if ( group->Contents().Get( 1 )->Kind() == RtfElementKind::Group )
                {
                    // the 'new' style where each font resides in a group of its own
                    VisitGroupChildren( group );
                }
                else
                {
                    // the 'old' style where individual fonts are 'terminated' by their
                    // respective name content text (which ends with ';')
                    // -> need to manually iterate from here
                    int childCount = group->Contents().Count();
                    fontBuilder->Reset();
                    for ( int i = 1; i < childCount; i++ ) // skip over the initial \fonttbl tag
                    {
                        group->Contents().Get(i)->Visit( fontBuilder );
                        if ( !fontBuilder->FontName().isEmpty() )
                        {
                            // fonts are 'terminated' by their name (as content text)
                            AddCurrentFont();
                            fontBuilder->Reset();
                        }
                    }
                    //BuildFontFromGroup( group ); // a single font info
                }
            }
        }
    } // DoVisitGroup

    // ----------------------------------------------------------------------
    void RtfFontTableBuilder::BuildFontFromGroup( RtfGroup* group )
    {
        fontBuilder->Reset();
        fontBuilder->VisitGroup( group );
        AddCurrentFont();
    } // BuildFontFromGroup

    // ----------------------------------------------------------------------
    void RtfFontTableBuilder::AddCurrentFont()
    {
        if ( !fontTable->ContainsFontWithId( fontBuilder->FontId() ) )
        {
            fontTable->Add( fontBuilder->CreateFont() );
        }
        else if ( !IgnoreDuplicatedFonts() )
        {
            assert (0);
            //throw new RtfFontTableFormatException( Strings.DuplicateFont( fontBuilder.FontId ) );
        }
    } // AddCurrentFont

} // namespace Itenso.Rtf.Interpreter
// -- EOF -------------------------------------------------------------------
