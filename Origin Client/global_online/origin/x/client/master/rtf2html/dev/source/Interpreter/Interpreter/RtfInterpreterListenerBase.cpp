// -- FILE ------------------------------------------------------------------
// name       : RtfInterpreterListenerBase.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.21
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
#include "RtfInterpreterListenerBase.h"

namespace RTF2HTML
{
    RtfInterpreterListenerBase::RtfInterpreterListenerBase (QObject* parent)
        : RtfInterpreterListener (parent)
    {
    }

    RtfInterpreterListenerBase::~RtfInterpreterListenerBase ()
    {
    }

    // ----------------------------------------------------------------------
    void RtfInterpreterListenerBase::BeginDocument( RtfInterpreterContext* context )
    {
        if ( context != NULL )
        {
            DoBeginDocument( context );
        }
    } // BeginDocument

    // ----------------------------------------------------------------------
    void RtfInterpreterListenerBase::InsertText( RtfInterpreterContext* context, QString& text )
    {
        if ( context != NULL )
        {
            DoInsertText( context, text );
        }
    } // InsertText

    // ----------------------------------------------------------------------
    void RtfInterpreterListenerBase::InsertSpecialChar( RtfInterpreterContext* context, RtfVisualSpecialCharKind::RtfVisualSpecialCharKind kind )
    {
        if ( context != NULL )
        {
            DoInsertSpecialChar( context, kind );
        }
    } // InsertSpecialChar

    // ----------------------------------------------------------------------
    void RtfInterpreterListenerBase::InsertBreak( RtfInterpreterContext* context, RtfVisualBreakKind::RtfVisualBreakKind kind )
    {
        if ( context != NULL )
        {
            DoInsertBreak( context, kind );
        }
    } // InsertBreak

#if 0 //UNIMPLEMENTED currently NOT USED
    // ----------------------------------------------------------------------
    public void InsertImage( IRtfInterpreterContext context, RtfVisualImageFormat format,
                             int width, int height, int desiredWidth, int desiredHeight,
                             int scaleWidthPercent, int scaleHeightPercent, string imageDataHex
                           )
    {
        if ( context != null )
        {
            DoInsertImage( context, format,
                           width, height, desiredWidth, desiredHeight,
                           scaleWidthPercent, scaleHeightPercent, imageDataHex );
        }
    } // InsertImage
#endif
    // ----------------------------------------------------------------------
    void RtfInterpreterListenerBase::EndDocument( RtfInterpreterContext* context )
    {
        if ( context != NULL )
        {
            DoEndDocument( context );
        }
    } // EndDocument

    // ----------------------------------------------------------------------
    void RtfInterpreterListenerBase::DoBeginDocument( RtfInterpreterContext* context )
    {
        Q_UNUSED (context);
    } // DoBeginDocument

    // ----------------------------------------------------------------------
    void RtfInterpreterListenerBase::DoInsertText( RtfInterpreterContext* context, QString& text )
    {
        Q_UNUSED (context);
        Q_UNUSED (text);
    } // DoInsertText

    // ----------------------------------------------------------------------
    void RtfInterpreterListenerBase::DoInsertSpecialChar( RtfInterpreterContext* context, RtfVisualSpecialCharKind::RtfVisualSpecialCharKind kind )
    {
        Q_UNUSED (context);
        Q_UNUSED (kind);
    } // DoInsertSpecialChar

    // ----------------------------------------------------------------------
    void RtfInterpreterListenerBase::DoInsertBreak( RtfInterpreterContext* context, RtfVisualBreakKind::RtfVisualBreakKind kind )
    {
        Q_UNUSED (context);
        Q_UNUSED (kind);
    } // DoInsertBreak

#if 0 //UNIMPLEMENTED currently NOT USED
    // ----------------------------------------------------------------------
    protected virtual void DoInsertImage( IRtfInterpreterContext context,
                                          RtfVisualImageFormat format,
                                          int width, int height, int desiredWidth, int desiredHeight,
                                          int scaleWidthPercent, int scaleHeightPercent,
                                          string imageDataHex
                                        )
    {
    } // DoInsertImage
#endif

    // ----------------------------------------------------------------------
    void RtfInterpreterListenerBase::DoEndDocument( RtfInterpreterContext* context )
    {
        Q_UNUSED (context);
    } // DoEndDocument


} // namespace Itenso.Rtf.Interpreter
// -- EOF -------------------------------------------------------------------
