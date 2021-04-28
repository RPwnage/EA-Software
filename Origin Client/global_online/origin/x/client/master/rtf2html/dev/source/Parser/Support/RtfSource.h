// -- FILE ------------------------------------------------------------------
// name       : RtfSource.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.20
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using System.IO;

#ifndef __RTF_SOURCE_H
#define __RTF_SOURCE_H

#include <string>
#include <fstream>

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfSource
    {
            // ----------------------------------------------------------------------
        public:
            RtfSource( std::wifstream* rtf );

            // ----------------------------------------------------------------------
            std::wifstream* Reader();

            // ----------------------------------------------------------------------
            // members
        private:
            std::wifstream* mReader;
    }; // class RtfSource
} // namespace Itenso.Rtf.Support
#endif //__RTF_SOURCE_H
// -- EOF -------------------------------------------------------------------
