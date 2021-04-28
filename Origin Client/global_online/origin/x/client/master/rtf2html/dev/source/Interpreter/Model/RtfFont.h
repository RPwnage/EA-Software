// -- FILE ------------------------------------------------------------------
// name       : IRtfFont.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.20
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
#ifndef __H_RTFFONT_H
#define __H_RTFFONT_H

#include <qstring.h>
#include "RtfFontEnums.h"
#include "ObjectBase.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfFont: public ObjectBase
    {
        public:
            RtfFont( QString id, RtfFontKind::RtfFontKind kind, RtfFontPitch::RtfFontPitch pitch, int charSet, int codePage, QString name, QObject* parent = 0 );
            virtual ~RtfFont();

            // ----------------------------------------------------------------------
            virtual bool Equals (RTF2HTML::ObjectBase* obj);
            virtual QString& ToString();

            virtual bool IsEqual (RTF2HTML::ObjectBase* obj );
            // ----------------------------------------------------------------------
            QString& Id ();

            // ----------------------------------------------------------------------
            RtfFontKind::RtfFontKind Kind ();

            // ----------------------------------------------------------------------
            RtfFontPitch::RtfFontPitch Pitch ();

            // ----------------------------------------------------------------------
            int CharSet ();

            // ----------------------------------------------------------------------
            int CodePage ();

            // ----------------------------------------------------------------------
            QString& Name ();



        private:
            QString id;
            RtfFontKind::RtfFontKind kind;
            RtfFontPitch::RtfFontPitch pitch;
            int charSet;
            int codePage;
            QString name;
            QString toStr;

    }; // interface IRtfFont

} // namespace Itenso.Rtf
// -- EOF -------------------------------------------------------------------
#endif //__H_RTFFONT_H