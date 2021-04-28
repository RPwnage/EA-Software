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

#ifndef __RTFPARSEBASE_H
#define __RTFPARSEBASE_H

#include <qobject.h>

#include "RtfTag.h"
#include "RtfText.h"
#include "RtfException.h"
#include "RtfParserListener.h"
#include "RtfSource.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfParserBase : public QObject
    {
        public:
            RtfParserBase(QObject* parent = 0);
            // ----------------------------------------------------------------------
            RtfParserBase( RtfParserListenerList& listeners, QObject* parent = 0 );
            // ----------------------------------------------------------------------
            /// <summary>
            /// Determines whether to ignore all content after the root group ends.
            /// Set this to true when parsing content from streams which contain other
            /// data after the RTF or if the writer of the RTF is known to terminate the
            /// actual RTF content with a null byte (as some popular sources such as
            /// WordPad are known to behave).
            /// </summary>
            virtual bool getIgnoreContentAfterRootGroup ();
            virtual void setIgnoreContentAfterRootGroup (bool set);

            // ----------------------------------------------------------------------
            /// <summary>
            /// Adds a listener that will get notified along the parsing process.
            /// </summary>
            /// <param name="listener">the listener to add</param>
            /// <exception cref="ArgumentNullException">in case of a null argument</exception>
            virtual void AddParserListener( RtfParserListener* listener );

            // ----------------------------------------------------------------------
            /// <summary>
            /// Removes a listener from this instance.
            /// </summary>
            /// <param name="listener">the listener to remove</param>
            /// <exception cref="ArgumentNullException">in case of a null argument</exception>
            virtual void RemoveParserListener( RtfParserListener* listener );

            // ----------------------------------------------------------------------
            /// <summary>
            /// Parses the given RTF text that is read from the given source.
            /// </summary>
            /// <param name="rtfTextSource">the source with RTF text to parse</param>
            /// <exception cref="RtfException">in case of invalid RTF syntax</exception>
            /// <exception cref="IOException">in case of an IO error</exception>
            /// <exception cref="ArgumentNullException">in case of a null argument</exception>
            virtual void Parse( char* srcBuffer, size_t srcBufSz );

            bool GetIgnoreContentAfterRootGroup ();
            bool SetIgnoreContentAfterRootGroup ();


            // ----------------------------------------------------------------------
            virtual void DoParse( char* srcBuffer, size_t srcBufSz );

            // ----------------------------------------------------------------------
            void NotifyParseBegin();

            // ----------------------------------------------------------------------
            void NotifyGroupBegin();

            // ----------------------------------------------------------------------
            void NotifyTagFound( RtfTag* tag );

            // ----------------------------------------------------------------------
            void NotifyTextFound( RtfText* text );

            // ----------------------------------------------------------------------
            void NotifyGroupEnd();

            // ----------------------------------------------------------------------
            void NotifyParseSuccess();
            // ----------------------------------------------------------------------
            void NotifyParseFail( RtfException* reason );
            // ----------------------------------------------------------------------
            void NotifyParseEnd();
            // ----------------------------------------------------------------------
            // members
        private:
            bool Contains (RtfParserListener* listener);

            bool mIgnoreContentAfterRootGroup;
            RtfParserListenerVector mListeners;
    }; // class RtfParserBase

} // namespace Itenso.Rtf.Parser
#endif //__RTFPARSEBASE_H
// -- EOF -------------------------------------------------------------------
