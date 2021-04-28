// -- FILE ------------------------------------------------------------------
// name       : RtfInterpreterContext.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.21
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using System.Collections;
//using Itenso.Rtf.Model;

#include <assert.h>
#include "RtfInterpreterState.h"
#include "RtfSpec.h"
#include "RtfInterpreterContext.h"


namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    RtfInterpreterContext::RtfInterpreterContext (QObject* parent)
        :QObject (parent)
    {
        fontTable = new RtfFontCollection(this);
        colorTable = new RtfColorCollection(this);
        uniqueTextFormats = new RtfTextFormatCollection(this);
        documentInfo = new RtfDocumentInfo(this);
        userProperties = new RtfDocumentPropertyCollection(this);
        currentTextFormat = NULL;
    }

    RtfInterpreterContext::~RtfInterpreterContext()
    {
    }
    // ----------------------------------------------------------------------
    RtfInterpreterState::RtfInterpreterState RtfInterpreterContext::getState ()
    {
        return state;
    } // State

    void RtfInterpreterContext::setState (RtfInterpreterState::RtfInterpreterState value)
    {
        state = value;
    } // State
    // ----------------------------------------------------------------------
    int RtfInterpreterContext::getRtfVersion ()
    {
        return rtfVersion;
    } // RtfVersion

    void RtfInterpreterContext::setRtfVersion (int value)
    {
        rtfVersion = value;
    } // RtfVersion

    // ----------------------------------------------------------------------
    QString& RtfInterpreterContext::getDefaultFontId ()
    {
        return defaultFontId;
    } // DefaultFontIndex

    void RtfInterpreterContext::setDefaultFontId (QString& value)
    {
        defaultFontId = value;
    } // DefaultFontIndex
    // ----------------------------------------------------------------------
    RtfFont* RtfInterpreterContext::DefaultFont()
    {
        RtfFont* defaultFont = fontTable->Get ( defaultFontId );
        if ( defaultFont != NULL )
        {
            return defaultFont;
        }
        assert (0);

		//MY: if default font doesn't exist, it's really a malformed .rtf
		//but so that it won't cause a crash, just choose the first one in the fontTable and return that as the default
		defaultFont = fontTable->Get(0);
		return defaultFont;
        //return NULL;
        //throw new RtfUndefinedFontException( Strings.InvalidDefaultFont(
        //  defaultFontId, fontTable.ToString() ) );
    } // DefaultFont

    // ----------------------------------------------------------------------
    RtfFontCollection* RtfInterpreterContext::FontTable()
    {
        return fontTable;
    } // FontTable

    // ----------------------------------------------------------------------
    RtfFontCollection* RtfInterpreterContext::WritableFontTable ()
    {
        return fontTable;
    } // WritableFontTable

    // ----------------------------------------------------------------------
    RtfColorCollection* RtfInterpreterContext::ColorTable()
    {
        return colorTable;
    } // ColorTable

    // ----------------------------------------------------------------------
    RtfColorCollection* RtfInterpreterContext::WritableColorTable()
    {
        return colorTable;
    } // WritableColorTable

    // ----------------------------------------------------------------------
    QString RtfInterpreterContext::getGenerator()
    {
        return generator;
    } // Generator

    void RtfInterpreterContext::setGenerator(QString& value)
    {
        generator = value;
    } // Generator
    // ----------------------------------------------------------------------
    RtfTextFormatCollection* RtfInterpreterContext::UniqueTextFormats ()
    {
        return uniqueTextFormats;
    } // UniqueTextFormats

    // ----------------------------------------------------------------------
    RtfTextFormat* RtfInterpreterContext::CurrentTextFormat ()
    {
        return currentTextFormat;
    } // CurrentTextFormat

    // ----------------------------------------------------------------------
    RtfTextFormat* RtfInterpreterContext::GetSafeCurrentTextFormat()
    {
        return currentTextFormat ? currentTextFormat : getWritableCurrentTextFormat();
    } // GetSafeCurrentTextFormat

    // ----------------------------------------------------------------------
    RtfTextFormat* RtfInterpreterContext::GetUniqueTextFormatInstance( RtfTextFormat* templateFormat )
    {
        if ( templateFormat == NULL )
        {
            assert (0);
            //throw new ArgumentNullException( "templateFormat" );
        }

        RtfTextFormat* uniqueInstance;
        int existingEquivalentPos = uniqueTextFormats->IndexOf( templateFormat );
        if ( existingEquivalentPos >= 0 )
        {
            // we already know an equivalent format -> reference that one for future use
            uniqueInstance = uniqueTextFormats->Get (existingEquivalentPos);
        }
        else
        {
            // this is a yet unknown format -> add it to the known formats and use it
            uniqueTextFormats->Add( templateFormat );
            uniqueInstance = templateFormat;
        }
        return uniqueInstance;
    } // GetUniqueTextFormatInstance

    // ----------------------------------------------------------------------
    RtfTextFormat* RtfInterpreterContext::getWritableCurrentTextFormat()
    {
        if ( currentTextFormat == NULL )
        {
            // set via property to ensure it will get added to the unique map
            setWritableCurrentTextFormat (new RtfTextFormat( DefaultFont(), RtfSpec::DefaultFontSize, this ));
        }
        return currentTextFormat;
    }

    void RtfInterpreterContext::setWritableCurrentTextFormat(RtfTextFormat* value)
    {
        currentTextFormat = (RtfTextFormat*)GetUniqueTextFormatInstance( value );
    } // WritableCurrentTextFormat

    // ----------------------------------------------------------------------
    RtfDocumentInfo* RtfInterpreterContext::DocumentInfo()
    {
        return documentInfo;
    } // DocumentInfo

    // ----------------------------------------------------------------------
    RtfDocumentInfo* RtfInterpreterContext::WritableDocumentInfo()
    {
        return documentInfo;
    } // WritableDocumentInfo

    // ----------------------------------------------------------------------
    RtfDocumentPropertyCollection* RtfInterpreterContext::UserProperties ()
    {
        return userProperties;
    } // UserProperties

    // ----------------------------------------------------------------------
    RtfDocumentPropertyCollection* RtfInterpreterContext::WritableUserProperties()
    {
        return userProperties;
    } // WritableUserProperties

    // ----------------------------------------------------------------------
    void RtfInterpreterContext::PushCurrentTextFormat()
    {
        textFormatStack.push( getWritableCurrentTextFormat() );
    } // PushCurrentTextFormat

    // ----------------------------------------------------------------------
    void RtfInterpreterContext::PopCurrentTextFormat()
    {
        if ( textFormatStack.size() == 0 )
        {
            assert (0);
//          throw new RtfStructureException( Strings.InvalidTextContextState );
        }
        currentTextFormat = (RtfTextFormat*)textFormatStack.pop();
    } // PopCurrentTextFormat

    // ----------------------------------------------------------------------
    void RtfInterpreterContext::Reset()
    {
        state = RtfInterpreterState::Init;
        rtfVersion = RtfSpec::RtfVersion1;
        defaultFontId = "f0";
        fontTable->Clear();
        colorTable->Clear();
        generator = "";
        uniqueTextFormats->Clear();
        textFormatStack.clear();
        currentTextFormat = NULL;
        documentInfo->Reset();
        userProperties->Clear();
    } // Reset

} // namespace Itenso.Rtf.Interpreter
// -- EOF -------------------------------------------------------------------
