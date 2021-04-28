// -- FILE ------------------------------------------------------------------
// name       : RtfInterpreter.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.20
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;

#include <assert.h>
#include "RtfInterpreter.h"
#include "RtfSpec.h"
#include "RtfTag.h"
#include "RtfText.h"

namespace RTF2HTML
{
    // ----------------------------------------------------------------------
    RtfInterpreter::RtfInterpreter( RtfInterpreterListenerList& listeners, QObject* parent )
        : RtfInterpreterBase (listeners, parent)
    {
        RtfInterpreterContext* context = Context();
        fontTableBuilder = new RtfFontTableBuilder( context->WritableFontTable(), true /*ignoreDuplicatedFonts*/, (RtfInterpreterBase*)this);
        colorTableBuilder = new RtfColorTableBuilder( context->WritableColorTable(), (RtfInterpreterBase*)this);
        documentInfoBuilder = new RtfDocumentInfoBuilder( context->WritableDocumentInfo(), (RtfInterpreterBase*)this );
        userPropertyBuilder = new RtfUserPropertyBuilder( context->WritableUserProperties(), (RtfInterpreterBase*)this );
//      imageBuilder = new RtfImageBuilder();
    } // RtfInterpreter

    RtfInterpreter::~RtfInterpreter ()
    {
    }

    // ----------------------------------------------------------------------
#if 0 //UNIMPLEMENTED currently NOT USED

    public RtfInterpreter( IRtfInterpreterSettings settings, params IRtfInterpreterListener[] listeners ) :
        base( settings, listeners )
    {
        fontTableBuilder = new RtfFontTableBuilder( Context.WritableFontTable, settings.IgnoreDuplicatedFonts );
        colorTableBuilder = new RtfColorTableBuilder( Context.WritableColorTable );
        documentInfoBuilder = new RtfDocumentInfoBuilder( Context.WritableDocumentInfo );
        userPropertyBuilder = new RtfUserPropertyBuilder( Context.WritableUserProperties );
        imageBuilder = new RtfImageBuilder();
    } // RtfInterpreter
#endif
    // ----------------------------------------------------------------------
    bool RtfInterpreter::IsSupportedDocument( RtfGroup* rtfDocument )
    {
        //try
        //{
        RtfGroup* grp;
        grp = GetSupportedDocument( rtfDocument );
        return (grp != NULL);
        //}
        //catch ( RtfException )
        //{
        //  return false;
        //}
        //return true;
    } // IsSupportedDocument

    // ----------------------------------------------------------------------
    RtfGroup* RtfInterpreter::GetSupportedDocument( RtfGroup* rtfDocument )
    {
        if ( rtfDocument == NULL )
        {
            return NULL;
            //throw new ArgumentNullException( "rtfDocument" );
        }
        if ( rtfDocument->Contents().Count() == 0 )
        {
            return NULL;
            //throw new RtfEmptyDocumentException( Strings.EmptyDocument );
        }
        RtfElement* firstElement = rtfDocument->Contents().Get(0);
        if ( firstElement->Kind() != RtfElementKind::Tag )
        {
            return NULL;
            //throw new RtfStructureException( Strings.MissingDocumentStartTag );
        }
        RtfTag* firstTag = (RtfTag*)firstElement;
        if ( RtfSpec::TagRtf != firstTag->Name() )
        {
            return NULL;
            //throw new RtfStructureException( Strings.InvalidDocumentStartTag( RtfSpec.TagRtf ) );
        }
        if ( !firstTag->HasValue() )
        {
            return NULL;
            //throw new RtfUnsupportedStructureException( Strings.MissingRtfVersion );
        }
        if ( firstTag->ValueAsNumber() != RtfSpec::RtfVersion1 )
        {
            return NULL;
            //throw new RtfUnsupportedStructureException( Strings.UnsupportedRtfVersion( firstTag.ValueAsNumber ) );
        }
        return rtfDocument;
    } // GetSupportedDocument

