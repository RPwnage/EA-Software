// -- FILE ------------------------------------------------------------------
// name       : RtfHtmlConvertSettings.cs
// project    : RTF Framelet
// created    : Jani Giannoudis - 2008.06.02
// language   : c#
// environment: .NET 2.0
// copyright  : (c) 2004-2013 by Jani Giannoudis, Switzerland
// --------------------------------------------------------------------------
//using System;
//using System.Collections.Specialized;
//using Itenso.Rtf.Converter.Image;

#ifndef __RTFHTMLCONVERTSETTINGS_H
#define __RTFHTMLCONVERTSETTINGS_H

#include <qobject.h>
#include <qstring.h>

namespace RTF2HTML
{

    namespace RtfHtmlConvertScope
    {
        enum RtfHtmlConvertScope
        {
            None = 0x00000000,

            Document = 0x00000001,
            Html = 0x00000010,
            Head = 0x00000100,
            Body = 0x00001000,
            Content = 0x00010000,

            All = Document | Html | Head | Body | Content,
        }; // class RtfHtmlConvertScope
    }

    // ------------------------------------------------------------------------
    class RtfHtmlConvertSettings : public QObject
    {
        public:

            static QString DefaultDocumentHeader;
            static QString DefaultDocumentCharacterSet;
            static QString DefaultSpecialCharRepresentation;

            // ----------------------------------------------------------------------
            RtfHtmlConvertSettings(QObject* parent = 0);

            // ----------------------------------------------------------------------
            RtfHtmlConvertScope::RtfHtmlConvertScope ConvertScope ();

            // ----------------------------------------------------------------------
            bool HasStyles ();

#if 0 //UNIMPLEMENTED, currently NOT USED
            // ----------------------------------------------------------------------
            public RtfHtmlCssStyleCollection Styles
            {
                get { return styles ?? ( styles = new RtfHtmlCssStyleCollection() ); }
            } // Styles

            // ----------------------------------------------------------------------
            public bool HasStyleSheetLinks
            {
                get { return styleSheetLinks != null && styleSheetLinks.Count > 0; }
            } // HasStyleSheetLinks

            // ----------------------------------------------------------------------
            public StringCollection StyleSheetLinks
            {
                get { return styleSheetLinks ?? ( styleSheetLinks = new StringCollection() ); }
            } // StyleSheetLinks
#endif
            // ----------------------------------------------------------------------
            QString& DocumentHeader();

            // ----------------------------------------------------------------------
            QString& getTitle ();
            void setTitle (QString& title);

            // ----------------------------------------------------------------------
            QString& CharacterSet ();

#if 0 //UNIMPLEMENTED, currently NOT USED
            // ----------------------------------------------------------------------
            public string VisualHyperlinkPattern { get; set; }
#endif

            // ----------------------------------------------------------------------
            QString& SpecialCharsRepresentation ();

            // ----------------------------------------------------------------------
            bool IsShowHiddenText ();

#if 0 //UNIMPLEMENTED, currently NOT USED
            // ----------------------------------------------------------------------
            public bool ConvertVisualHyperlinks { get; set; }
#endif
            // ----------------------------------------------------------------------
            bool UseNonBreakingSpaces ();

#if 0 //UNIMPLEMENTED, currently NOT USED
            // ----------------------------------------------------------------------
            public string ImagesPath { get; set; }

            // ----------------------------------------------------------------------
            public string GetImageUrl( int index, RtfVisualImageFormat rtfVisualImageFormat )
            {
                string imageFileName = imageAdapter.ResolveFileName( index, rtfVisualImageFormat );
                return imageFileName.Replace( '\\', '/' );
            } // GetImageUrl
#endif

            void setIgnoreBackgroundColor (bool set) {
                ignoreBGcolor = set;
            }
            bool ignoreBackgroundColor () {
                return ignoreBGcolor;
            }

        private:
            // ----------------------------------------------------------------------
            // members
#if 0 //UNIMPLEMENTED, currently NOT USED
            private readonly IRtfVisualImageAdapter imageAdapter;
            private RtfHtmlCssStyleCollection styles;
            private StringCollection styleSheetLinks;
#endif
            QString documentHeader;
            QString characterSet;
            QString title;

            RtfHtmlConvertScope::RtfHtmlConvertScope convertScope;
            bool ignoreBGcolor;
    }; // class RtfHtmlConvertSettings

} // namespace Itenso.Rtf.Converter.Html
// -- EOF -------------------------------------------------------------------
#endif //__RTFHTMLCONVERTSETTINGS_H