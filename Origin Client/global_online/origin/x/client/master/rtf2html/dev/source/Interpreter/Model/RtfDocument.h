// -- FILE ------------------------------------------------------------------
// name       : IRtfDocument.cs
// project    : RTF Framelet
// created    : Leon Poyyayil - 2008.05.21
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
#ifndef __RTFDOCUMENT_H
#define __RTFDOCUMENT_H

#include <qstring.h>
#include "RtfColorCollection.h"
#include "RtfDocumentInfo.h"
#include "RtfDocumentProperty.h"
#include "RtfDocumentPropertyCollection.h"
#include "RtfFont.h"
#include "RtfFontCollection.h"
#include "RtfTextFormat.h"
#include "RtfTextFormatCollection.h"
#include "RtfVisualCollection.h"
#include "RtfInterpreterContext.h"

namespace RTF2HTML
{

    // ------------------------------------------------------------------------
    class RtfDocument : public QObject
    {
        public:

            RtfDocument( RtfInterpreterContext* context, RtfVisualCollection* visualContent, QObject* parent = 0 );
            virtual ~RtfDocument ();

            void Init(
                int rtfVersion,
                RtfFont* defaultFont,
                RtfFontCollection* fontTable,
                RtfColorCollection* colorTable,
                QString& generator,
                RtfTextFormatCollection* uniqueTextFormats,
                RtfDocumentInfo* documentInfo,
                RtfDocumentPropertyCollection* userProperties,
                RtfVisualCollection* visualContent);

            virtual QString& ToString();

            // ----------------------------------------------------------------------
            int RtfVersion();

            // ----------------------------------------------------------------------
            RtfFont* DefaultFont();

            // ----------------------------------------------------------------------
            RtfTextFormat* DefaultTextFormat ();

            // ----------------------------------------------------------------------
            RtfFontCollection* FontTable ();

            // ----------------------------------------------------------------------
            RtfColorCollection* ColorTable ();

            // ----------------------------------------------------------------------
            QString& Generator();

            // ----------------------------------------------------------------------
            RtfTextFormatCollection* UniqueTextFormats ();

            // ----------------------------------------------------------------------
            RtfDocumentInfo* DocumentInfo ();

            // ----------------------------------------------------------------------
            RtfDocumentPropertyCollection* UserProperties ();

            // ----------------------------------------------------------------------
            RtfVisualCollection* VisualContent ();

        private:
            int rtfVersion;
            RtfFont* defaultFont;
            RtfTextFormat* defaultTextFormat;
            RtfFontCollection* fontTable;
            RtfColorCollection* colorTable;
            QString generator;
            RtfTextFormatCollection* uniqueTextFormats;
            RtfDocumentInfo* documentInfo;
            RtfDocumentPropertyCollection* userProperties;
            RtfVisualCollection* visualContent;

            QString toStr;
    };// interface IRtfDocument

} // namespace Itenso.Rtf
#endif
// -- EOF -------------------------------------------------------------------
