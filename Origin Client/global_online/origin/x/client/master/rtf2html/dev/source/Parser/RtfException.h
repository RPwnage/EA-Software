// -- FILE ------------------------------------------------------------------
// name       : RtfException.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.19
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using System.Runtime.Serialization;

#ifndef __RTFEXCEPTION_H
#define __RTFEXCEPTION_H

#include <qstring.h>

namespace RTF2HTML
{
    // ------------------------------------------------------------------------
    /// <summary>Thrown upon RTF specific error conditions.</summary>
//  [Serializable]
    class RtfException
    {
        public:
            // ----------------------------------------------------------------------
            /// <summary>Creates a new instance.</summary>
            RtfException();

            // ----------------------------------------------------------------------
            /// <summary>Creates a new instance with the given message.</summary>
            /// <param name="message">the message to display</param>
            RtfException( QString message );

            /*
                        // ----------------------------------------------------------------------
                        /// <summary>Creates a new instance with the given message, based on the given cause.</summary>
                        /// <param name="message">the message to display</param>
                        /// <param name="cause">the original cause for this exception</param>
                        public RtfException( string message, Exception cause ) :
                        base( message, cause )
                    {
                    } // RtfException
            */

            /*
                    // ----------------------------------------------------------------------
                    /// <summary>Serialization support.</summary>
                    /// <param name="info">the info to use for serialization</param>
                    /// <param name="context">the context to use for serialization</param>
                    protected RtfException( SerializationInfo info, StreamingContext context ) :
                        base( info, context )
                    {
                    } // RtfException
            */
        private:
            QString mMsg;
    }; // class RtfException
} // namespace Itenso.Rtf
#endif //__RTFEXCEPTION_H
// -- EOF -------------------------------------------------------------------
