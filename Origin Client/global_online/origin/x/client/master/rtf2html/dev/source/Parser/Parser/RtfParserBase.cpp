// -- FILE ------------------------------------------------------------------
// name       : RtfParserBase.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.20
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using System.Collections;

#include <assert.h>
#include "RtfParserBase.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    RtfParserBase::RtfParserBase(QObject* parent)
        : QObject (parent)
        , mIgnoreContentAfterRootGroup(false)
    {

    } // RtfParserBase

    // ----------------------------------------------------------------------
    RtfParserBase::RtfParserBase( RtfParserListenerList& listeners, QObject* parent )
        : QObject (parent)
        , mIgnoreContentAfterRootGroup (false)
    {
        if ( listeners.size() > 0 )
        {
            RtfParserListenerList::iterator it;
            for (it = listeners.begin(); it != listeners.end(); it++)
            {
                AddParserListener ((*it));
            }
        }
    } // RtfParserBase

    // ----------------------------------------------------------------------
    bool RtfParserBase::getIgnoreContentAfterRootGroup ()
    {
        return mIgnoreContentAfterRootGroup;
    }

    void RtfParserBase::setIgnoreContentAfterRootGroup (bool set)
    {
        mIgnoreContentAfterRootGroup = set;
    }

    // ----------------------------------------------------------------------
    void RtfParserBase::AddParserListener( RtfParserListener* listener )
    {
        if ( listener == NULL )
        {
            assert (0);
        }

        if ( !mListeners.Contains( listener ) )
        {
            mListeners.Add( listener );
        }
    } // AddParserListener

    // ----------------------------------------------------------------------
    void RtfParserBase::RemoveParserListener( RtfParserListener* listener )
    {
        if ( listener == NULL )
        {
            assert (0);
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
    } // RemoveParserListener

    // ----------------------------------------------------------------------
    void RtfParserBase::Parse( char* srcBuffer, size_t srcBufSz )
    {
        if ( srcBuffer == NULL )
        {
            assert (0);
        }
        DoParse( srcBuffer, srcBufSz );
    } // Parse

    // ----------------------------------------------------------------------
    void RtfParserBase::DoParse( char* srcBuffer, size_t srcBufSz )
    {
        Q_UNUSED (srcBuffer);
        Q_UNUSED (srcBufSz);
    }

    // ----------------------------------------------------------------------
    void RtfParserBase::NotifyParseBegin()
    {
        RtfParserListenerList* listeners = mListeners.ListenerVector();

        if ( listeners->size() > 0 )
        {
            RtfParserListenerList::iterator it;
            for (it = listeners->begin(); it != listeners->end(); it++)
            {
                (*it)->ParseBegin();
            }
        }
    } // NotifyParseBegin

    // ----------------------------------------------------------------------
    void RtfParserBase::NotifyGroupBegin()
    {
        RtfParserListenerList* listeners = mListeners.ListenerVector();

        if ( listeners->size() > 0 )
        {
            RtfParserListenerList::iterator it;
            for (it = listeners->begin(); it != listeners->end(); it++)
            {
                (*it)->GroupBegin();
            }
        }
    } // NotifyGroupBegin

    // ----------------------------------------------------------------------
    void RtfParserBase::NotifyTagFound( RtfTag* tag )
    {
        RtfParserListenerList* listeners = mListeners.ListenerVector();

        if ( listeners->size() > 0 )
        {
            RtfParserListenerList::iterator it;
            for (it = listeners->begin(); it != listeners->end(); it++)
            {
                (*it)->TagFound( tag );
            }
        }
    } // NotifyTagFound

    // ----------------------------------------------------------------------
    void RtfParserBase::NotifyTextFound( RtfText* text )
    {
        RtfParserListenerList* listeners = mListeners.ListenerVector();

        if ( listeners->size() > 0 )
        {
            RtfParserListenerList::iterator it;
            for (it = listeners->begin(); it != listeners->end(); it++)
            {
                (*it)->TextFound( text );
            }
        }
    } // NotifyTextFound

    // ----------------------------------------------------------------------
    void RtfParserBase::NotifyGroupEnd()
    {
        RtfParserListenerList* listeners = mListeners.ListenerVector();

        if ( listeners->size() > 0 )
        {
            RtfParserListenerList::iterator it;
            for (it = listeners->begin(); it != listeners->end(); it++)
            {
                (*it)->GroupEnd();
            }
        }
    } // NotifyGroupEnd

    // ----------------------------------------------------------------------
    void RtfParserBase::NotifyParseSuccess()
    {
        RtfParserListenerList* listeners = mListeners.ListenerVector();

        if ( listeners->size() > 0 )
        {
            RtfParserListenerList::iterator it;
            for (it = listeners->begin(); it != listeners->end(); it++)
            {
                (*it)->ParseSuccess();
            }
        }
    } // NotifyParseSuccess

    // ----------------------------------------------------------------------
    void RtfParserBase::NotifyParseFail( RtfException* reason )
    {
        RtfParserListenerList* listeners = mListeners.ListenerVector();

        if ( listeners->size() > 0 )
        {
            RtfParserListenerList::iterator it;
            for (it = listeners->begin(); it != listeners->end(); it++)
            {
                (*it)->ParseFail( reason );
            }
        }
    } // NotifyParseFail

    // ----------------------------------------------------------------------
    void RtfParserBase::NotifyParseEnd()
    {
        RtfParserListenerList* listeners = mListeners.ListenerVector();

        if ( listeners->size() > 0 )
        {
            RtfParserListenerList::iterator it;
            for (it = listeners->begin(); it != listeners->end(); it++)
            {
                (*it)->ParseEnd();
            }
        }
    } // NotifyParseEnd


} // namespace Itenso.Rtf.Parser
// -- EOF -------------------------------------------------------------------
