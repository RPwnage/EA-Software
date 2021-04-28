// -- FILE ------------------------------------------------------------------
// name       : IRtfColor.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.20
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System.Drawing;

#ifndef __RTFCOLOR_H
#define __RTFCOLOR_H

#include <qobject.h>
#include <qcolor.h>
#include "ObjectBase.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfColor: public ObjectBase
    {
        public:

            RtfColor( QObject* parent = 0);
            RtfColor( int red, int green, int blue, QObject* parent = 0 );

            virtual ~RtfColor();

            // ----------------------------------------------------------------------
            virtual bool Equals (RTF2HTML::ObjectBase* obj);
            virtual QString& ToString();
            virtual bool IsEqual (RTF2HTML::ObjectBase* obj );

            // ----------------------------------------------------------------------
            int Red ();

            // ----------------------------------------------------------------------
            int Green ();

            // ----------------------------------------------------------------------
            int Blue ();

            // ----------------------------------------------------------------------
            QColor& AsDrawingColor ();

            // ----------------------------------------------------------------------
            void setRGB (int red, int green, int blue);

            static RtfColor Black;
            static RtfColor White;

        private:
            int red;
            int green;
            int blue;

            QColor drawingColor;
            QString toStr;

    }; // interface IRtfColor

} // namespace Itenso.Rtf
// -- EOF -------------------------------------------------------------------
#endif //__RTFCOLOR_H