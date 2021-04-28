// -- FILE ------------------------------------------------------------------
// name       : RtfParserListenerStructureBuilder.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.19
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System.Collections;
//using Itenso.Rtf.Model;

#include <assert.h>
#include "RtfParserListenerStructureBuilder.h"

namespace RTF2HTML
{
    // ------------------------------------------------------------------------
    RtfParserListenerStructureBuilder::RtfParserListenerStructureBuilder(QObject* parent)
        : RtfParserListenerBase (parent)
    {
    }

    RtfParserListenerStructureBuilder::~RtfParserListenerStructureBuilder()
    {
    }

    // ----------------------------------------------------------------------
    RtfGroup* RtfParserListenerStructureBuilder::StructureRoot ()
    {
        return structureRoot;
    } // StructureRoot

    // ----------------------------------------------------------------------
    void RtfParserListenerStructureBuilder::DoParseBegin()
    {
        //openGroupStack.Clear();
        clearOpenGroupStack();
        curGroup = NULL;
        structureRoot = NULL;
    } // DoParseBegin

    // ----------------------------------------------------------------------
    void RtfParserListenerStructureBuilder::DoGroupBegin()
    {
        RtfGroup* newGroup = new RtfGroup(this);
        if ( curGroup != NULL )
        {
            openGroupStack.push( curGroup );
            curGroup->WritableContents().Add( newGroup );
        }
        curGroup = newGroup;
    } // DoGroupBegin

    // ----------------------------------------------------------------------
    void RtfParserListenerStructureBuilder::DoTagFound( RtfTag* tag )
    {
        if ( curGroup == NULL )
        {
            assert (0);
        }
        curGroup->WritableContents().Add( tag );
    } // DoTagFound

    // ----------------------------------------------------------------------
    void RtfParserListenerStructureBuilder::DoTextFound( RtfText* text )
    {
        if ( curGroup == NULL )
        {
            assert (0);
        }
        curGroup->WritableContents().Add( text );
    } // DoTextFound

    // ----------------------------------------------------------------------
    void RtfParserListenerStructureBuilder::DoGroupEnd()
    {
        if ( openGroupStack.size() > 0 )
        {
            curGroup = openGroupStack.top();
            openGroupStack.pop();
        }
        else
        {
            if ( structureRoot != NULL )
            {
                assert (0);
            }
            structureRoot = curGroup;
            curGroup = NULL;
        }
    } // DoGroupEnd

    // ----------------------------------------------------------------------
    void RtfParserListenerStructureBuilder::DoParseEnd()
    {
        if ( openGroupStack.size() > 0 )
        {
            assert (0);
            //throw new RtfBraceNestingException( Strings.UnclosedGroups );
        }
    } // DoParseEnd

    // ----------------------------------------------------------------------
    void RtfParserListenerStructureBuilder::clearOpenGroupStack()
    {
        while (openGroupStack.size() > 0)
        {
            openGroupStack.pop();
        }
    }

} // namespace Itenso.Rtf.Parser
// -- EOF -------------------------------------------------------------------