    // ----------------------------------------------------------------------
    void RtfInterpreter::DoInterpret( RtfGroup* rtfDocument )
    {
        InterpretContents( GetSupportedDocument( rtfDocument ) );
    } // DoInterpret

    // ----------------------------------------------------------------------
    void RtfInterpreter::InterpretContents( RtfGroup* rtfDocument )
    {
        // by getting here we already know that the given document is supported, and hence
        // we know it has version 1
        Context()->Reset(); // clears all previous content and sets the version to 1
        lastGroupWasPictureWrapper = false;
        NotifyBeginDocument();
        VisitChildrenOf( rtfDocument );
        Context()->setState(RtfInterpreterState::Ended);
        NotifyEndDocument();
    } // InterpretContents

    // ----------------------------------------------------------------------
    void RtfInterpreter::VisitChildrenOf( RtfGroup* group )
    {
        bool pushedTextFormat = false;
        if ( Context()->getState() == RtfInterpreterState::InDocument )
        {
            Context()->PushCurrentTextFormat();
            pushedTextFormat = true;
        }
        //try
        //{
        int childCount = group->Contents().Count();
        for ( int i = 0; i < childCount; i++ ) // skip over the initial \fonttbl tag
        {
            group->Contents().Get(i)->Visit( this );
        }
        //}
        //finally
        //{
        if ( pushedTextFormat )
        {
            Context()->PopCurrentTextFormat();
        }
        //}
    } // VisitChildrenOf

    QString RtfInterpreter::ExtractHyperlink (RtfGroup* group)
    {
        QString testHyperlink;

        // 0 = \*
        // 1 = \fldinst
        // 2 = start of group def that may contain HYPERLINK
        int count = group->Contents().Count();

        for (int i = 2; i < count; i++)
        {
            RtfGroup* child = static_cast<RtfGroup*>(group->Contents().Get(i));

            int numGC = child->Contents().Count();
            for ( int j = 0; j < numGC; j++ )
            {
                RtfElement* gc = child->Contents().Get(j);
                if (gc->Kind() == RtfElementKind::Text)
                {
                    testHyperlink += gc->ToString();
                }
            }
        }


        QString fullStr = testHyperlink.trimmed();
        if (fullStr.indexOf("HYPERLINK") == 0)
        {
            //there might be extraneous comment like stuff, so we only want HYPERLINK "xxx"
            int firstquote = fullStr.indexOf('"');
            if (firstquote >= 0)
            {
                int secondquote = fullStr.indexOf('"', firstquote + 1);
                if (secondquote > 0 && secondquote < (fullStr.length() - 1))
                {
                    QString trimmedStr = fullStr.left(secondquote+1);
                    return trimmedStr;
                }
            }
            return fullStr; //just return string as is
        }
        return QString();
    }

    QString RtfInterpreter::ExtractHyperlinkVisual (RtfGroup* group)
    {
        QString testHyperlinkVisual;

        int count = group->Contents().Count();

        for (int i = 1; i < count; i++)
        {
            RtfGroup* child = static_cast<RtfGroup*>(group->Contents().Get(i));

            int numGC = child->Contents().Count();

            for (int j = 0; j < numGC; j++)
            {
                RtfElement* gc = child->Contents().Get (j);
                if (gc->Kind() == RtfElementKind::Text)
                {
                    //collect the hyperlink visual text
                    testHyperlinkVisual += gc->ToString();
                }
                else
                {
                    //process the style for hyperlink visual text
                    gc->Visit(this);
                }
            }

        }
        return testHyperlinkVisual;
    }

