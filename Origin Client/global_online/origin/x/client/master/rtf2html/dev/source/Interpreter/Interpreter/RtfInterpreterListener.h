// -- FILE ------------------------------------------------------------------
// name       : IRtfInterpreterListener.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.21
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
#ifndef __RTFINTERPRETERLISTENER_H
#define __RTFINTERPRETERLISTENER_H

#include <qstring.h>
#include "RtfInterpreterContext.h"
#include "RtfVisualSpecialChar.h"
#include "RtfVisualBreak.h"

namespace RTF2HTML
{

    class RtfInterpreterListener;

    typedef std::vector<RtfInterpreterListener*> RtfInterpreterListenerList;

    class RtfInterpreterListenerVector : public QObject
    {
        public:
            RtfInterpreterListenerVector(QObject* parent = 0);
            ~RtfInterpreterListenerVector();

            void Add (RtfInterpreterListener* listener);
            bool Contains (RtfInterpreterListener* listener);
            void Remove (RtfInterpreterListener* listener);

            RtfInterpreterListenerList* ListenerVector();


        private:
            RtfInterpreterListenerList mListenerVector;
    };

    // ------------------------------------------------------------------------
    class RtfInterpreterListener : public QObject
    {
        public:

            RtfInterpreterListener (QObject* parent = 0);
            virtual ~RtfInterpreterListener ();

            // ----------------------------------------------------------------------
            virtual void BeginDocument( RtfInterpreterContext* context );

            // ----------------------------------------------------------------------
            virtual void InsertText( RtfInterpreterContext* context, QString& text );

            // ----------------------------------------------------------------------
            virtual void InsertSpecialChar( RtfInterpreterContext* context, RtfVisualSpecialCharKind::RtfVisualSpecialCharKind kind );

            // ----------------------------------------------------------------------
            virtual void InsertBreak( RtfInterpreterContext* context, RtfVisualBreakKind::RtfVisualBreakKind kind );

#if 0 //UNIMPLEMENTED currently NOT USED
            // ----------------------------------------------------------------------
            void InsertImage( IRtfInterpreterContext context, RtfVisualImageFormat format,
                              int width, int height, int desiredWidth, int desiredHeight,
                              int scaleWidthPercent, int scaleHeightPercent, string imageDataHex
                            );
#endif
            // ----------------------------------------------------------------------
            virtual void EndDocument( RtfInterpreterContext* context );

    }; // interface IRtfInterpreterListener

} // namespace Itenso.Rtf
// -- EOF -------------------------------------------------------------------
#endif //__RTFINTERPRETERLISTENER_H