// -- FILE ------------------------------------------------------------------
// name       : RtfInterpreter.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.20
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
#ifndef __RTFINTERPRETER_H
#define __RTFINTERPRETER_H

#include "RtfInterpreterBase.h"
#include "RtfElementVisitor.h"
#include "RtfFontTableBuilder.h"
#include "RtfColorTableBuilder.h"
#include "RtfDocumentInfoBuilder.h"
#include "RtfUserPropertyBuilder.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfInterpreter : public RtfInterpreterBase, public RtfElementVisitor
    {
        public:

            RtfInterpreter( RtfInterpreterListenerList& listeners, QObject* parent = 0 );
            virtual ~RtfInterpreter();

            // ----------------------------------------------------------------------

            // ----------------------------------------------------------------------
            static bool IsSupportedDocument( RtfGroup* rtfDocument );

            // ----------------------------------------------------------------------
            static RtfGroup* GetSupportedDocument( RtfGroup* rtfDocument );

            // ----------------------------------------------------------------------
            virtual void DoInterpret( RtfGroup* rtfDocument );

            // ----------------------------------------------------------------------
            void InterpretContents( RtfGroup* rtfDocument );

            // ----------------------------------------------------------------------
            // RTFElementVisitor
            // ----------------------------------------------------------------------

            virtual void VisitTag( RtfTag* tag );

            virtual void VisitGroup( RtfGroup* group );

            virtual void VisitText( RtfText* text );

        private:
            // ----------------------------------------------------------------------
            void VisitChildrenOf( RtfGroup* group );
            // ----------------------------------------------------------------------
            QString ExtractHyperlink (RtfGroup* group);
            QString ExtractHyperlinkVisual (RtfGroup* group);

            // ----------------------------------------------------------------------
            // members
            RtfFontTableBuilder* fontTableBuilder;
            RtfColorTableBuilder* colorTableBuilder;
            RtfDocumentInfoBuilder* documentInfoBuilder;
            RtfUserPropertyBuilder* userPropertyBuilder;
            QString hyperlinkStr;
            QString hyperlinkVisualText;
//UNUSED
//          private readonly RtfImageBuilder imageBuilder;
            bool lastGroupWasPictureWrapper;

    }; // class RtfInterpreter

} // namespace Itenso.Rtf.Interpreter
// -- EOF -------------------------------------------------------------------
#endif //__RTFINTERPRETER_H