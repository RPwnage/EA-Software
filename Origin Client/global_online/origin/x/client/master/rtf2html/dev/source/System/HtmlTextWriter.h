// HtmlTextWriter.h
// make a class to mimic to .NET's HtmlTextWriter

#ifndef __HTMLTEXTWRITER_H
#define __HTMLTEXTWRITER_H

#include <qstring.h>
#include <qstack.h>
#include <qtextdocument.h>

#include "HtmlTextWriterEnums.h"

namespace RTF2HTML
{
    // ------------------------------------------------------------------------
    class HtmlTextWriter: public QTextDocument
    {
        public:
            HtmlTextWriter();
            ~HtmlTextWriter();

            void AddAttribute (HtmlTextWriterAttribute::Attr key, QString value);
            void AddAttribute (QString name, QString value);

            void AddStyleAttribute (HtmlTextWriterStyle::Style key, QString value);

            void RenderBeginTag( HtmlTextWriterTag::Tag tag );
            void RenderEndTag();

            void Write (QString str);
            void WriteBreak();
            void WriteLine();
            void WriteLine (QString line);

            QString& toString();

        private:

            static const int MAX_ATTR_LIST = 20;

            typedef struct AttrInfo
            {
                QString name;
                bool encode;
                bool isUrl;
            };

            typedef struct RenderAttribute
            {
                QString name;
                QString value;
                HtmlTextWriterAttribute::Attr key;
                bool encode;
                bool isUrl;
            } RenderAttribute;

            typedef struct EndTagEntry
            {
                HtmlTextWriterTag::Tag tagKey;
                QString endTagText;
            } EndTagEntry;

            void AddAttribute (QString name, QString value, HtmlTextWriterAttribute::Attr key, bool encode, bool isUrl);

            void OutputTabs ();
            QString PopEndTag();
            void PushEndTag (QString& endTag);
            void RegisterAttribute (char* attrName, HtmlTextWriterAttribute::Attr attribute, bool encode, bool isUrl);
            void RegisterAttributes ();


            QString htmlString;
            QStack <HtmlTextWriterTag::Tag> htmlStack;
            QString styleAttribute;

            int indentLevel;
            bool tabsPending;
            QString tabString;

            int _attrCount;
            int _endTagCount;
            //TagStackEntry[] _endTags;
            //HttpWriter _httpWriter;
            int _inlineCount;
            bool _isDescendant;
            //RenderStyle[] _styleList;
            int _styleCount;
            int _tagIndex;
            //HtmlTextWriterTag _tagKey;
            QString _tagName;
            HtmlTextWriterTag::Tag _tagKey;

            RenderAttribute _attrList [MAX_ATTR_LIST];
            QStack <EndTagEntry> endTagStack;
            QHash<HtmlTextWriterAttribute::Attr, AttrInfo> _attrNameLookup;
    };
} // namespace Itenso.Rtf.Model
#endif //__HTMLTEXTWRITER_H
// -- EOF -------------------------------------------------------------------
