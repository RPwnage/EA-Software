// -- FILE ------------------------------------------------------------------
// name       : RtfText.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.19
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using Itenso.Sys;

#ifndef __RTFTEXT_H
#define __RTFTEXT_H

#include <string>
#include "RtfElement.h"

namespace RTF2HTML
{
    // ------------------------------------------------------------------------
    class RtfText : public RtfElement
    {
        public:

            // ----------------------------------------------------------------------
            RtfText( QString text, int codePage=1252, QObject* parent = 0 );
            virtual ~RtfText();

            // ----------------------------------------------------------------------
            QString& Text ();

            // ----------------------------------------------------------------------
            int codePage () {
                return textCodePage;
            }

            // ----------------------------------------------------------------------
            virtual QString& ToString();

            // ----------------------------------------------------------------------
            virtual void DoVisit( RtfElementVisitor* visitor );

            virtual bool IsEqual (ObjectBase* obj);
        private:
            // ----------------------------------------------------------------------
            // members
            QString text;
            int  textCodePage;

    }; // class RtfText
} // namespace Itenso.Rtf.Model
#endif //__RTFTEXT_H
// -- EOF -------------------------------------------------------------------
