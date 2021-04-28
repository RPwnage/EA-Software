// -- FILE ------------------------------------------------------------------
// name       : IRtfTextFormat.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.20
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using System.Text;
//using Itenso.Sys;

#include <assert.h>
#include <qtextstream.h>
#include "RtfTextFormat.h"
#include "RtfSpec.h"

namespace RTF2HTML
{

    QString RtfTextFormat::RtfTextAlignmentStr [] =
    {
        "Left",
        "Center",
        "Right",
        "Justify"
    };

    // ----------------------------------------------------------------------
    RtfTextFormat::RtfTextFormat( RtfFont* font, int fontSize, QObject* parent )
        : ObjectBase (parent)
        , alignment(RtfTextAlignment::Left)
    {
        if ( font == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "font" );
        }
        if ( fontSize <= 0 || fontSize > 0xFFFF )
        {
            assert (0);
            //throw new ArgumentException( Strings.FontSizeOutOfRange( fontSize ) );
        }

        this->font = font;
        this->fontSize = fontSize;

        //do some initialization
        superScript = 0;
        bold = false;
        italic = false;
        underline = false;
        strikeThrough = false;
        hidden = false;

        foregroundColor.setRGB(RtfColor::Black.Red(), RtfColor::Black.Green(), RtfColor::Black.Blue());
        backgroundColor.setRGB(RtfColor::White.Red(), RtfColor::White.Green(), RtfColor::White.Blue());
    } // RtfTextFormat

    // ----------------------------------------------------------------------
    RtfTextFormat::RtfTextFormat( RtfTextFormat* copy, QObject* parent )
        : ObjectBase (parent)
    {
        if ( copy == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "copy" );
        }

        font = copy->Font(); // enough because immutable
        fontSize = copy->FontSize ();
        superScript = copy->SuperScript ();
        bold = copy->IsBold ();
        italic = copy->IsItalic ();
        underline = copy->IsUnderline ();
        strikeThrough = copy->IsStrikeThrough ();
        hidden = copy->IsHidden ();
        backgroundColor.setRGB (copy->BackgroundColor()->Red(), copy->BackgroundColor()->Green(), copy->BackgroundColor()->Blue()); // enough because immutable
        foregroundColor.setRGB (copy->ForegroundColor()->Red(), copy->ForegroundColor()->Green(), copy->ForegroundColor()->Blue()); // enough because immutable
        alignment = copy->Alignment();
    } // RtfTextFormat

    // ----------------------------------------------------------------------
    QString RtfTextFormat::FontDescriptionDebug ()
    {
        QString desc (font->Name());
        QTextStream descStream (&desc);

        descStream << ", ";
        descStream << fontSize;

        if (superScript >= 0)
        {
            descStream << "+";
        }
        descStream << superScript;
        descStream << ", ";

        if ( bold || italic || underline || strikeThrough )
        {
            bool combined = false;
            if ( bold )
            {
                descStream << "bold";
                combined = true;
            }
            if ( italic )
            {
                if (combined)
                {
                    descStream << "+italic";
                }
                else
                {
                    descStream << "italic";
                }
                combined = true;
            }
            if ( underline )
            {
                if (combined)
                {
                    descStream << "+underline";
                }
                else
                {
                    descStream << "underline";
                }
                combined = true;
            }
            if ( strikeThrough )
            {
                if (combined)
                {
                    descStream << "+strikethrough";
                }
                else
                {
                    descStream << "strikethrough";
                }
            }
        }
        else
        {
            descStream << "plain";
        }
        if ( hidden )
        {
            descStream << ", hidden";
        }

        return desc;
    } // FontDescriptionDebug

    // ----------------------------------------------------------------------
    RtfFont* RtfTextFormat::Font ()
    {
        return font;
    } // Font

    // ----------------------------------------------------------------------
    RtfTextFormat* RtfTextFormat::DeriveWithFont( RtfFont* rtfFont )
    {
        if ( rtfFont == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "rtfFont" );
        }
        if ( font->Equals( rtfFont ) )
        {
            return this;
        }

        RtfTextFormat* copy = new RtfTextFormat( this, this->parent() );
        copy->font = rtfFont;
        return copy;
    } // DeriveWithFont

    // ----------------------------------------------------------------------
    int RtfTextFormat::FontSize ()
    {
        return fontSize;
    } // FontSize

    // ----------------------------------------------------------------------
    RtfTextFormat* RtfTextFormat::DeriveWithFontSize( int derivedFontSize )
    {
        if ( derivedFontSize < 0 || derivedFontSize > 0xFFFF )
        {
            assert (0);
            //throw new ArgumentException( Strings.FontSizeOutOfRange( derivedFontSize ) );
        }
        if ( fontSize == derivedFontSize )
        {
            return this;
        }

        RtfTextFormat* copy = new RtfTextFormat( this, this->parent() );
        copy->fontSize = derivedFontSize;
        return copy;
    } // DeriveWithFontSize

    // ----------------------------------------------------------------------
    int RtfTextFormat::SuperScript()
    {
        return superScript;
    } // SuperScript

    // ----------------------------------------------------------------------
    RtfTextFormat* RtfTextFormat::DeriveWithSuperScript( int deviation )
    {
        if ( superScript == deviation )
        {
            return this;
        }

        RtfTextFormat* copy = new RtfTextFormat( this, this->parent() );
        copy->superScript = deviation;
        // reset font size
        if ( deviation == 0 )
        {
            copy->fontSize = ( fontSize / 2 ) * 3;
        }
        return copy;
    } // DeriveWithSuperScript

    // ----------------------------------------------------------------------
    RtfTextFormat* RtfTextFormat::DeriveWithSuperScript( bool super )
    {
        RtfTextFormat* copy = new RtfTextFormat( this, this->parent() );
        //copy.fontSize = Math.Max( 1, ( fontSize * 2 ) / 3 );
        copy->fontSize = ( fontSize * 2 ) / 3 ;
        if (copy->fontSize < 1)
            copy->fontSize = 1;

        int maxsize = fontSize / 2;
        if (maxsize < 1)
            maxsize = 1;

        copy->superScript = ( super ? 1 : -1 ) * maxsize;
        return copy;
    } // DeriveWithSuperScript

    // ----------------------------------------------------------------------
    bool RtfTextFormat::IsNormal ()
    {
        bool isNormal = (!bold && !italic && !underline && !strikeThrough &&
                         !hidden &&
                         fontSize == RtfSpec::DefaultFontSize &&
                         superScript == 0 &&
                         RtfColor::Black.Equals( &foregroundColor ) &&
                         RtfColor::White.Equals( &backgroundColor ));
        return isNormal;
    } // IsNormal

    // ----------------------------------------------------------------------
    RtfTextFormat* RtfTextFormat::DeriveNormal()
    {
        if ( IsNormal () )
        {
            return this;
        }

        RtfTextFormat* copy = new RtfTextFormat( font, RtfSpec::DefaultFontSize, this->parent() );
        copy->alignment = alignment; // this is a paragraph property, keep it
        return copy;
    } // DeriveNormal

    // ----------------------------------------------------------------------
    bool RtfTextFormat::IsBold ()
    {
        return bold;
    } // IsBold

    // ----------------------------------------------------------------------
    RtfTextFormat* RtfTextFormat::DeriveWithBold( bool derivedBold )
    {
        if ( bold == derivedBold )
        {
            return this;
        }

        RtfTextFormat* copy = new RtfTextFormat( this, this->parent() );
        copy->bold = derivedBold;
        return copy;
    } // DeriveWithBold

    // ----------------------------------------------------------------------
    bool RtfTextFormat::IsItalic ()
    {
        return italic;
    } // IsItalic

    // ----------------------------------------------------------------------
    RtfTextFormat* RtfTextFormat::DeriveWithItalic( bool derivedItalic )
    {
        if ( italic == derivedItalic )
        {
            return this;
        }

        RtfTextFormat* copy = new RtfTextFormat( this, this->parent() );
        copy->italic = derivedItalic;
        return copy;
    } // DeriveWithItalic

    // ----------------------------------------------------------------------
    bool RtfTextFormat::IsUnderline ()
    {
        return underline;
    } // IsUnderline

    // ----------------------------------------------------------------------
    RtfTextFormat* RtfTextFormat::DeriveWithUnderline( bool derivedUnderline )
    {
        if ( underline == derivedUnderline )
        {
            return this;
        }

        RtfTextFormat* copy = new RtfTextFormat( this, this->parent() );
        copy->underline = derivedUnderline;
        return copy;
    } // DeriveWithUnderline

    // ----------------------------------------------------------------------
    bool RtfTextFormat::IsStrikeThrough ()
    {
        return strikeThrough;
    } // IsStrikeThrough

    // ----------------------------------------------------------------------
    RtfTextFormat* RtfTextFormat::DeriveWithStrikeThrough( bool derivedStrikeThrough )
    {
        if ( strikeThrough == derivedStrikeThrough )
        {
            return this;
        }

        RtfTextFormat* copy = new RtfTextFormat( this, this->parent() );
        copy->strikeThrough = derivedStrikeThrough;
        return copy;
    } // DeriveWithStrikeThrough

    // ----------------------------------------------------------------------
    bool RtfTextFormat::IsHidden ()
    {
        return hidden;
    } // IsHidden

    // ----------------------------------------------------------------------
    RtfTextFormat* RtfTextFormat::DeriveWithHidden( bool derivedHidden )
    {
        if ( hidden == derivedHidden )
        {
            return this;
        }

        RtfTextFormat* copy = new RtfTextFormat( this, this->parent() );
        copy->hidden = derivedHidden;
        return copy;
    } // DeriveWithHidden

    // ----------------------------------------------------------------------
    RtfColor* RtfTextFormat::BackgroundColor ()
    {
        return &backgroundColor;
    } // BackgroundColor

    // ----------------------------------------------------------------------
    RtfTextFormat* RtfTextFormat::DeriveWithBackgroundColor( RtfColor* derivedBackgroundColor )
    {
        if ( derivedBackgroundColor == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "derivedBackgroundColor" );
        }
        if ( backgroundColor.Equals( derivedBackgroundColor ) )
        {
            return this;
        }

        RtfTextFormat* copy = new RtfTextFormat( this, this->parent() );
        copy->backgroundColor.setRGB (derivedBackgroundColor->Red(), derivedBackgroundColor->Green(), derivedBackgroundColor->Blue());
        return copy;
    } // DeriveWithBackgroundColor

    // ----------------------------------------------------------------------
    RtfColor* RtfTextFormat::ForegroundColor ()
    {
        return &foregroundColor;
    } // ForegroundColor

    // ----------------------------------------------------------------------
    RtfTextFormat* RtfTextFormat::DeriveWithForegroundColor( RtfColor* derivedForegroundColor )
    {
        if ( derivedForegroundColor == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "derivedForegroundColor" );
        }
        if ( foregroundColor.Equals( derivedForegroundColor ) )
        {
            return this;
        }

        RtfTextFormat* copy = new RtfTextFormat( this, this->parent() );
        copy->foregroundColor.setRGB (derivedForegroundColor->Red(), derivedForegroundColor->Green(), derivedForegroundColor->Blue());
        return copy;
    } // DeriveWithForegroundColor

    // ----------------------------------------------------------------------
    RtfTextAlignment::RtfTextAlignment RtfTextFormat::Alignment ()
    {
        return alignment;
    } // Alignment

    // ----------------------------------------------------------------------
    RtfTextFormat* RtfTextFormat::DeriveWithAlignment( RtfTextAlignment::RtfTextAlignment derivedAlignment )
    {
        if ( alignment == derivedAlignment )
        {
            return this;
        }

        RtfTextFormat* copy = new RtfTextFormat( this, this->parent() );
        copy->alignment = derivedAlignment;
        return copy;
    } // DeriveWithForegroundColor

    // ----------------------------------------------------------------------
    RtfTextFormat* RtfTextFormat::Duplicate()
    {
        return new RtfTextFormat( this, this->parent() );
    } // IRtfTextFormat.Duplicate

    // ----------------------------------------------------------------------
    bool RtfTextFormat::Equals( RTF2HTML::ObjectBase*  obj )
    {
        RtfTextFormat* tf = static_cast<RtfTextFormat*>(obj);
        if ( this == tf )
        {
            return true;
        }

        if ( obj == NULL || (GetType() != tf->GetType()))
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
    bool RtfTextFormat::IsEqual( RTF2HTML::ObjectBase*  obj )
    {
        RtfTextFormat* tf = static_cast<RtfTextFormat*>(obj);
        return
            font->Equals( tf->font ) &&
            fontSize == tf->fontSize &&
            superScript == tf->superScript &&
            bold == tf->bold &&
            italic == tf->italic &&
            underline == tf->underline &&
            strikeThrough == tf->strikeThrough &&
            hidden == tf->hidden &&
            backgroundColor.Equals( &(tf->backgroundColor) ) &&
            foregroundColor.Equals( &(tf->foregroundColor) ) &&
            alignment == tf->alignment;
    } // IsEqual

#if 0 //UNIMPLEMENTED currently NOT USED
    // ----------------------------------------------------------------------
    private int ComputeHashCode()
    {
        int hash = font.GetHashCode();
        hash = HashTool.AddHashCode( hash, fontSize );
        hash = HashTool.AddHashCode( hash, superScript );
        hash = HashTool.AddHashCode( hash, bold );
        hash = HashTool.AddHashCode( hash, italic );
        hash = HashTool.AddHashCode( hash, underline );
        hash = HashTool.AddHashCode( hash, strikeThrough );
        hash = HashTool.AddHashCode( hash, hidden );
        hash = HashTool.AddHashCode( hash, backgroundColor );
        hash = HashTool.AddHashCode( hash, foregroundColor );
        hash = HashTool.AddHashCode( hash, alignment );
        return hash;
    } // ComputeHashCode
#endif
    // ----------------------------------------------------------------------
    QString& RtfTextFormat::ToString()
    {
        toStr.clear();
        QTextStream descStream (&toStr);

        descStream << "Font ";
        descStream << FontDescriptionDebug ();
        descStream << ", ";
        descStream << RtfTextFormat::RtfTextAlignmentStr [alignment];
        descStream << ", ";
        descStream << foregroundColor.ToString();
        descStream << " on ";
        descStream << backgroundColor.ToString();

        return toStr;
    } // ToString
} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
