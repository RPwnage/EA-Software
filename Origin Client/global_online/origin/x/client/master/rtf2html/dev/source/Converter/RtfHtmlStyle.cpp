// -- FILE ------------------------------------------------------------------
// name       : RtfHtmlStyle.cs
// project    : RTF Framelet
// created    : Jani Giannoudis - 2008.06.02
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using Itenso.Sys;

#include "RtfHtmlStyle.h"

namespace RTF2HTML
{
    RtfHtmlStyle::RtfHtmlStyle (QObject* parent)
        :ObjectBase (parent)
    {
    }

    // ----------------------------------------------------------------------
    QString& RtfHtmlStyle::getForegroundColor ()
    {
        return foregroundColor;
    }

    void RtfHtmlStyle::setForegroundColor (QString& value)
    {
        foregroundColor = value;
    } // ForegroundColor

    // ----------------------------------------------------------------------
    QString& RtfHtmlStyle::getBackgroundColor ()
    {
        return backgroundColor;
    }

    void RtfHtmlStyle::setBackgroundColor (QString& value)
    {
        backgroundColor = value;
    } // BackgroundColor

    // ----------------------------------------------------------------------
    QString& RtfHtmlStyle::getFontFamily()
    {
        return fontFamily;
    }

    void RtfHtmlStyle::setFontFamily (QString& value)
    {
        fontFamily = value;
    } // FontFamily

    // ----------------------------------------------------------------------
    QString& RtfHtmlStyle::getFontSize ()
    {
        return fontSize;
    }

    void RtfHtmlStyle::setFontSize(QString& value)
    {
        fontSize = value;
    } // FontSize

    // ----------------------------------------------------------------------
    bool RtfHtmlStyle::IsEmpty ()
    {
        //return Equals( Empty ); }
        if (foregroundColor.isEmpty() && backgroundColor.isEmpty() &&
                fontSize.isEmpty() && fontFamily.isEmpty())
        {
            return true;
        }
        return false;
    } // IsEmpty

    // ----------------------------------------------------------------------
    bool RtfHtmlStyle::Equals( ObjectBase* obj )
    {
        RtfHtmlStyle* hs = static_cast<RtfHtmlStyle*>(obj);

        if ( hs == this )
        {
            return true;
        }

        if ( obj == NULL || GetType() != hs->GetType() )
        {
            return false;
        }

        return IsEqual( obj );
    } // Equals

#if 0 //UNIMPLEMENTED, currently NOT USED
    // ----------------------------------------------------------------------
    public sealed override int GetHashCode()
    {
        return HashTool.AddHashCode( GetType().GetHashCode(), ComputeHashCode() );
    } // GetHashCode
#endif
    // ----------------------------------------------------------------------
    bool RtfHtmlStyle::IsEqual( ObjectBase* obj )
    {
        RtfHtmlStyle* hs = static_cast<RtfHtmlStyle*>(obj);

        return (
                   hs != NULL &&
                   foregroundColor == hs->foregroundColor &&
                   backgroundColor == hs->backgroundColor  &&
                   fontFamily == hs->fontFamily &&
                   fontSize == hs->fontSize );
    } // IsEqual

#if 0 //UNIMPLEMENTED, currently NOT USED
    // ----------------------------------------------------------------------
    private int ComputeHashCode()
    {
        int hash = foregroundColor.GetHashCode();
        hash = HashTool.AddHashCode( hash, backgroundColor );
        hash = HashTool.AddHashCode( hash, fontFamily );
        hash = HashTool.AddHashCode( hash, fontSize );
        return hash;
    } // ComputeHashCode
#endif
} // namespace Itenso.Rtf.Converter.Html
// -- EOF -------------------------------------------------------------------
