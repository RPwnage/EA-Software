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

#include "RtfFontBuilder.h"
#include "RtfSpec.h"
#include "RtfGroup.h"
#include "RtfTag.h"
#include "RtfText.h"

namespace RTF2HTML
{
    // ----------------------------------------------------------------------
    RtfFontBuilder::RtfFontBuilder(QObject* parent)
        : RtfElementVisitorBase ( RtfElementVisitorOrder::NonRecursive, parent )
    {
        // we iterate over our children ourselves -> hence non-recursive
        Reset();
    } // RtfFontBuilder

    // ----------------------------------------------------------------------
    QString& RtfFontBuilder::FontId ()
    {
        return fontId;
    } // FontId

    // ----------------------------------------------------------------------
    int RtfFontBuilder::FontIndex ()
    {
        return fontIndex;
    } // FontIndex

    // ----------------------------------------------------------------------
    int RtfFontBuilder::FontCharset ()
    {
        return fontCharset;
    } // FontCharset

    // ----------------------------------------------------------------------
    int RtfFontBuilder::FontCodePage ()
    {
        return fontCodePage;
    } // FontCodePage

    // ----------------------------------------------------------------------
    RtfFontKind::RtfFontKind RtfFontBuilder::FontKind ()
    {
        return fontKind;
    } // FontKind

    // ----------------------------------------------------------------------
    RtfFontPitch::RtfFontPitch RtfFontBuilder::FontPitch ()
    {
        return fontPitch;
    } // FontPitch

    // ----------------------------------------------------------------------
    QString RtfFontBuilder::FontName ()
    {
        QString fontName = "";

        int len = fontNameBuffer.length();
        if ( len > 0 && fontNameBuffer[ len - 1 ] == ';' )
        {
            fontName = fontNameBuffer.left (len-1);
        }
        return fontName;
    } // FontName

    // ----------------------------------------------------------------------
    RtfFont* RtfFontBuilder::CreateFont()
    {
        QString fontName = FontName ();
        if ( fontName.isEmpty())
        {
            fontName = "UnnamedFont_" + fontId;
        }
        return new RtfFont( fontId, fontKind, fontPitch,
                            fontCharset, fontCodePage, fontName, this );
    } // CreateFont

    // ----------------------------------------------------------------------
    void RtfFontBuilder::Reset()
    {
        fontIndex = 0;
        fontCharset = 0;
        fontCodePage = 0;
        fontKind = RtfFontKind::Nil;
        fontPitch = RtfFontPitch::Default;
        fontNameBuffer.clear();
    } // Reset

    // ----------------------------------------------------------------------
    void RtfFontBuilder::DoVisitGroup( RtfGroup* group )
    {
        QString dest = group->Destination();

        if (dest == RtfSpec::TagFont ||
                dest == RtfSpec::TagThemeFontLoMajor ||
                dest == RtfSpec::TagThemeFontHiMajor ||
                dest == RtfSpec::TagThemeFontDbMajor ||
                dest == RtfSpec::TagThemeFontBiMajor ||
                dest == RtfSpec::TagThemeFontLoMinor ||
                dest == RtfSpec::TagThemeFontHiMinor ||
                dest == RtfSpec::TagThemeFontDbMinor ||
                dest == RtfSpec::TagThemeFontBiMinor )
        {
            VisitGroupChildren( group );
        }
    } // DoVisitGroup

    // ----------------------------------------------------------------------
    void RtfFontBuilder::DoVisitTag( RtfTag* tag )
    {
        QString tagName = tag->Name();

        if (tagName == RtfSpec::TagThemeFontLoMajor ||
                tagName == RtfSpec::TagThemeFontHiMajor ||
                tagName == RtfSpec::TagThemeFontDbMajor ||
                tagName == RtfSpec::TagThemeFontBiMajor ||
                tagName == RtfSpec::TagThemeFontLoMinor ||
                tagName == RtfSpec::TagThemeFontHiMinor ||
                tagName == RtfSpec::TagThemeFontDbMinor ||
                tagName == RtfSpec::TagThemeFontBiMinor )
        {
            // skip and ignore for the moment
        }
        else if (tagName == RtfSpec::TagFont)
        {
            fontId = tag->FullName();
            fontIndex = tag->ValueAsNumber();
        }
        else if (tagName == RtfSpec::TagFontKindNil)
        {
            fontKind = RtfFontKind::Nil;
        }
        else if (tagName == RtfSpec::TagFontKindRoman)
        {
            fontKind = RtfFontKind::Roman;
        }
        else if (tagName == RtfSpec::TagFontKindSwiss)
        {
            fontKind = RtfFontKind::Swiss;
        }
        else if (tagName == RtfSpec::TagFontKindModern)
        {
            fontKind = RtfFontKind::Modern;
        }
        else if (tagName == RtfSpec::TagFontKindScript)
        {
            fontKind = RtfFontKind::Script;
        }
        else if (tagName == RtfSpec::TagFontKindDecor)
        {
            fontKind = RtfFontKind::Decor;
        }
        else if (tagName == RtfSpec::TagFontKindTech)
        {
            fontKind = RtfFontKind::Tech;
        }
        else if (tagName == RtfSpec::TagFontKindBidi)
        {
            fontKind = RtfFontKind::Bidi;
        }
        else if (tagName == RtfSpec::TagFontCharset)
        {
            fontCharset = tag->ValueAsNumber();
        }
        else if (tagName == RtfSpec::TagCodePage)
        {
            fontCodePage = tag->ValueAsNumber();
        }
        else if (tagName == RtfSpec::TagFontPitch)
        {
            switch ( tag->ValueAsNumber() )
            {
                case 0:
                    fontPitch = RtfFontPitch::Default;
                    break;
                case 1:
                    fontPitch = RtfFontPitch::Fixed;
                    break;
                case 2:
                    fontPitch = RtfFontPitch::Variable;
                    break;
            }
        }
    } // DoVisitTag

    // ----------------------------------------------------------------------
    void RtfFontBuilder::DoVisitText( RtfText* text )
    {
        fontNameBuffer += text->Text();
    } // DoVisitText

    // ----------------------------------------------------------------------

} // namespace Itenso.Rtf.Interpreter
// -- EOF -------------------------------------------------------------------
