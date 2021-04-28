// -- FILE ------------------------------------------------------------------
// name       : RtfDocument.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.21
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;

#include <assert.h>
#include "RtfDocument.h"
#include "RtfSpec.h"

namespace RTF2HTML
{
    // ----------------------------------------------------------------------
    RtfDocument::RtfDocument( RtfInterpreterContext* context, RtfVisualCollection* visualContent, QObject* parent )
        : QObject (parent)
    {
        QString genStr = context->getGenerator();
        Init(context->getRtfVersion(),
             context->DefaultFont(),
             context->FontTable(),
             context->ColorTable(),
             genStr,
             context->UniqueTextFormats(),
             context->DocumentInfo(),
             context->UserProperties(),
             visualContent);
    } // RtfDocument

    // ----------------------------------------------------------------------
    void RtfDocument::Init (
        int rtfVersion,
        RtfFont* defaultFont,
        RtfFontCollection* fontTable,
        RtfColorCollection* colorTable,
        QString& generator,
        RtfTextFormatCollection* uniqueTextFormats,
        RtfDocumentInfo* documentInfo,
        RtfDocumentPropertyCollection* userProperties,
        RtfVisualCollection* visualContent
    )
    {
        if ( rtfVersion != RtfSpec::RtfVersion1 )
        {
            assert (0);
            //throw new RtfUnsupportedStructureException( Strings.UnsupportedRtfVersion( rtfVersion ) );
        }
        if ( defaultFont == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "defaultFont" );
        }
        if ( fontTable == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "fontTable" );
        }
        if ( colorTable == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "colorTable" );
        }
        if ( uniqueTextFormats == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "uniqueTextFormats" );
        }
        if ( documentInfo == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "documentInfo" );
        }
        if ( userProperties == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "userProperties" );
        }
        if ( visualContent == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "visualContent" );
        }
        this->rtfVersion = rtfVersion;
        this->defaultFont = defaultFont;
        defaultTextFormat = new RtfTextFormat( defaultFont, RtfSpec::DefaultFontSize, this );
        this->fontTable = fontTable;
        this->colorTable = colorTable;
        this->generator = generator;
        this->uniqueTextFormats = uniqueTextFormats;
        this->documentInfo = documentInfo;
        this->userProperties = userProperties;
        this->visualContent = visualContent;

        toStr = QString("RTFv%0").arg(rtfVersion);
    } // RtfDocument


    // ----------------------------------------------------------------------
    RtfDocument::~RtfDocument ()
    {
    }
    // ----------------------------------------------------------------------
    int RtfDocument::RtfVersion ()
    {
        return rtfVersion;
    } // RtfVersion

    // ----------------------------------------------------------------------
    RtfFont* RtfDocument::DefaultFont ()
    {
        return defaultFont;
    } // DefaultFont

    // ----------------------------------------------------------------------
    RtfTextFormat*  RtfDocument::DefaultTextFormat()
    {
        return defaultTextFormat;
    } // DefaultTextFormat

    // ----------------------------------------------------------------------
    RtfFontCollection*  RtfDocument::FontTable ()
    {
        return fontTable;
    } // FontTable

    // ----------------------------------------------------------------------
    RtfColorCollection* RtfDocument::ColorTable()
    {
        return colorTable;
    } // ColorTable

    // ----------------------------------------------------------------------
    QString& RtfDocument::Generator()
    {
        return generator;
    } // Generator

    // ----------------------------------------------------------------------
    RtfTextFormatCollection* RtfDocument::UniqueTextFormats()
    {
        return uniqueTextFormats;
    } // UniqueTextFormats

    // ----------------------------------------------------------------------
    RtfDocumentInfo* RtfDocument::DocumentInfo ()
    {
        return documentInfo;
    } // DocumentInfo

    // ----------------------------------------------------------------------
    RtfDocumentPropertyCollection* RtfDocument::UserProperties ()
    {
        return userProperties;
    } // UserProperties

    // ----------------------------------------------------------------------
    RtfVisualCollection* RtfDocument::VisualContent()
    {
        return visualContent;
    } // VisualContent

    // ----------------------------------------------------------------------
    QString& RtfDocument::ToString()
    {
        return toStr;
    } // ToString

} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
