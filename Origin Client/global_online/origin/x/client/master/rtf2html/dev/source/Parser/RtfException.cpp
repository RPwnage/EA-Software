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

#include "RtfException.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    /// <summary>Thrown upon RTF specific error conditions.</summary>
//  [Serializable]
    // ----------------------------------------------------------------------
    /// <summary>Creates a new instance.</summary>
    RtfException::RtfException()
        :mMsg("")
    {
    } // RtfException

    // ----------------------------------------------------------------------
    /// <summary>Creates a new instance with the given message.</summary>
    /// <param name="message">the message to display</param>
    RtfException::RtfException( QString message )
        :mMsg ( message )
    {
    } // RtfException

    /*
        // ----------------------------------------------------------------------
        /// <summary>Creates a new instance with the given message, based on the given cause.</summary>
        /// <param name="message">the message to display</param>
        /// <param name="cause">the original cause for this exception</param>
        public RtfException( string message, Exception cause ) :
            base( message, cause )
        {
        } // RtfException

        // ----------------------------------------------------------------------
        /// <summary>Serialization support.</summary>
        /// <param name="info">the info to use for serialization</param>
        /// <param name="context">the context to use for serialization</param>
        protected RtfException( SerializationInfo info, StreamingContext context ) :
            base( info, context )
        {
        } // RtfException
    */

} // namespace Itenso.Rtf
// -- EOF -------------------------------------------------------------------
