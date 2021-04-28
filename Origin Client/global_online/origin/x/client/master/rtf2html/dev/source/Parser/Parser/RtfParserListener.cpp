// -- FILE ------------------------------------------------------------------
// name       : IRtfParserListener.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.19
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------

#include <vector>
#include "RtfTag.h"
#include "RtfText.h"
#include "RtfException.h"

#include "RtfParserListener.h"

namespace RTF2HTML
{
    // ------------------------------------------------------------------------
    // RtfParserListenerVector
    // ------------------------------------------------------------------------
    RtfParserListenerVector::RtfParserListenerVector (QObject* parent)
        : QObject (parent)
    {
    }

    RtfParserListenerVector::~RtfParserListenerVector ()
    {
    }

    RtfParserListenerList* RtfParserListenerVector::ListenerVector()
    {
        return &mListenerVector;
    }

    void RtfParserListenerVector::Add (RtfParserListener* listener)
    {
        mListenerVector.push_back (listener);
    }

    bool RtfParserListenerVector::Contains (RtfParserListener* listener)
    {
        RtfParserListenerList::iterator it;
        for (it = mListenerVector.begin(); it != mListenerVector.end(); it++)
        {
            if (*it == listener)
            {
                return true;
            }
        }
        return false;
    }

    void RtfParserListenerVector::Remove (RtfParserListener* listener)
    {
        RtfParserListenerList::iterator it;
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


    RtfParserListener::RtfParserListener (QObject* parent)
        : QObject (parent)
    {
    }

    RtfParserListener::~RtfParserListener ()
    {
    }

    // ----------------------------------------------------------------------
    /// <summary>
    /// Called before any other of the methods upon starting parsing of new input.
    /// </summary>
    void RtfParserListener::ParseBegin()
    {
    }

    // ----------------------------------------------------------------------
    /// <summary>
    /// Called when a new group began.
    /// </summary>
    void RtfParserListener::GroupBegin()
    {
    }

    // ----------------------------------------------------------------------
    /// <summary>
    /// Called when a new tag was found.
    /// </summary>
    /// <param name="tag">the newly found tag</param>
    void RtfParserListener::TagFound( RtfTag* tag )
    {
        Q_UNUSED (tag);
    }

    // ----------------------------------------------------------------------
    /// <summary>
    /// Called when a new text was found.
    /// </summary>
    /// <param name="text">the newly found text</param>
    void RtfParserListener::TextFound( RtfText* text )
    {
        Q_UNUSED (text);
    }

    // ----------------------------------------------------------------------
    /// <summary>
    /// Called after a group ended.
    /// </summary>
    void RtfParserListener::GroupEnd()
    {
    }

    // ----------------------------------------------------------------------
    /// <summary>
    /// Called if parsing finished sucessfully.
    /// </summary>
    void RtfParserListener::ParseSuccess()
    {
    }

    // ----------------------------------------------------------------------
    /// <summary>
    /// Called if parsing failed.
    /// </summary>
    /// <param name="reason">the reason for the failure</param>
    void RtfParserListener::ParseFail( RtfException* reason )
    {
        Q_UNUSED (reason);
    }

    // ----------------------------------------------------------------------
    /// <summary>
    /// Called after parsing finished. Always called, also in case of a failure.
    /// </summary>
    void RtfParserListener::ParseEnd()
    {
    }
} //namespace
// -- EOF -------------------------------------------------------------------
