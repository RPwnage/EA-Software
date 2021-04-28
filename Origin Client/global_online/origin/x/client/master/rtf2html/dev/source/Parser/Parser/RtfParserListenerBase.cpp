// -- FILE ------------------------------------------------------------------
// name       : RtfParserListenerBase.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.19
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------

#include <assert.h>
#include "RtfParserListenerBase.h"

namespace RTF2HTML
{
    // ------------------------------------------------------------------------
    // RtfParserListenerBase
    // ------------------------------------------------------------------------
    RtfParserListenerBase::RtfParserListenerBase (QObject* parent)
        : RtfParserListener (parent)
    {
    }

    RtfParserListenerBase::~RtfParserListenerBase ()
    {
    }
    // ----------------------------------------------------------------------
    int RtfParserListenerBase::Level ()
    {
        return level;
    } // Level

    // ----------------------------------------------------------------------
    void RtfParserListenerBase::ParseBegin()
    {
        level = 0; // in case something interrupted the normal flow of things previously ...
        DoParseBegin();
    } // ParseBegin

    // ----------------------------------------------------------------------
    void RtfParserListenerBase::DoParseBegin()
    {
    } // DoParseBegin

    // ----------------------------------------------------------------------
    void RtfParserListenerBase::GroupBegin()
    {
        DoGroupBegin();
        level++;
    } // GroupBegin

    // ----------------------------------------------------------------------
    void RtfParserListenerBase::DoGroupBegin()
    {
    } // DoGroupBegin

    // ----------------------------------------------------------------------
    void RtfParserListenerBase::TagFound( RtfTag* tag )
    {
        if ( tag != NULL )
        {
            DoTagFound( tag );
        }
    } // TagFound

    // ----------------------------------------------------------------------
    void RtfParserListenerBase::DoTagFound( RtfTag* tag )
    {
        Q_UNUSED (tag);
    } // DoTagFound

    // ----------------------------------------------------------------------
    void RtfParserListenerBase::TextFound( RtfText* text )
    {
        if ( text != NULL )
        {
            DoTextFound( text );
        }
    } // TextFound

    // ----------------------------------------------------------------------
    void RtfParserListenerBase::DoTextFound( RtfText* text )
    {
        Q_UNUSED (text);
    } // DoTextFound

    // ----------------------------------------------------------------------
    void RtfParserListenerBase::GroupEnd()
    {
        level--;
        DoGroupEnd();
    } // GroupEnd

    // ----------------------------------------------------------------------
    void RtfParserListenerBase::DoGroupEnd()
    {
    } // DoGroupEnd

    // ----------------------------------------------------------------------
    void RtfParserListenerBase::ParseSuccess()
    {
        DoParseSuccess();
    } // ParseSuccess

    // ----------------------------------------------------------------------
    void RtfParserListenerBase::DoParseSuccess()
    {
    } // DoParseSuccess

    // ----------------------------------------------------------------------
    void RtfParserListenerBase::ParseFail( RtfException* reason )
    {
        DoParseFail( reason );
    } // ParseFail

    // ----------------------------------------------------------------------
    void RtfParserListenerBase::DoParseFail( RtfException* reason )
    {
        Q_UNUSED (reason);
    } // DoParseFail

    // ----------------------------------------------------------------------
    void RtfParserListenerBase::ParseEnd()
    {
        DoParseEnd();
    } // ParseEnd

    // ----------------------------------------------------------------------
    void RtfParserListenerBase::DoParseEnd()
    {
    } // DoParseEnd

} // namespace Itenso.Rtf.Parser
// -- EOF -------------------------------------------------------------------
