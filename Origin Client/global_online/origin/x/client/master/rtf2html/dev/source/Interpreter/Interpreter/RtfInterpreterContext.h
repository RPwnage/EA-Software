// -- FILE ------------------------------------------------------------------
// name       : IRtfInterpreterContext.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.20
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
#ifndef __RTFINTERPRETERCONTEXT_H
#define __RTFINTERPRETERCONTEXT_H
#include <qobject.h>
#include <qstack.h>
#include "RtfInterpreterState.h"
#include "RtfFontCollection.h"
#include "RtfColorCollection.h"
#include "RtfTextFormatCollection.h"
#include "RtfDocumentInfo.h"
#include "RtfDocumentPropertyCollection.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfInterpreterContext : public QObject
    {
        public:

            RtfInterpreterContext(QObject* parent = 0);
            ~RtfInterpreterContext();

            // ----------------------------------------------------------------------
            RtfInterpreterState::RtfInterpreterState getState ();
            void setState (RtfInterpreterState::RtfInterpreterState value);

            // ----------------------------------------------------------------------
            int getRtfVersion ();
            void setRtfVersion (int value);

            // ----------------------------------------------------------------------
            QString& getDefaultFontId ();
            void setDefaultFontId (QString& value);

            // ----------------------------------------------------------------------
            RtfFont* DefaultFont ();

            // ----------------------------------------------------------------------
            RtfFontCollection* FontTable ();
            RtfFontCollection* WritableFontTable ();

            // ----------------------------------------------------------------------
            RtfColorCollection* ColorTable ();
            RtfColorCollection* WritableColorTable();

            // ----------------------------------------------------------------------
            QString getGenerator();
            void setGenerator(QString& value);

            // ----------------------------------------------------------------------
            RtfTextFormatCollection* UniqueTextFormats ();

            // ----------------------------------------------------------------------
            RtfTextFormat* CurrentTextFormat ();

            // ----------------------------------------------------------------------
            RtfTextFormat* GetSafeCurrentTextFormat();

            // ----------------------------------------------------------------------
            RtfTextFormat* GetUniqueTextFormatInstance( RtfTextFormat* templateFormat );

            // ----------------------------------------------------------------------
            RtfDocumentInfo* DocumentInfo ();
            RtfDocumentInfo* WritableDocumentInfo();

            // ----------------------------------------------------------------------
            RtfDocumentPropertyCollection* UserProperties ();

            // ----------------------------------------------------------------------
            RtfTextFormat* getWritableCurrentTextFormat();
            void setWritableCurrentTextFormat(RtfTextFormat* value);

            // ----------------------------------------------------------------------
            RtfDocumentPropertyCollection* WritableUserProperties();

            // ----------------------------------------------------------------------
            void PushCurrentTextFormat();
            void PopCurrentTextFormat();

            // ----------------------------------------------------------------------
            void Reset();

        private:
            RtfInterpreterState::RtfInterpreterState state;
            int rtfVersion;
            QString defaultFontId;
            RtfFontCollection* fontTable;
            RtfColorCollection* colorTable;
            QString generator;
            RtfTextFormatCollection* uniqueTextFormats;
            QStack<RtfTextFormat*> textFormatStack;
            RtfTextFormat* currentTextFormat;
            RtfDocumentInfo* documentInfo;
            RtfDocumentPropertyCollection* userProperties;

    }; // interface IRtfInterpreterContext

} // namespace Itenso.Rtf
// -- EOF -------------------------------------------------------------------
#endif //__RTFINTERPRETERCONTEXT_H