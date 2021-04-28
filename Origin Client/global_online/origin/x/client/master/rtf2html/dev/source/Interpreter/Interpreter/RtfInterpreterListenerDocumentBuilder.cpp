// -- FILE ------------------------------------------------------------------
// name       : RtfInterpreterListenerDocumentBuilder.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.21
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System.Text;
//using Itenso.Rtf.Model;

#include <assert.h>
#include "RtfInterpreterListenerDocumentBuilder.h"
#include "RtfVisualText.h"

namespace RTF2HTML
{

    RtfInterpreterListenerDocumentBuilder::RtfInterpreterListenerDocumentBuilder(QObject* parent)
        : RtfInterpreterListenerBase (parent)
    {
        combineTextWithSameFormat = true;
        pendingParagraphContent = new RtfVisualCollection(this);
        document = NULL;
        pendingTextFormat = NULL;
    }

    RtfInterpreterListenerDocumentBuilder::~RtfInterpreterListenerDocumentBuilder()
    {
    }

    // ----------------------------------------------------------------------
    bool RtfInterpreterListenerDocumentBuilder::getCombineTextWithSameFormat ()
    {
        return combineTextWithSameFormat;
    }
    void RtfInterpreterListenerDocumentBuilder::setCombineTextWithSameFormat (bool value)
    {
        combineTextWithSameFormat = value;
    } // CombineTextWithSameFormat

    // ----------------------------------------------------------------------
    RtfDocument* RtfInterpreterListenerDocumentBuilder::Document ()
    {
        return document;
    } // Document

    // ----------------------------------------------------------------------
    void RtfInterpreterListenerDocumentBuilder::DoBeginDocument( RtfInterpreterContext* context )
    {
        Q_UNUSED (context);
        document = NULL;
        visualDocumentContent = new RtfVisualCollection(this);
    } // DoBeginDocument

    // ----------------------------------------------------------------------
    void RtfInterpreterListenerDocumentBuilder::DoInsertText( RtfInterpreterContext* context, QString& text )
    {
        if ( combineTextWithSameFormat )
        {
            RtfTextFormat* newFormat = context->GetSafeCurrentTextFormat();
            if ( !newFormat->Equals( pendingTextFormat ) )
            {
                FlushPendingText();
            }
            pendingTextFormat = newFormat;
            pendingText.append( text );
        }
        else
        {
            AppendAlignedVisual( new RtfVisualText( text, context->GetSafeCurrentTextFormat(), this ) );
        }
    } // DoInsertText

    // ----------------------------------------------------------------------
    void RtfInterpreterListenerDocumentBuilder::DoInsertSpecialChar( RtfInterpreterContext* context, RtfVisualSpecialCharKind::RtfVisualSpecialCharKind kind )
    {
        Q_UNUSED (context);
        FlushPendingText();
        visualDocumentContent->Add( new RtfVisualSpecialChar( kind, this ) );
    } // DoInsertSpecialChar

    // ----------------------------------------------------------------------
    void RtfInterpreterListenerDocumentBuilder::DoInsertBreak( RtfInterpreterContext* context, RtfVisualBreakKind::RtfVisualBreakKind kind )
    {
        FlushPendingText();
        visualDocumentContent->Add( new RtfVisualBreak( kind, this ) );
        switch ( kind )
        {
            case RtfVisualBreakKind::Paragraph:
            case RtfVisualBreakKind::Section:
                EndParagraph( context );
                break;
        }
    } // DoInsertBreak

#if 0 //UNIMPLEMENTED currently NOT USED
    // ----------------------------------------------------------------------
    void RtfInterpreterListenerDocumentBuilder::DoInsertImage( IRtfInterpreterContext* context,
            RtfVisualImageFormat format,
            int width, int height, int desiredWidth, int desiredHeight,
            int scaleWidthPercent, int scaleHeightPercent,
            string imageDataHex
                                                             )
    {
        FlushPendingText();
        AppendAlignedVisual( new RtfVisualImage( format,
                             context.GetSafeCurrentTextFormat().Alignment,
                             width, height, desiredWidth, desiredHeight,
                             scaleWidthPercent, scaleHeightPercent, imageDataHex ) );
    } // DoInsertImage
#endif
    // ----------------------------------------------------------------------
    void RtfInterpreterListenerDocumentBuilder::DoEndDocument( RtfInterpreterContext* context )
    {
        FlushPendingText();
        EndParagraph( context );
        document = new RtfDocument( context, visualDocumentContent, this );
    } // DoEndDocument

    // ----------------------------------------------------------------------
    void RtfInterpreterListenerDocumentBuilder::EndParagraph( RtfInterpreterContext* context )
    {
        RtfTextAlignment::RtfTextAlignment finalParagraphAlignment = context->GetSafeCurrentTextFormat()->Alignment();
        int count = pendingParagraphContent->Count();

        for (int i = 0; i < count; i++)
        {
            RtfVisual* alignedVisual = pendingParagraphContent->Get(i);

            switch ( alignedVisual->Kind () )
            {
#if 0 //UNIMPLEMENTED currently NOT USED
                case RtfVisualKind::Image:
                    RtfVisualImage* image = (RtfVisualImage*)alignedVisual;
                    // ReSharper disable RedundantCheckBeforeAssignment
                    if ( image->Alignment() != finalParagraphAlignment )
                        // ReSharper restore RedundantCheckBeforeAssignment
                    {
                        image->Alignment() = finalParagraphAlignment;
                    }
                    break;
#endif
                case RtfVisualKind::Text:
                    RtfVisualText* text = (RtfVisualText*)alignedVisual;
                    if ( text->getFormat()->Alignment() != finalParagraphAlignment )
                    {
                        RtfTextFormat* correctedFormat = ( (RtfTextFormat*)text->getFormat())->DeriveWithAlignment( finalParagraphAlignment );
                        RtfTextFormat* correctedUniqueFormat = context->GetUniqueTextFormatInstance( correctedFormat );
                        text->setFormat (correctedUniqueFormat);
                    }
                    break;
            }
        }
        pendingParagraphContent->Clear();
    } // EndParagraph

    // ----------------------------------------------------------------------
    void RtfInterpreterListenerDocumentBuilder::FlushPendingText()
    {
        if ( pendingTextFormat != NULL )
        {
            AppendAlignedVisual( new RtfVisualText( pendingText, pendingTextFormat, this ) );
            pendingTextFormat = NULL;
            pendingText = "";
        }
    } // FlushPendingText

    // ----------------------------------------------------------------------
    void RtfInterpreterListenerDocumentBuilder::AppendAlignedVisual( RtfVisual* visual )
    {
        visualDocumentContent->Add( visual );
        pendingParagraphContent->Add( visual );
    } // AppendAlignedVisual

} // namespace Itenso.Rtf.Interpreter
// -- EOF -------------------------------------------------------------------
