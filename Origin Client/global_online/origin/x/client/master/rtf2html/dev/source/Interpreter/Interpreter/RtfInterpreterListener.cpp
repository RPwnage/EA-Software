// -- FILE ------------------------------------------------------------------
// name       : IRtfInterpreterListener.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.21
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------

#include "RtfInterpreterListener.h"

namespace RTF2HTML
{
    // ------------------------------------------------------------------------
    // RtfInterpreterListenerVector
    // ------------------------------------------------------------------------
    RtfInterpreterListenerVector::RtfInterpreterListenerVector (QObject* parent)
        : QObject (parent)
    {
    }

    RtfInterpreterListenerVector::~RtfInterpreterListenerVector ()
    {
    }

    RtfInterpreterListenerList* RtfInterpreterListenerVector::ListenerVector()
    {
        return &mListenerVector;
    }

    void RtfInterpreterListenerVector::Add (RtfInterpreterListener* listener)
    {
        mListenerVector.push_back (listener);
    }

    bool RtfInterpreterListenerVector::Contains (RtfInterpreterListener* listener)
    {
        RtfInterpreterListenerList::iterator it;
        for (it = mListenerVector.begin(); it != mListenerVector.end(); it++)
        {
            if (*it == listener)
            {
                return true;
            }
        }
        return false;
    }

    void RtfInterpreterListenerVector::Remove (RtfInterpreterListener* listener)
    {
        RtfInterpreterListenerList::iterator it;
        int pos = 0;
        for (it = mListenerVector.begin(); it != mListenerVector.end(); it++)
        {
            if (*it == listener)
            {
                mListenerVector.erase (mListenerVector.begin() + pos);
                break;
            }
        }
    }

    // ------------------------------------------------------------------------
    // RtfInterpreterListener
    // ------------------------------------------------------------------------
    RtfInterpreterListener::RtfInterpreterListener (QObject* parent)
        : QObject (parent)
    {
    }

    RtfInterpreterListener::~RtfInterpreterListener()
    {
    }
    // ----------------------------------------------------------------------
    void RtfInterpreterListener::BeginDocument( RtfInterpreterContext* context )
    {
        Q_UNUSED (context);
    }

    // ----------------------------------------------------------------------
    void RtfInterpreterListener::InsertText( RtfInterpreterContext* context, QString& text )
    {
        Q_UNUSED (context);
        Q_UNUSED (text);
    }

    // ----------------------------------------------------------------------
    void RtfInterpreterListener::InsertSpecialChar( RtfInterpreterContext* context, RtfVisualSpecialCharKind::RtfVisualSpecialCharKind kind )
    {
        Q_UNUSED (context);
        Q_UNUSED (kind);
    }

    // ----------------------------------------------------------------------
    void RtfInterpreterListener::InsertBreak( RtfInterpreterContext* context, RtfVisualBreakKind::RtfVisualBreakKind kind )
    {
        Q_UNUSED (context);
        Q_UNUSED (kind);
    }

#if 0 //UNIMPLEMENTED currently NOT USED
    // ----------------------------------------------------------------------
    void InsertImage( IRtfInterpreterContext context, RtfVisualImageFormat format,
                      int width, int height, int desiredWidth, int desiredHeight,
                      int scaleWidthPercent, int scaleHeightPercent, string imageDataHex
                    );
#endif
    // ----------------------------------------------------------------------
    void RtfInterpreterListener::EndDocument( RtfInterpreterContext* context )
    {
        Q_UNUSED (context);
    }

} // namespace Itenso.Rtf
// -- EOF -------------------------------------------------------------------
