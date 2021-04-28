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

#include <assert.h>
#include "RtfInterpreterBase.h"

namespace RTF2HTML
{
    // ----------------------------------------------------------------------
    RtfInterpreterBase::RtfInterpreterBase( RtfInterpreterListenerList& listeners, QObject* parent )
        : QObject (parent)
    {
        if ( listeners.size() > 0 )
        {
            RtfInterpreterListenerList::iterator it;
            for (it = listeners.begin(); it != listeners.end(); it++)
            {
                AddInterpreterListener ((*it));
            }
        }

        context = new RtfInterpreterContext(this);
    } // RtfInterpreterBase

    RtfInterpreterBase::~RtfInterpreterBase()
    {
    }

#if USE_SETTINGS
    // ----------------------------------------------------------------------
    protected RtfInterpreterBase( IRtfInterpreterSettings settings, params IRtfInterpreterListener[] listeners )

    {
        if ( settings == null )
        {
            throw new ArgumentNullException( "settings" );
        }

        this.settings = settings;
        if ( listeners != null )
        {
            foreach ( IRtfInterpreterListener listener in listeners )
            {
                AddInterpreterListener( listener );
            }
        }
    } // RtfInterpreterBase

    // ----------------------------------------------------------------------
    public IRtfInterpreterSettings Settings
    {
        get { return settings; }
    } // Settings
#endif
    // ----------------------------------------------------------------------
    void RtfInterpreterBase::AddInterpreterListener( RtfInterpreterListener* listener )
    {
        if ( listener == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "listener" );
        }
        if ( !mListeners.Contains( listener ) )
        {
            mListeners.Add( listener );
        }
    } // AddInterpreterListener

    // ----------------------------------------------------------------------
    void RtfInterpreterBase::RemoveInterpreterListener( RtfInterpreterListener* listener )
    {
        if ( listener == NULL)
        {
            assert (0);
            //throw new ArgumentNullException( "listener" );
        }
        if ( mListeners.ListenerVector()->size() > 0)
        {
            if ( mListeners.Contains( listener ) )
            {
                mListeners.Remove( listener );
            }
            //if ( listeners.Count == 0 )
            //{
            //  listeners = null;
            //}
        }
    } // RemoveInterpreterListener

    // ----------------------------------------------------------------------
    void RtfInterpreterBase::Interpret( RtfGroup* rtfDocument )
    {
        if ( rtfDocument == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "rtfDocument" );
        }
        DoInterpret( rtfDocument );
    } // Interpret

    // ----------------------------------------------------------------------
    void RtfInterpreterBase::DoInterpret( RtfGroup* rtfDocument )
    {
        Q_UNUSED (rtfDocument);
    }

    // ----------------------------------------------------------------------
    void RtfInterpreterBase::NotifyBeginDocument()
    {
        RtfInterpreterListenerList* listeners = mListeners.ListenerVector();

        if ( listeners->size() > 0 )
        {
            RtfInterpreterListenerList::iterator it;
            for (it = listeners->begin(); it != listeners->end(); it++)
            {
                (*it)->BeginDocument(context);
            }
        }
    } // NotifyBeginDocument

    // ----------------------------------------------------------------------
    void RtfInterpreterBase::NotifyInsertText( QString& text )
    {
        RtfInterpreterListenerList* listeners = mListeners.ListenerVector();

        if ( listeners->size() > 0 )
        {
            RtfInterpreterListenerList::iterator it;
            for (it = listeners->begin(); it != listeners->end(); it++)
            {
                (*it)->InsertText( context, text );
            }
        }
    } // NotifyInsertText

    // ----------------------------------------------------------------------
    void RtfInterpreterBase::NotifyInsertSpecialChar( RtfVisualSpecialCharKind::RtfVisualSpecialCharKind kind )
    {
        RtfInterpreterListenerList* listeners = mListeners.ListenerVector();

        if ( listeners->size() > 0 )
        {
            RtfInterpreterListenerList::iterator it;
            for (it = listeners->begin(); it != listeners->end(); it++)
            {
                (*it)->InsertSpecialChar( context, kind );
            }
        }
    } // NotifyInsertSpecialChar

    // ----------------------------------------------------------------------
    void RtfInterpreterBase::NotifyInsertBreak( RtfVisualBreakKind::RtfVisualBreakKind kind )
    {
        RtfInterpreterListenerList* listeners = mListeners.ListenerVector();

        if ( listeners->size() > 0 )
        {
            RtfInterpreterListenerList::iterator it;
            for (it = listeners->begin(); it != listeners->end(); it++)
            {
                (*it)->InsertBreak( context, kind );
            }
        }

    } // NotifyInsertBreak

    // ----------------------------------------------------------------------
#if 0 //UNIMPLEMENTED currently NOT USED
    protected void NotifyInsertImage( RtfVisualImageFormat format,
                                      int width, int height, int desiredWidth, int desiredHeight,
                                      int scaleWidthPercent, int scaleHeightPercent, string imageDataHex
                                    )
    {
        if ( listeners != null )
        {
            foreach ( IRtfInterpreterListener listener in listeners )
            {
                listener.InsertImage(
                    context,
                    format,
                    width,
                    height,
                    desiredWidth,
                    desiredHeight,
                    scaleWidthPercent,
                    scaleHeightPercent,
                    imageDataHex );
            }
        }
    } // NotifyInsertImage
#endif
    // ----------------------------------------------------------------------
    void RtfInterpreterBase::NotifyEndDocument()
    {
        RtfInterpreterListenerList* listeners = mListeners.ListenerVector();

        if ( listeners->size() > 0 )
        {
            RtfInterpreterListenerList::iterator it;
            for (it = listeners->begin(); it != listeners->end(); it++)
            {
                (*it)->EndDocument( context );
            }
        }

    } // NotifyEndDocument

    // ----------------------------------------------------------------------
    RtfInterpreterContext* RtfInterpreterBase::Context()
    {
        return context;
    } // Context

} // namespace Itenso.Rtf.Interpreter
// -- EOF -------------------------------------------------------------------
