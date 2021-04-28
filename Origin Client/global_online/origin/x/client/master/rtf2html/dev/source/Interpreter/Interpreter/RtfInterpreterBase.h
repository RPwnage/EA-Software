// -- FILE ------------------------------------------------------------------
// name       : RtfInterpreterBase.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.21
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using System.Collections;
#ifndef __RTFINTERPRETERBASE_H
#define __RTFINTERPRETERBASE_H

#include "RtfInterpreterListener.h"
#include "RtfGroup.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfInterpreterBase: public QObject
    {
        public:
            // ----------------------------------------------------------------------
            RtfInterpreterBase( RtfInterpreterListenerList& listeners, QObject* parent = 0);
            virtual ~RtfInterpreterBase();

#if USE_SETTINGS
            // ----------------------------------------------------------------------
            protected RtfInterpreterBase( IRtfInterpreterSettings settings, params IRtfInterpreterListener[] listeners )
#endif
            // ----------------------------------------------------------------------
#if USE_SETTINGS
            public IRtfInterpreterSettings Settings
            {
                get { return settings; }
            } // Settings
#endif
            // ----------------------------------------------------------------------
            void AddInterpreterListener( RtfInterpreterListener* listener );

            // ----------------------------------------------------------------------
            void RemoveInterpreterListener( RtfInterpreterListener* listener );

            // ----------------------------------------------------------------------
            void Interpret( RtfGroup* rtfDocument );

            // ----------------------------------------------------------------------
            virtual void DoInterpret( RtfGroup* rtfDocument );

            // ----------------------------------------------------------------------
            void NotifyBeginDocument();

            // ----------------------------------------------------------------------
            void NotifyInsertText( QString& text );

            // ----------------------------------------------------------------------
            void NotifyInsertSpecialChar( RtfVisualSpecialCharKind::RtfVisualSpecialCharKind kind );

            // ----------------------------------------------------------------------
            void NotifyInsertBreak( RtfVisualBreakKind::RtfVisualBreakKind kind );

            // ----------------------------------------------------------------------
#if 0 //UNIMPLEMENTED currently NOT USED
            protected void NotifyInsertImage( RtfVisualImageFormat format,
                                              int width, int height, int desiredWidth, int desiredHeight,
                                              int scaleWidthPercent, int scaleHeightPercent, string imageDataHex
                                            )
#endif

            // ----------------------------------------------------------------------
            void NotifyEndDocument();
            // ----------------------------------------------------------------------
            RtfInterpreterContext* Context ();

        private:
            // ----------------------------------------------------------------------
            // members
            RtfInterpreterContext* context;
#if USE_SETTINGS
            private readonly IRtfInterpreterSettings settings;
#endif
            RtfInterpreterListenerVector mListeners;


    }; // class RtfInterpreterBase

} // namespace Itenso.Rtf.Interpreter
// -- EOF -------------------------------------------------------------------
#endif //__RTFINTERPRETERBASE_H