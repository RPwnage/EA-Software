// -- FILE ------------------------------------------------------------------
// name       : RtfFont.cs
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
#include "RtfFont.h"
#include "RtfSpec.h"

namespace RTF2HTML
{

    // ----------------------------------------------------------------------
    RtfFont::RtfFont( QString id, RtfFontKind::RtfFontKind kind, RtfFontPitch::RtfFontPitch pitch, int charSet, int codePage, QString name, QObject* parent )
        : ObjectBase (parent)
    {
        if ( id.isEmpty() )
        {
            assert (0);
            //throw new ArgumentNullException( "id" );
        }
        if ( charSet < 0 )
        {
            assert (0);
            //throw new ArgumentException( Strings.InvalidCharacterSet( charSet ) );
        }
        if ( codePage < 0 )
        {
            assert (0);
            //throw new ArgumentException( Strings.InvalidCodePage( codePage ) );
        }
        if ( name.isEmpty() )
        {
            assert (0);
            //throw new ArgumentNullException( "name" );
        }

        this->id = id;
        this->kind = kind;
        this->pitch = pitch;
        this->charSet = charSet;
        this->codePage = codePage;
        this->name = name;
    } // RtfFont

    // ----------------------------------------------------------------------
    RtfFont::~RtfFont ()
    {
    }
    // ----------------------------------------------------------------------
    QString& RtfFont::Id ()
    {
        return id;
    } // Id

    // ----------------------------------------------------------------------
    RtfFontKind::RtfFontKind RtfFont::Kind ()
    {
        return kind;
    } // Kind

    // ----------------------------------------------------------------------
    RtfFontPitch::RtfFontPitch RtfFont::Pitch()
    {
        return pitch;
    } // Pitch

    // ----------------------------------------------------------------------
    int RtfFont::CharSet()
    {
        return charSet;
    } // CharSet

    // ----------------------------------------------------------------------
    int RtfFont::CodePage()
    {
        if ( codePage == 0 )
        {
            // unspecified codepage: use the one derived from the charset:
            return RtfSpec::GetCodePage( charSet );
        }
        return codePage;
    } // CodePage

#if 0 //UNIMPLEMENTED currently NOT USED, doesn't seem to be used so leave it off for now
    // ----------------------------------------------------------------------
    Encoding GetEncoding()
    {
        return Encoding.GetEncoding( CodePage );
    } // GetEncoding
#endif

    // ----------------------------------------------------------------------
    QString& RtfFont::Name()
    {
        return name;
    } // Name

    // ----------------------------------------------------------------------
    bool RtfFont::Equals( RTF2HTML::ObjectBase*  obj )
    {
        RtfFont* font = static_cast<RtfFont*>(obj);
        if ( this == font )
        {
            return true;
        }

        if ( obj == NULL || (GetType() != font->GetType()))
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
    QString& RtfFont::ToString()
    {
        toStr = id + ":" + name;
        return toStr;
    } // ToString

    // ----------------------------------------------------------------------
    bool RtfFont::IsEqual( RTF2HTML::ObjectBase*  obj )
    {

        if (obj != NULL)
        {
            RtfFont* font = static_cast<RtfFont*>(obj);
            if ((font->id == id) &&
                    (font->kind == kind) &&
                    (font->pitch == pitch) &&
                    (font->charSet == charSet) &&
                    (font->codePage == codePage) &&
                    (font->name == name))
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
        int hash = id.GetHashCode();
        hash = HashTool.AddHashCode( hash, kind );
        hash = HashTool.AddHashCode( hash, pitch );
        hash = HashTool.AddHashCode( hash, charSet );
        hash = HashTool.AddHashCode( hash, codePage );
        hash = HashTool.AddHashCode( hash, name );
        return hash;
    } // ComputeHashCode
#endif

} // namespace Itenso.Rtf
// -- EOF -------------------------------------------------------------------
