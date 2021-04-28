// -- FILE ------------------------------------------------------------------
// name       : RtfInterpreterState.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.19
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
#ifndef __RTFINTERPRETERSTATE_H
#define __RTFINTERPRETERSTATE_H

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    namespace RtfInterpreterState
    {
        enum RtfInterpreterState
        {
            Init,
            InHeader,
            InDocument,
            Ended
        }; // enum RtfInterpreterState
    }

} // namespace Itenso.Rtf
// -- EOF -------------------------------------------------------------------
#endif //__RTFINTERPRETERSTATE_H