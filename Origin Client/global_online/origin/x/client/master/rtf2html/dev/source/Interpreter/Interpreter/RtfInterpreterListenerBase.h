// -- FILE ------------------------------------------------------------------
// name       : RtfInterpreterListenerBase.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.21
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
#ifndef __RTFINTERPRETERLISTENERBASE_H
#define __RTFINTERPRETERLISTENERBASE_H

#include <qstring.h>
#include "RtfInterpreterListener.h"

namespace RTF2HTML
{

    // -----------------------------------------------------------------------
    class RtfInterpreterListenerBase: public RtfInterpreterListener
    {
        public:

            RtfInterpreterListenerBase (QObject* parent = 0);
            virtual ~RtfInterpreterListenerBase ();

            // ----------------------------------------------------------------------
            void BeginDocument( RtfInterpreterContext* context );

            // ----------------------------------------------------------------------
            void InsertText( RtfInterpreterContext* context, QString& text );

            // ----------------------------------------------------------------------
            void InsertSpecialChar( RtfInterpreterContext* context, RtfVisualSpecialCharKind::RtfVisualSpecialCharKind kind );

            // ----------------------------------------------------------------------
            void InsertBreak( RtfInterpreterContext* context, RtfVisualBreakKind::RtfVisualBreakKind kind );

#if 0 //UNIMPLEMENTED currently NOT USED
            // ----------------------------------------------------------------------
            public void InsertImage( IRtfInterpreterContext context, RtfVisualImageFormat format,
                                     int width, int height, int desiredWidth, int desiredHeight,
                                     int scaleWidthPercent, int scaleHeightPercent, string imageDataHex
                                   )
#endif

            // ----------------------------------------------------------------------
            void EndDocument( RtfInterpreterContext* context );

            // ----------------------------------------------------------------------
            virtual void DoBeginDocument( RtfInterpreterContext* context );

            // ----------------------------------------------------------------------
            virtual void DoInsertText( RtfInterpreterContext* context, QString& text );

            // ----------------------------------------------------------------------
            virtual void DoInsertSpecialChar( RtfInterpreterContext* context, RtfVisualSpecialCharKind::RtfVisualSpecialCharKind kind );

            // ----------------------------------------------------------------------
            virtual void DoInsertBreak( RtfInterpreterContext* context, RtfVisualBreakKind::RtfVisualBreakKind kind );

#if 0 //UNIMPLEMENTED currently NOT USED
            // ----------------------------------------------------------------------
            virtual void DoInsertImage( IRtfInterpreterContext context,
                                        RtfVisualImageFormat format,
                                        int width, int height, int desiredWidth, int desiredHeight,
                                        int scaleWidthPercent, int scaleHeightPercent,
                                        string imageDataHex
                                      )
#endif

            // ----------------------------------------------------------------------
            virtual void DoEndDocument( RtfInterpreterContext* context );

    }; // class RtfInterpreterListenerBase

} // namespace Itenso.Rtf.Interpreter
// -- EOF -------------------------------------------------------------------
#endif //__RTFINTERPRETERLISTENERBASE_H