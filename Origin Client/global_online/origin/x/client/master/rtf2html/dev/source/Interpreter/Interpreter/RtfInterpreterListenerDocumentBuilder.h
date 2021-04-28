// -- FILE ------------------------------------------------------------------
// name       : RtfInterpreterListenerDocumentBuilder.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.21
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System.Text;
//using Itenso.Rtf.Model;

#ifndef __RTFINTERPRETERLISTENERDOCUMENTBUILDER_H
#define __RTFINTERPRETERLISTENERDOCUMENTBUILDER_H

#include "RtfInterpreterListenerBase.h"
#include "RtfDocument.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfInterpreterListenerDocumentBuilder : public RtfInterpreterListenerBase
    {
        public:

            RtfInterpreterListenerDocumentBuilder(QObject* parent = 0);
            virtual ~RtfInterpreterListenerDocumentBuilder();

            // ----------------------------------------------------------------------
            bool getCombineTextWithSameFormat ();
            void setCombineTextWithSameFormat (bool value);

            // ----------------------------------------------------------------------
            RtfDocument* Document ();

            // ----------------------------------------------------------------------
            virtual void DoBeginDocument( RtfInterpreterContext* context );
            // ----------------------------------------------------------------------
            virtual void DoInsertText( RtfInterpreterContext* context, QString& text );
            // ----------------------------------------------------------------------
            virtual void DoInsertSpecialChar( RtfInterpreterContext* context, RtfVisualSpecialCharKind::RtfVisualSpecialCharKind kind );
            // ----------------------------------------------------------------------
            virtual void DoInsertBreak( RtfInterpreterContext* context, RtfVisualBreakKind::RtfVisualBreakKind kind );
            // ----------------------------------------------------------------------
#if 0 //UNIMPLEMENTED currently NOT USED
            protected override void DoInsertImage( IRtfInterpreterContext context,
                                                   RtfVisualImageFormat format,
                                                   int width, int height, int desiredWidth, int desiredHeight,
                                                   int scaleWidthPercent, int scaleHeightPercent,
                                                   string imageDataHex
                                                 )
#endif
            // ----------------------------------------------------------------------
            virtual void DoEndDocument( RtfInterpreterContext* context );

            // ----------------------------------------------------------------------
        private:

            void EndParagraph( RtfInterpreterContext* context );

            // ----------------------------------------------------------------------
            void FlushPendingText();

            // ----------------------------------------------------------------------
            void AppendAlignedVisual( RtfVisual* visual );

            // ----------------------------------------------------------------------
            // members
            bool combineTextWithSameFormat;

            RtfDocument* document;
            RtfVisualCollection* visualDocumentContent;
            RtfVisualCollection* pendingParagraphContent;

            RtfTextFormat* pendingTextFormat;

            QString pendingText;
    }; // class RtfInterpreterListenerDocumentBuilder

} // namespace Itenso.Rtf.Interpreter
// -- EOF -------------------------------------------------------------------
#endif //__RTFINTERPRETERLISTENERDOCUMENTBUILDER_H