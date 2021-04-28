// -- FILE ------------------------------------------------------------------
// name       : IRtfSource.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.20
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System.IO;

#ifndef __RTFSOURCE_H
#define __RTFSOURCE_H


namespace RTF2HTML
{
    class TextReader;

    // ------------------------------------------------------------------------
    class RtfSource
    {
        public:

            // ----------------------------------------------------------------------
            virtual TextReader Reader ();

    }; // interface IRtfSource
} // namespace Itenso.Rtf

#endif //__RTFSOURCE_H
// -- EOF -------------------------------------------------------------------
