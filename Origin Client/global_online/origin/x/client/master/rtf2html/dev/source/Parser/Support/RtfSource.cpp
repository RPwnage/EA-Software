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

#include <assert.h>
#include "RtfSource.h"

namespace RTF2HTML
{

    RtfSource::RtfSource( std::wifstream* rtf )
    {
        if (rtf == NULL)
        {
            assert (0);
        }
        mReader = rtf;
    }

    // ----------------------------------------------------------------------
    std::wifstream* RtfSource::Reader ()
    {
        return mReader;
    } // Reader
} // namespace Itenso.Rtf.Support
// -- EOF -------------------------------------------------------------------
