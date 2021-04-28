// -- FILE ------------------------------------------------------------------
// name       : RtfVisualSpecialChar.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.22
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using Itenso.Sys;

#ifndef __RTFVISUALSPECIALCHAR_H
#define __RTFVISUALSPECIALCHAR_H

#include "RtfVisual.h"
#include "RtfVisualVisitor.h"

namespace RTF2HTML
{
    // ------------------------------------------------------------------------
    namespace RtfVisualSpecialCharKind
    {
        enum RtfVisualSpecialCharKind
        {
            Tabulator,
            ParagraphNumberBegin,
            ParagraphNumberEnd,
            NonBreakingSpace,
            EmDash,
            EnDash,
            EmSpace,
            EnSpace,
            QmSpace,
            Bullet,
            LeftSingleQuote,
            RightSingleQuote,
            LeftDoubleQuote,
            RightDoubleQuote,
            OptionalHyphen,
            NonBreakingHyphen
        }; // enum RtfVisualSpecialCharKind
    }

    // ------------------------------------------------------------------------
    class RtfVisualSpecialChar : public RtfVisual
    {
        public:

            // ----------------------------------------------------------------------
            RtfVisualSpecialChar( RtfVisualSpecialCharKind::RtfVisualSpecialCharKind charKind, QObject* parent = 0 );
            // ----------------------------------------------------------------------
            virtual bool IsEqual( ObjectBase* obj );
            virtual QString& ToString();

            // ----------------------------------------------------------------------
            virtual void DoVisit( RtfVisualVisitor* visitor );

            // ----------------------------------------------------------------------
            RtfVisualSpecialCharKind::RtfVisualSpecialCharKind CharKind ();
            // ----------------------------------------------------------------------

            // ----------------------------------------------------------------------
            // members
        private:
            RtfVisualSpecialCharKind::RtfVisualSpecialCharKind charKind;

            QString toStr;

    }; // class RtfVisualSpecialChar
} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
#endif //__RTFVISUALSPECIALCHAR_H