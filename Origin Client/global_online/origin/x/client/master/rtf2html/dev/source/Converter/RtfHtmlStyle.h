// -- FILE ------------------------------------------------------------------
// name       : RtfHtmlStyle.cs
// project    : RTF Framelet
// created    : Jani Giannoudis - 2008.06.02
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using Itenso.Sys;

#ifndef __RTFHTMLSTYLE_H
#define __RTFHTMLSTYLE_H

#include "ObjectBase.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfHtmlStyle : public ObjectBase
    {
        public:
            RtfHtmlStyle(QObject* parent = 0);

            // ----------------------------------------------------------------------
            QString& getForegroundColor ();
            void setForegroundColor (QString& value);

            // ----------------------------------------------------------------------
            QString& getBackgroundColor();
            void setBackgroundColor (QString& value);

            // ----------------------------------------------------------------------
            QString& getFontFamily();
            void setFontFamily (QString& value);

            // ----------------------------------------------------------------------
            QString& getFontSize();
            void setFontSize (QString& value);

            // ----------------------------------------------------------------------
            bool IsEmpty();

            // ----------------------------------------------------------------------
            virtual bool Equals( ObjectBase* obj );

#if 0 //UNIMPLEMENTED, currently NOT USED
            // ----------------------------------------------------------------------
            public sealed override int GetHashCode()
#endif
            // ----------------------------------------------------------------------
            virtual bool IsEqual( ObjectBase* obj );
            // ----------------------------------------------------------------------
#if 0 //UNIMPLEMENTED, currently NOT USED
            private int ComputeHashCode()
#endif

        private:
            // ----------------------------------------------------------------------
            // members
            QString foregroundColor;
            QString backgroundColor;
            QString fontFamily;
            QString fontSize;

    }; // class RtfHtmlStyle

} // namespace Itenso.Rtf.Converter.Html
// -- EOF -------------------------------------------------------------------
#endif //__RTFHTMLSTYLE_H