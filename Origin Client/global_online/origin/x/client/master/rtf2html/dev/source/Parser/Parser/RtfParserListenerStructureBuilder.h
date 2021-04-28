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

#ifndef __RTFPARSERLISTENERSTRUCTUREBUILDER_H
#define __RTFPARSERLISTENERSTRUCTUREBUILDER_H

#include <stack>
#include "RtfParserListenerBase.h"
#include "RtfGroup.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfParserListenerStructureBuilder : public RtfParserListenerBase
    {
        public:
            RtfParserListenerStructureBuilder(QObject* parent = 0);
            virtual ~RtfParserListenerStructureBuilder();

            // ----------------------------------------------------------------------
            RtfGroup* StructureRoot ();

        protected:
            // ----------------------------------------------------------------------
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
            virtual void DoParseEnd();

        private:
            void clearOpenGroupStack();
            // ----------------------------------------------------------------------
            // members
            std::stack<RtfGroup*> openGroupStack;
            RtfGroup* curGroup;
            RtfGroup* structureRoot;
    }; // class RtfParserListenerStructureBuilder

} // namespace Itenso.Rtf.Parser
#endif //__RTFPARSERLISTENERSTRUCTUREBUILDER_H
// -- EOF -------------------------------------------------------------------
