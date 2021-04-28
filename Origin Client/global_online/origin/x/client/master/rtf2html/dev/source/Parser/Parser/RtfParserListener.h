// -- FILE ------------------------------------------------------------------
// name       : IRtfParserListener.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.19
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------

#ifndef __RTFPARSELISTENER_H
#define __RTFPARSELISTENER_H

#include <qobject.h>
#include <vector>
#include "RtfTag.h"
#include "RtfText.h"
#include "RtfException.h"

namespace RTF2HTML
{
    class RtfParserListener;

    typedef std::vector<RtfParserListener*> RtfParserListenerList;

    class RtfParserListenerVector : public QObject
    {
        public:
            RtfParserListenerVector(QObject* parent = 0);
            ~RtfParserListenerVector();

            void Add (RtfParserListener* listener);
            bool Contains (RtfParserListener* listener);
            void Remove (RtfParserListener* listener);

            RtfParserListenerList* ListenerVector();


        private:
            RtfParserListenerList mListenerVector;
    };

    // ------------------------------------------------------------------------
    class RtfParserListener : public QObject
    {
        public:
            RtfParserListener (QObject* parent = 0);
            virtual ~RtfParserListener();

            // ----------------------------------------------------------------------
            /// <summary>
            /// Called before any other of the methods upon starting parsing of new input.
            /// </summary>
            virtual void ParseBegin();

            // ----------------------------------------------------------------------
            /// <summary>
            /// Called when a new group began.
            /// </summary>
            virtual void GroupBegin();

            // ----------------------------------------------------------------------
            /// <summary>
            /// Called when a new tag was found.
            /// </summary>
            /// <param name="tag">the newly found tag</param>
            virtual void TagFound( RtfTag* tag );

            // ----------------------------------------------------------------------
            /// <summary>
            /// Called when a new text was found.
            /// </summary>
            /// <param name="text">the newly found text</param>
            virtual void TextFound( RtfText* text );

            // ----------------------------------------------------------------------
            /// <summary>
            /// Called after a group ended.
            /// </summary>
            virtual void GroupEnd();

            // ----------------------------------------------------------------------
            /// <summary>
            /// Called if parsing finished sucessfully.
            /// </summary>
            virtual void ParseSuccess();

            // ----------------------------------------------------------------------
            /// <summary>
            /// Called if parsing failed.
            /// </summary>
            /// <param name="reason">the reason for the failure</param>
            virtual void ParseFail( RtfException* reason );

            // ----------------------------------------------------------------------
            /// <summary>
            /// Called after parsing finished. Always called, also in case of a failure.
            /// </summary>
            virtual void ParseEnd();
    }; // interface IRtfParserListener
} // namespace Itenso.Rtf
#endif //__RTFPARSELISTENER_H
// -- EOF -------------------------------------------------------------------