    // ----------------------------------------------------------------------
    // RtfElementVisitor portion of the class
    // ----------------------------------------------------------------------
    void RtfInterpreter::VisitTag( RtfTag* tag )
    {
        if ( Context()->getState() != RtfInterpreterState::InDocument )
        {
            if ( Context()->FontTable()->Count() > 0 )
            {
                // somewhat of a hack to detect state change from header to in-document for
                // rtf-docs which do neither have a generator group nor encapsulate the
                // actual document content in a group.
                if ( Context()->ColorTable()->Count() > 0 || RtfSpec::TagViewKind == tag->Name() )
                {
                    Context()->setState(RtfInterpreterState::InDocument);
                }
            }
        }

        RtfInterpreterState::RtfInterpreterState state = Context()->getState();
        switch ( state )
        {
            case RtfInterpreterState::Init:
                {
                    if ( RtfSpec::TagRtf ==  tag->Name() )
                    {
                        Context()->setState (RtfInterpreterState::InHeader);
                        Context()->setRtfVersion (tag->ValueAsNumber());
                    }
                    else
                    {
                        assert (0);
                        //throw new RtfStructureException( Strings.InvalidInitTagState( tag.ToString() ) );
                    }
                }
                break;

            case RtfInterpreterState::InHeader:
                {
                    if (tag->Name () == RtfSpec::TagDefaultFont)
                    {
                        QString fontId = QString("%1%2").arg(RtfSpec::TagFont).arg(tag->ValueAsNumber());
                        Context()->setDefaultFontId (fontId);
                    }
                }
                break;

            case RtfInterpreterState::InDocument:
                {
                    QString tagName = tag->Name();
                    if (tagName == RtfSpec::TagPlain)
                    {
                        Context()->setWritableCurrentTextFormat(Context()->getWritableCurrentTextFormat()->DeriveNormal());
                    }
                    else if (tagName == RtfSpec::TagParagraphDefaults ||
                             tagName == RtfSpec::TagSectionDefaults)
                    {
                        Context()->setWritableCurrentTextFormat (Context()->getWritableCurrentTextFormat()->DeriveWithAlignment( RtfTextAlignment::Left ));
                    }
                    else if (tagName == RtfSpec::TagBold)
                    {
                        bool bold = (!tag->HasValue() || (tag->ValueAsNumber() != 0));
                        Context()->setWritableCurrentTextFormat (Context()->getWritableCurrentTextFormat()->DeriveWithBold( bold ));
                    }
                    else if (tagName == RtfSpec::TagItalic)
                    {
                        bool italic = (!tag->HasValue() || (tag->ValueAsNumber() != 0));
                        Context()->setWritableCurrentTextFormat (Context()->getWritableCurrentTextFormat()->DeriveWithItalic( italic ));
                    }
                    else if (tagName == RtfSpec::TagUnderLine)
                    {
                        bool underline = (!tag->HasValue() || (tag->ValueAsNumber() != 0));
                        Context()->setWritableCurrentTextFormat (Context()->getWritableCurrentTextFormat()->DeriveWithUnderline( underline ));
                    }
                    else if (tagName == RtfSpec::TagUnderLineNone)
                    {
                        Context()->setWritableCurrentTextFormat (Context()->getWritableCurrentTextFormat()->DeriveWithUnderline( false ));
                    }
                    else if (tagName == RtfSpec::TagStrikeThrough)
                    {
                        bool strikeThrough = (!tag->HasValue() || (tag->ValueAsNumber() != 0));
                        Context()->setWritableCurrentTextFormat (Context()->getWritableCurrentTextFormat()->DeriveWithStrikeThrough( strikeThrough ));
                    }
                    else if (tagName == RtfSpec::TagHidden)
                    {
                        bool hidden = (!tag->HasValue() || (tag->ValueAsNumber() != 0));
                        Context()->setWritableCurrentTextFormat (Context()->getWritableCurrentTextFormat()->DeriveWithHidden( hidden ));
                    }
                    else if (tagName == RtfSpec::TagFont)
                    {
                        QString& fontId = tag->FullName();
                        if ( Context()->FontTable()->ContainsFontWithId( fontId ) )
                        {
                            Context()->setWritableCurrentTextFormat (Context()->getWritableCurrentTextFormat()->DeriveWithFont(Context()->FontTable()->Get( fontId )));
                        }
                        else
                        {
#if 0 //UNIMPLEMENTED currently NOT USED, ignoreunknown fonts = false
                            if ( Settings.IgnoreUnknownFonts && Context.FontTable.Count > 0 )
                            {
                                Context.WritableCurrentTextFormat =
                                    Context.WritableCurrentTextFormat.DeriveWithFont( Context.FontTable[ 0 ] );
                            }
                            else
#endif
                            {
                                assert (0);
                                //throw new RtfUndefinedFontException( Strings.UndefinedFont( fontId ) );
                            }
                        }
                    }
                    else if (tagName == RtfSpec::TagFontSize)
                    {
                        int fontSize = tag->ValueAsNumber();
                        if ( fontSize >= 0 )
                        {
                            Context()->setWritableCurrentTextFormat (Context()->getWritableCurrentTextFormat()->DeriveWithFontSize( fontSize ));
                        }
                        else
                        {
                            assert (0);
                            //throw new RtfInvalidDataException( Strings.InvalidFontSize( fontSize ) );
                        }
                    }
                    else if (tagName == RtfSpec::TagFontSubscript)
                    {
                        Context()->setWritableCurrentTextFormat (Context()->getWritableCurrentTextFormat()->DeriveWithSuperScript( false ));
                    }
                    else if (tagName == RtfSpec::TagFontSuperscript)
                    {
                        Context()->setWritableCurrentTextFormat (Context()->getWritableCurrentTextFormat()->DeriveWithSuperScript( true ));
                        \
                    }
                    else if (tagName == RtfSpec::TagFontNoSuperSub)
                    {
                        Context()->setWritableCurrentTextFormat (Context()->getWritableCurrentTextFormat()->DeriveWithSuperScript( 0 ));
                    }
                    else if (tagName == RtfSpec::TagFontDown)
                    {
                        int moveDown = tag->ValueAsNumber();
                        if ( moveDown == 0 )
                        {
                            moveDown = 6; // the default value according to rtf spec
                        }
                        Context()->setWritableCurrentTextFormat (Context()->getWritableCurrentTextFormat()->DeriveWithSuperScript( -moveDown ));
                    }
                    else if (tagName == RtfSpec::TagFontUp)
                    {
                        int moveUp = tag->ValueAsNumber();
                        if ( moveUp == 0 )
                        {
                            moveUp = 6; // the default value according to rtf spec
                        }
                        Context()->setWritableCurrentTextFormat (Context()->getWritableCurrentTextFormat()->DeriveWithSuperScript( moveUp ));
                    }
                    else if (tagName == RtfSpec::TagAlignLeft)
                    {
                        Context()->setWritableCurrentTextFormat (Context()->getWritableCurrentTextFormat()->DeriveWithAlignment( RtfTextAlignment::Left ));
                    }
                    else if (tagName == RtfSpec::TagAlignCenter)
                    {
                        Context()->setWritableCurrentTextFormat (Context()->getWritableCurrentTextFormat()->DeriveWithAlignment( RtfTextAlignment::Center ));
                    }
                    else if (tagName == RtfSpec::TagAlignRight)
                    {
                        Context()->setWritableCurrentTextFormat (Context()->getWritableCurrentTextFormat()->DeriveWithAlignment( RtfTextAlignment::Right ));
                    }
                    else if (tagName == RtfSpec::TagAlignJustify)
                    {
                        Context()->setWritableCurrentTextFormat (Context()->getWritableCurrentTextFormat()->DeriveWithAlignment( RtfTextAlignment::Justify ));
                    }
                    else if (tagName == RtfSpec::TagColorBackground ||
                             tagName == RtfSpec::TagColorBackgroundWord ||
                             tagName == RtfSpec::TagColorHighlight ||
                             tagName == RtfSpec::TagColorForeground)
                    {
                        int colorIndex = tag->ValueAsNumber();
                        if ( colorIndex >= 0 && colorIndex < Context()->ColorTable()->Count() )
                        {
                            RtfColor* newColor = Context()->ColorTable()->Get( colorIndex );
                            bool isForeground = (RtfSpec::TagColorForeground == tagName );
                            RtfTextFormat* tformat;
                            if (isForeground)
                            {
                                tformat = Context()->getWritableCurrentTextFormat()->DeriveWithForegroundColor( newColor );
                            }
                            else
                            {
                                tformat = Context()->getWritableCurrentTextFormat()->DeriveWithBackgroundColor( newColor );
                            }
                            Context()->setWritableCurrentTextFormat (tformat);
                        }
                        else
                        {
                            assert (0);
                            //throw new RtfUndefinedColorException( Strings.UndefinedColor( colorIndex ) );
                        }
                    }
                    else if (tagName == RtfSpec::TagSection)
                    {
                        NotifyInsertBreak( RtfVisualBreakKind::Section );
                    }
                    else if (tagName == RtfSpec::TagParagraph)
                    {
                        NotifyInsertBreak( RtfVisualBreakKind::Paragraph );
                    }
                    else if (tagName == RtfSpec::TagLine)
                    {
                        NotifyInsertBreak( RtfVisualBreakKind::Line );
                    }
                    else if (tagName == RtfSpec::TagPage)
                    {
                        NotifyInsertBreak( RtfVisualBreakKind::Page );
                    }
                    else if (tagName == RtfSpec::TagTabulator)
                    {
                        NotifyInsertSpecialChar( RtfVisualSpecialCharKind::Tabulator );
                    }
                    else if (tagName == RtfSpec::TagTilde)
                    {
                        NotifyInsertSpecialChar( RtfVisualSpecialCharKind::NonBreakingSpace );
                    }
                    else if (tagName == RtfSpec::TagEmDash)
                    {
                        NotifyInsertSpecialChar( RtfVisualSpecialCharKind::EmDash );
                    }
                    else if (tagName == RtfSpec::TagEnDash)
                    {
                        NotifyInsertSpecialChar( RtfVisualSpecialCharKind::EnDash );
                    }
                    else if (tagName == RtfSpec::TagEmSpace)
                    {
                        NotifyInsertSpecialChar( RtfVisualSpecialCharKind::EmSpace );
                    }
                    else if (tagName == RtfSpec::TagEnSpace)
                    {
                        NotifyInsertSpecialChar( RtfVisualSpecialCharKind::EnSpace );
                    }
                    else if (tagName == RtfSpec::TagQmSpace)
                    {
                        NotifyInsertSpecialChar( RtfVisualSpecialCharKind::QmSpace );
                    }
                    else if (tagName == RtfSpec::TagBulltet)
                    {
                        NotifyInsertSpecialChar( RtfVisualSpecialCharKind::Bullet );
                    }
                    else if (tagName == RtfSpec::TagLeftSingleQuote)
                    {
                        NotifyInsertSpecialChar( RtfVisualSpecialCharKind::LeftSingleQuote );
                    }
                    else if (tagName == RtfSpec::TagRightSingleQuote)
                    {
                        NotifyInsertSpecialChar( RtfVisualSpecialCharKind::RightSingleQuote );
                    }
                    else if (tagName == RtfSpec::TagLeftDoubleQuote)
                    {
                        NotifyInsertSpecialChar( RtfVisualSpecialCharKind::LeftDoubleQuote );
                    }
                    else if (tagName == RtfSpec::TagRightDoubleQuote)
                    {
                        NotifyInsertSpecialChar( RtfVisualSpecialCharKind::RightDoubleQuote );
                    }
                    else if (tagName == RtfSpec::TagHyphen)
                    {
                        NotifyInsertSpecialChar( RtfVisualSpecialCharKind::OptionalHyphen );
                    }
                    else if (tagName == RtfSpec::TagUnderscore)
                    {
                        NotifyInsertSpecialChar( RtfVisualSpecialCharKind::NonBreakingHyphen );
                    }
                }
                break;
        }
    } // IRtfElementVisitor.VisitTag

