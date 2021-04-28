// -- FILE ------------------------------------------------------------------
// name       : RtfColor.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.21
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System.Drawing;
//using Itenso.Sys;

#include <assert.h>
#include "RtfColor.h"


namespace RTF2HTML
{
    RtfColor RtfColor::Black(0, 0, 0);
    RtfColor RtfColor::White (255, 255, 255);

    RtfColor::RtfColor (QObject* parent)
        :ObjectBase (parent)
    {
        setRGB (0, 0, 0);
    }

    // ----------------------------------------------------------------------
    RtfColor::RtfColor( int red, int green, int blue, QObject* parent )
        : ObjectBase (parent)
    {
        if ( red < 0 || red > 255 )
        {
            assert (0);
            //throw new RtfColorException( Strings.InvalidColor( red ) );
        }
        if ( green < 0 || green > 255 )
        {
            assert (0);
            //throw new RtfColorException( Strings.InvalidColor( green ) );
        }
        if ( blue < 0 || blue > 255 )
        {
            assert (0);
            //throw new RtfColorException( Strings.InvalidColor( blue ) );
        }

        setRGB (red, green, blue);
    } // RtfColor

    // ----------------------------------------------------------------------
    RtfColor::~RtfColor ()
    {
    }

    // ----------------------------------------------------------------------
    int RtfColor::Red ()
    {
        return red;
    } // Red

    // ----------------------------------------------------------------------
    int RtfColor::Green ()
    {
        return green;
    } // Green

    // ----------------------------------------------------------------------
    int RtfColor::Blue ()
    {
        return blue;
    } // Blue

    // ----------------------------------------------------------------------
    QColor& RtfColor::AsDrawingColor ()
    {
        return drawingColor;
    } // AsDrawingColor
    // ----------------------------------------------------------------------
    void RtfColor::setRGB (int red, int green, int blue)
    {
        this->red = red;
        this->green = green;
        this->blue = blue;
        drawingColor.setRgb (red, green, blue);
    }

    // ----------------------------------------------------------------------
    bool RtfColor::Equals( RTF2HTML::ObjectBase*  obj )
    {
        RtfColor* color = static_cast<RtfColor*>(obj);
        if ( color == this )
        {
            return true;
        }

        if ( obj == NULL || (GetType() != color->GetType()))
        {
            return false;
        }

        return IsEqual( obj );
    } // Equals

#if 0 //UNIMPLEMENTED currently NOT USED
    // ----------------------------------------------------------------------
    public override int GetHashCode()
    {
        return HashTool.AddHashCode( GetType().GetHashCode(), ComputeHashCode() );
    } // GetHashCode
#endif

    // ----------------------------------------------------------------------
    QString& RtfColor::ToString()
    {
        toStr = QString("Color{%0,%1,%2}").arg(red).arg(green).arg(blue);
        return toStr;
    } // ToString

    // ----------------------------------------------------------------------
    bool RtfColor::IsEqual( RTF2HTML::ObjectBase*  obj )
    {

        if (obj != NULL)
        {
            RtfColor* color = static_cast<RtfColor*>(obj);
            if ((color->red == red) &&
                    (color->green == green) &&
                    (color->blue == blue))
            {
                return true;
            }
        }
        return false;
    } // IsEqual

#if 0 //UNIMPLEMENTED currently NOT USED
    // ----------------------------------------------------------------------
    private int ComputeHashCode()
    {
        int hash = red;
        hash = HashTool.AddHashCode( hash, green );
        hash = HashTool.AddHashCode( hash, blue );
        return hash;
    } // ComputeHashCode
#endif
} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
