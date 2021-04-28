// -- FILE ------------------------------------------------------------------
// name       : RtfParserListenerBase.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.19
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------

#ifndef __RTFPARSERLISTENERBASE_H
#define __RTFPARSERLISTENERBASE_H

#include "RtfParserListener.h"

namespace RTF2HTML
{
    // ------------------------------------------------------------------------
    class RtfParserListenerBase : public RtfParserListener
    {
        public:
            RtfParserListenerBase (QObject* parent = 0);
            virtual ~RtfParserListenerBase();

            // ----------------------------------------------------------------------
            int Level ();

            // ----------------------------------------------------------------------
            virtual void ParseBegin();
            // ----------------------------------------------------------------------
            virtual void GroupBegin();
            // ----------------------------------------------------------------------
            virtual void TagFound( RtfTag* tag );
            // ----------------------------------------------------------------------
            virtual void TextFound( RtfText* text );
            // ----------------------------------------------------------------------
            virtual void GroupEnd();
            // ----------------------------------------------------------------------
            virtual void ParseSuccess();
            // ----------------------------------------------------------------------
            virtual void ParseFail( RtfException* reason );
            // ----------------------------------------------------------------------
            virtual void ParseEnd();
            // ----------------------------------------------------------------------

            // ----------------------------------------------------------------------
        protected:
            virtual void DoParseBegin();
            // ----------------------------------------------------------------------
            virtual void DoGroupBegin();
            // ----------------------------------------------------------------------
            virtual void DoTagFound( RtfTag* tag );
            // ----------------------------------------------------------------------
            virtual void DoTextFound( RtfText* text );
            // ----------------------------------------------------------------------
            virtual void DoGroupEnd();
            // ----------------------------------------------------------------------
            virtual void DoParseSuccess();
            // ----------------------------------------------------------------------
            virtual void DoParseFail( RtfException* reason );
            // ----------------------------------------------------------------------
            virtual void DoParseEnd();

        private:
            // ----------------------------------------------------------------------
            // members
            int level;

    }; // class RtfParserListenerBase
} // namespace Itenso.Rtf.Parser
#endif //__RTFPARSERLISTENERBASE_H
// -- EOF -------------------------------------------------------------------