    // ----------------------------------------------------------------------
    void RtfInterpreter::VisitGroup( RtfGroup* group )
    {
        QString& groupDestination = group->Destination();

        switch ( Context()->getState() )
        {
            case RtfInterpreterState::Init:
                if ( RtfSpec::TagRtf == groupDestination )
                {
                    VisitChildrenOf( group );
                }
                else
                {
                    assert (0);
                    //throw new RtfStructureException( Strings.InvalidInitGroupState( groupDestination ) );
                }
                break;

            case RtfInterpreterState::InHeader:
                {
                    if (groupDestination == RtfSpec::TagFontTable)
                    {
                        fontTableBuilder->VisitGroup( group );
                    }
                    else if (groupDestination == RtfSpec::TagColorTable)
                    {
                        colorTableBuilder->VisitGroup( group );
                    }
                    else if (groupDestination == RtfSpec::TagGenerator)
                    {
                        // last group with a destination in header, but no need to process its contents
                        Context()->setState (RtfInterpreterState::InDocument);

                        //RtfText* generator = group->Contents()->Count() == 3 ? group->Contents()->Get( 2 ) as IRtfText : null;
                        //if ( generator != NULL )
                        //{
                        // string generatorName = generator.Text;
                        // Context.Generator = generatorName.EndsWith( ";" ) ?
                        //  generatorName.Substring( 0, generatorName.Length - 1 ) : generatorName;
                        //}
                        //else
                        //{
                        // throw new RtfInvalidDataException( Strings.InvalidGeneratorGroup( group.ToString() ) );
                        //}
                        RtfText* generator = NULL;
                        if (group->Contents().Count() == 3)
                        {
                            generator = (RtfText*)group->Contents().Get( 2 );
                            QString& generatorName = generator->Text();

                            QString tmpStr;

                            if (generatorName.endsWith (";"))
                            {
                                tmpStr = generatorName.left(generatorName.length()-1);//strip off the ;
                            }
                            else
                            {
                                tmpStr = generatorName;
                            }

                            Context()->setGenerator(tmpStr);
                        }
                        else
                        {
                            assert (0);
                            //throw new RtfInvalidDataException( Strings.InvalidGeneratorGroup( group.ToString() ) );
                        }
                    }
                    else if (groupDestination == RtfSpec::TagPlain ||
                             groupDestination == RtfSpec::TagParagraphDefaults ||
                             groupDestination == RtfSpec::TagSectionDefaults ||
                             groupDestination == RtfSpec::TagUnderLineNone ||
                             groupDestination.isEmpty())
                    {
                        // <tags>: special tags commonly used to reset state in a beginning group. necessary to recognize
                        //         state transition form header to document in case no other mechanism detects it and the
                        //         content starts with a group with such a 'destination' ...
                        // 'null': group without destination cannot be part of header, but need to process its contents
                        Context()->setState (RtfInterpreterState::InDocument);
                        if ( !group->IsExtensionDestination () )
                        {
                            VisitChildrenOf( group );
                        }
                    }
                }
                break;

            case RtfInterpreterState::InDocument:
                {
                    if (groupDestination == RtfSpec::TagUserProperties)
                    {
                        userPropertyBuilder->VisitGroup( group );
                    }
                    else if (groupDestination == RtfSpec::TagInfo)
                    {
                        documentInfoBuilder->VisitGroup( group );
                    }
                    else if (groupDestination == RtfSpec::TagUnicodeAlternativeChoices)
                    {
                        QString altStr (RtfSpec::TagUnicodeAlternativeUnicode);
                        RtfGroup* alternativeWithUnicodeSupport =
                            group->SelectChildGroupWithDestination( altStr );
                        if ( alternativeWithUnicodeSupport != NULL )
                        {
                            // there is an alternative with unicode formatted content -> use this
                            VisitChildrenOf( alternativeWithUnicodeSupport );
                        }
                        else
                        {
                            // try to locate the alternative without unicode -> only ANSI fallbacks
                            RtfGroup* alternativeWithoutUnicode = NULL;

                            if (group->Contents().Count() > 2)
                            {
                                alternativeWithoutUnicode = (RtfGroup*)group->Contents().Get(2);
                                VisitChildrenOf( alternativeWithoutUnicode );
                            }
                        }
                    }
                    else if (groupDestination == RtfSpec::TagHeader ||
                             groupDestination == RtfSpec::TagHeaderFirst ||
                             groupDestination == RtfSpec::TagHeaderLeft ||
                             groupDestination == RtfSpec::TagHeaderRight ||
                             groupDestination == RtfSpec::TagFooter ||
                             groupDestination == RtfSpec::TagFooterFirst ||
                             groupDestination == RtfSpec::TagFooterLeft ||
                             groupDestination == RtfSpec::TagFooterRight ||
                             groupDestination == RtfSpec::TagFootnote ||
                             groupDestination == RtfSpec::TagStyleSheet )
                    {
                        // groups we currently ignore, so their content doesn't intermix with
                        // the actual document content
                        ;
                    }
                    else if (groupDestination == RtfSpec::TagPictureWrapper)
                    {
                        VisitChildrenOf( group );
                        lastGroupWasPictureWrapper = true;
                    }
                    else if (groupDestination == RtfSpec::TagPictureWrapperAlternative)
                    {
                        if ( !lastGroupWasPictureWrapper )
                        {
                            VisitChildrenOf( group );
                        }
                        lastGroupWasPictureWrapper = false;
                    }
#if 0 //UNIMPLEMENTED currently NOT USED
                    else if (groupDestination == RtfSpec::TagPicture)
                    {
                        imageBuilder.VisitGroup( group );
                        NotifyInsertImage(
                            imageBuilder.Format,
                            imageBuilder.Width,
                            imageBuilder.Height,
                            imageBuilder.DesiredWidth,
                            imageBuilder.DesiredHeight,
                            imageBuilder.ScaleWidthPercent,
                            imageBuilder.ScaleHeightPercent,
                            imageBuilder.ImageDataHex );
                    }
#endif
                    else if (groupDestination == RtfSpec::TagParagraphNumberText ||
                             groupDestination == RtfSpec::TagListNumberText)
                    {
//                          NotifyInsertSpecialChar( RtfVisualSpecialCharKind.ParagraphNumberBegin );
                        VisitChildrenOf( group );
//                          NotifyInsertSpecialChar( RtfVisualSpecialCharKind.ParagraphNumberEnd );
                    }
                    else
                    {
                        if ((groupDestination == "fldrslt") && (!hyperlinkStr.isEmpty()))
                        {
                            bool pushedTextFormat = false;
                            if (Context()->getState() == RtfInterpreterState::InDocument)
                            {
                                Context()->PushCurrentTextFormat();
                                pushedTextFormat = true;
                            }

                            hyperlinkVisualText = ExtractHyperlinkVisual(group);
                            if (!hyperlinkVisualText.isEmpty())
                            {
                                QString combined;
                                combined = hyperlinkStr;
                                combined += "|"; //add separator
                                combined += hyperlinkVisualText;
                                RtfText* hyperlinkText = new RtfText (combined, 1252, static_cast<RtfInterpreterBase*>(this));
                                NotifyInsertText(hyperlinkText->Text());
                            }
                            if (pushedTextFormat)
                            {
                                Context()->PopCurrentTextFormat();
                            }
                            hyperlinkStr.clear();
                        }
                        else if ( !group->IsExtensionDestination () )
                        {
                            // nested text group
                            VisitChildrenOf( group );
                        }
                        else if (groupDestination == "fldinst")
                        {
                            hyperlinkStr = ExtractHyperlink(group);
                        }
                    }
                }
                break;
        }
    } // IRtfElementVisitor.VisitGroup

    // ----------------------------------------------------------------------
    void RtfInterpreter::VisitText( RtfText* text )
    {
        switch ( Context()->getState() )
        {
            case RtfInterpreterState::Init:
                assert (0);
            //throw new RtfStructureException( Strings.InvalidInitTextState( text.Text ) );
            case RtfInterpreterState::InHeader:
                // allow spaces in between header tables
                if ( !text->Text().isEmpty() )
                {
                    Context()->setState (RtfInterpreterState::InDocument);
                }
                break;
            case RtfInterpreterState::InDocument:
                break;
        }
        NotifyInsertText( text->Text () );
    } // IRtfElementVisitor.VisitText

} // namespace Itenso.Rtf.Interpreter
// -- EOF -------------------------------------------------------------------
