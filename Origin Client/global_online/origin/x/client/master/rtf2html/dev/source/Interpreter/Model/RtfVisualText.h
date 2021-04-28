// -- FILE ------------------------------------------------------------------
// name       : RtfVisualText.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.22
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using Itenso.Sys;

#ifndef __RTFVISUALTEXT_H
#define __RTFVISUALTEXT_H

#include <qstring.h>
#include "RtfVisual.h"
#include "RtfTextFormat.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfVisualText : public RtfVisual
    {
        public:

            // ----------------------------------------------------------------------
            RtfVisualText( QString& text, RtfTextFormat* format, QObject* parent = 0 );

            // ----------------------------------------------------------------------
            virtual void DoVisit( RtfVisualVisitor* visitor );
            virtual bool IsEqual( ObjectBase* obj );
            virtual QString& ToString();

            // ----------------------------------------------------------------------
            QString getText();

            // ----------------------------------------------------------------------
            RtfTextFormat* getFormat ();
            void setFormat (RtfTextFormat* format);

            // ----------------------------------------------------------------------
            // members
        private:
            QString text;
            RtfTextFormat* format;

            QString toStr;
    }; // class RtfVisualText

} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
#endif //__RTFVISUALTEXT_H