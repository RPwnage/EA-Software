// -- FILE ------------------------------------------------------------------
// name       : RtfFontBuilder.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.21
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System.Text;
//using Itenso.Rtf.Model;
//using Itenso.Rtf.Support;

#ifndef __RTFFONTBUILDER_H
#define __RTFFONTBUILDER_H

#include <qstring.h>
#include "RtfElementVisitorBase.h"
#include "RtfFont.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfFontBuilder : public RtfElementVisitorBase
    {
        public:

            // ----------------------------------------------------------------------
            RtfFontBuilder(QObject* parent = 0);

            // ----------------------------------------------------------------------
            QString& FontId ();

            // ----------------------------------------------------------------------
            int FontIndex ();

            // ----------------------------------------------------------------------
            int FontCharset ();
            // ----------------------------------------------------------------------

            int FontCodePage ();

            // ----------------------------------------------------------------------
            RtfFontKind::RtfFontKind FontKind ();

            // ----------------------------------------------------------------------
            RtfFontPitch::RtfFontPitch FontPitch ();

            // ----------------------------------------------------------------------
            QString FontName ();

            // ----------------------------------------------------------------------
            RtfFont* CreateFont();

            // ----------------------------------------------------------------------
            void Reset();

            // ----------------------------------------------------------------------
            virtual void DoVisitGroup( RtfGroup* group );
            virtual void DoVisitTag( RtfTag* tag );
            virtual void DoVisitText( RtfText* text );

        private:
            // ----------------------------------------------------------------------
            // members
            QString fontId;
            int fontIndex;
            int fontCharset;
            int fontCodePage;
            RtfFontKind::RtfFontKind fontKind;
            RtfFontPitch::RtfFontPitch fontPitch;

            QString fontNameBuffer;
    }; // class RtfFontBuilder

} // namespace Itenso.Rtf.Interpreter
// -- EOF -------------------------------------------------------------------
#endif //__RTFFONTBUILDER_H