// -- FILE ------------------------------------------------------------------
// name       : IRtfTextFormat.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.20
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
#ifndef __RTFTEXTFORMAT_H
#define __RTFTEXTFORMAT_H

#include <qstring.h>
#include "ObjectBase.h"
#include "RtfColor.h"
#include "RtfFont.h"

namespace RTF2HTML
{
    // ------------------------------------------------------------------------
    namespace RtfTextAlignment
    {
        enum RtfTextAlignment
        {
            Left,
            Center,
            Right,
            Justify
        }; // enum RtfTextAlignment
    }

    // ------------------------------------------------------------------------
    class RtfTextFormat: public ObjectBase
    {
        public:

            static QString RtfTextAlignmentStr [];

            RtfTextFormat( RtfFont* font, int fontSize, QObject* parent = 0 );
            RtfTextFormat( RtfTextFormat* copy, QObject* parent = 0 );

            // ----------------------------------------------------------------------
            virtual bool Equals (RTF2HTML::ObjectBase* obj);
            virtual QString& ToString();
            virtual bool IsEqual (RTF2HTML::ObjectBase* obj );

            // ----------------------------------------------------------------------
            RtfFont* Font ();

            // ----------------------------------------------------------------------
            int FontSize ();

            // ----------------------------------------------------------------------
            /// <summary>
            /// Combines the setting for sub/super script: negative values are considered
            /// equivalent to subscript, positive values correspond to superscript.<br/>
            /// Same unit as font size.
            /// </summary>
            int SuperScript ();

            // ----------------------------------------------------------------------
            bool IsNormal ();

            // ----------------------------------------------------------------------
            bool IsBold ();

            // ----------------------------------------------------------------------
            bool IsItalic ();

            // ----------------------------------------------------------------------
            bool IsUnderline ();

            // ----------------------------------------------------------------------
            bool IsStrikeThrough ();

            // ----------------------------------------------------------------------
            bool IsHidden ();

            // ----------------------------------------------------------------------
            QString FontDescriptionDebug ();

            // ----------------------------------------------------------------------
            RtfColor* BackgroundColor ();

            // ----------------------------------------------------------------------
            RtfColor* ForegroundColor ();

            // ----------------------------------------------------------------------
            RtfTextAlignment::RtfTextAlignment Alignment ();

            // ----------------------------------------------------------------------
            RtfTextFormat* Duplicate();

            // ----------------------------------------------------------------------
            RtfTextFormat* DeriveWithFont( RtfFont* rtfFont );
            RtfTextFormat* DeriveWithFontSize( int derivedFontSize );
            RtfTextFormat* DeriveWithSuperScript( int deviation );
            RtfTextFormat* DeriveWithSuperScript( bool super );
            RtfTextFormat* DeriveNormal();
            RtfTextFormat* DeriveWithBold( bool derivedBold );
            RtfTextFormat* DeriveWithItalic( bool derivedItalic );
            RtfTextFormat* DeriveWithUnderline( bool derivedUnderline );
            RtfTextFormat* DeriveWithStrikeThrough( bool derivedStrikeThrough );
            RtfTextFormat* DeriveWithHidden( bool derivedHidden );
            RtfTextFormat* DeriveWithBackgroundColor( RtfColor* derivedBackgroundColor );
            RtfTextFormat* DeriveWithForegroundColor( RtfColor* derivedForegroundColor );
            RtfTextFormat* DeriveWithAlignment( RtfTextAlignment::RtfTextAlignment derivedAlignment );

        private:
            RtfFont* font;
            int fontSize;
            int superScript;
            bool bold;
            bool italic;
            bool underline;
            bool strikeThrough;
            bool hidden;
            RtfColor backgroundColor;
            RtfColor foregroundColor;
            RtfTextAlignment::RtfTextAlignment alignment;

            QString toStr;


    }; // interface IRtfTextFormat

} // namespace Itenso.Rtf
// -- EOF -------------------------------------------------------------------
#endif //__RTFTEXTFORMAT_H