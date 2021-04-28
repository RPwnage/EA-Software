// -- FILE ------------------------------------------------------------------
// name       : RtfFontKind.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.19
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
#ifndef __RTFFONTKIND_H
#define __RTFFONTKIND_H

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    namespace RtfFontKind
    {
        enum RtfFontKind
        {
            Nil,
            Roman,
            Swiss,
            Modern,
            Script,
            Decor,
            Tech,
            Bidi
        }; // enum RtfFontKind
    }

    // ------------------------------------------------------------------------
    namespace RtfFontPitch
    {
        enum RtfFontPitch
        {
            Default,
            Fixed,
            Variable
        }; // enum RtfFontPitch
    }

} // namespace Itenso.Rtf
// -- EOF -------------------------------------------------------------------
#endif //__RTFFONTKIND_H