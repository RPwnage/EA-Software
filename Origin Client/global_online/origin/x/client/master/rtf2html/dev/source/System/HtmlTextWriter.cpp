// HtmlTextWriter.h
// make a class to mimic to .NET's HtmlTextWriter

#include <assert.h>

#include <qstring.h>
#include <qurl.h>
#include <qtextdocument.h>
#include "HtmlTextWriter.h"

namespace RTF2HTML
{
    namespace HtmlTextWriterNS
    {
        namespace TagType
        {
            enum TagEnum
            {
                Inline = 0,
                NonClosing,
                Other
            };
        }

        typedef struct TagInfoType
        {
            char* name;
            TagType::TagEnum type;
            char* closingTag;
        } TagInfoType;

        typedef struct AttrInfoType
        {
            char* name;

        } AttrInfoType;

        const QChar TagLeftChar = '<';
        const QChar TagRightChar = '>';
        const QString SelfClosingChars = " /";
        const QString SelfClosingTagEnd = " />";
        const QString EndTagLeftChars = "</";
        const QChar DoubleQuoteChar = '"';
        const QChar SingleQuoteChar = '\'';
        const QChar SpaceChar = ' ';
        const QChar EqualsChar = '=';
        const QChar SlashChar = '/';
        const QString EqualsDoubleQuoteString = "=\"";
        const QChar SemicolonChar = ';';
        const QChar StyleEqualsChar = ':';
        const QString DefaultTabString = "\t";

    }

    using namespace HtmlTextWriterNS;

    static HtmlTextWriterNS::TagInfoType TagInfoTable[HtmlTextWriterTag::Xml+1] =
    {
        {""           , TagType::Other, "</>"},        //HtmlTextWriterTag.Unknown
        {"a"          , TagType::Inline, "</a>"},       //HtmlTextWriterTag.A
        {"acronym"    , TagType::Inline, "</acronym>"},       //HtmlTextWriterTag.Acronym
        {"address"    , TagType::Other, "</address>"},        //.HtmlTextWriterTag.Address
        {"area"       , TagType::NonClosing, ""},   //HtmlTextWriterTag.Area
        {"b"          , TagType::Inline, "</b>"},       //HtmlTextWriterTag.B
        {"base"       , TagType::NonClosing, ""},   //HtmlTextWriterTag.Base
        {"basefont"   , TagType::NonClosing, ""},   //HtmlTextWriterTag.Basefont
        {"bdo"        , TagType::Inline, "</bdo>"},       //HtmlTextWriterTag.Bdo
        {"bgsound"    , TagType::NonClosing, ""},   //HtmlTextWriterTag.Bgsound
        {"big"        , TagType::Inline, "</big>"},       //HtmlTextWriterTag.Big
        {"blockquote" , TagType::Other, "</blockquote>"},        //HtmlTextWriterTag.Blockquote
        {"body"       , TagType::Other, "</body>"},        //HtmlTextWriterTag.Body
        {"br"         , TagType::Other, "</br>"},        //HtmlTextWriterTag.Br
        {"button"     , TagType::Inline, "</button>"},       //HtmlTextWriterTag.Button
        {"caption"    , TagType::Other, "</caption>"},        //HtmlTextWriterTag.Caption
        {"center"     , TagType::Other, "</center>"},        //HtmlTextWriterTag.Center
        {"cite"       , TagType::Inline, "</cite>"},       //HtmlTextWriterTag.Cite
        {"code"       , TagType::Inline, "</code>"},       //HtmlTextWriterTag.Code
        {"col"        , TagType::NonClosing, ""},   //HtmlTextWriterTag.Col
        {"colgroup"   , TagType::Other, "</colgroup>"},        //HtmlTextWriterTag.Colgroup
        {"dd"         , TagType::Inline, "</dd>"},       //HtmlTextWriterTag.Dd
        {"del"        , TagType::Inline, "</del>"},       //HtmlTextWriterTag.Del
        {"dfn"        , TagType::Inline, "</dfn>"},       //HtmlTextWriterTag.Dfn
        {"dir"        , TagType::Other, "</dir>"},        //HtmlTextWriterTag.Dir
        {"div"        , TagType::Other, "</div>"},        //HtmlTextWriterTag.Div
        {"dl"         , TagType::Other, "</dl>"},        //HtmlTextWriterTag.Dl
        {"dt"         , TagType::Inline, "</dt>"},       //HtmlTextWriterTag.Dt
        {"em"         , TagType::Inline, "</em>"},       //HtmlTextWriterTag.Em
        {"embed"      , TagType::NonClosing, ""},   //HtmlTextWriterTag.Embed
        {"fieldset"   , TagType::Other, "</fieldset>"},        // HtmlTextWriterTag.Fieldset
        {"font"       , TagType::Inline, "</font>"},       //Font
        {"form"       , TagType::Other, "</form>"},        //Form
        {"frame"      , TagType::NonClosing, ""},   //Frame
        {"frameset"   , TagType::Other, "</frameset>"},        //Frameset
        {"h1"         , TagType::Other, "</h1>"},        //H1
        {"h2"         , TagType::Other, "</h2>"},        //H2
        {"h3"         , TagType::Other, "</h3>"},        //H3
        {"h4"         , TagType::Other, "</h4>"},        //H4
        {"h5"         , TagType::Other, "</h5>"},        //H5
        {"h6"         , TagType::Other, "</h6>"},        //H6
        {"head"       , TagType::Other, "</head>"},        //Head
        {"hr"         , TagType::NonClosing, ""},   //Hr
        {"html"       , TagType::Other, "</html>"},        //Html
        {"i"          , TagType::Inline, "</i>"},       //I
        {"iframe"     , TagType::Other, "</iframe>"},        //Iframe
        {"img"        , TagType::NonClosing, ""},   //Img
        {"input"      , TagType::NonClosing, ""},   //Input
        {"ins"        , TagType::Inline, "</ins>"},       //Ins
        {"isindex"    , TagType::NonClosing, ""},   //Isindex
        {"kbd"        , TagType::Inline, "</kbd>"},       //Kbd,
        {"label"      , TagType::Inline, "</label>"},       //Label
        {"legend"     , TagType::Other, "</legend>"},        //Legend
        {"li"         , TagType::Inline, "</li>"},       //Li
        {"link"       , TagType::NonClosing, ""},   //Link
        {"map"        , TagType::Other, "</map>"},        //Map
        {"marquee"    , TagType::Other, "</marquee>"},        //Marquee
        {"menu"       , TagType::Other, "</menu>"},        //Menu
        {"meta"       , TagType::NonClosing, ""},   //Meta
        {"nobr"       , TagType::Inline, "</nobr>"},       //Nobr
        {"noframes"   , TagType::Other, "</noframes>"},        //Noframes
        {"noscript"   , TagType::Other, "</noscript>"},        //Noscript
        {"object"     , TagType::Other, "</object>"},        //Object
        {"ol"         , TagType::Other, "</ol>"},        //Ol
        {"option"     , TagType::Other, "</option>"},        //Option
        {"p"          , TagType::Inline, "</p>"},       //P
        {"param"      , TagType::Other, "</param>"},        //Param
        {"pre"        , TagType::Other, "</pre>"},        //Pre
        {"q"          , TagType::Inline, "</q>"},       //Q
        {"rt"         , TagType::Other, "</rt>"},        //Rt
        {"ruby"       , TagType::Other, "</ruby>"},        //Ruby
        {"s"          , TagType::Inline, "</s>"},       //S
        {"samp"       , TagType::Inline, "</samp>"},       //Samp
        {"script"     , TagType::Other, "</script>"},        //Script
        {"select"     , TagType::Other, "</select>"},        //Select
        {"small"      , TagType::Other, "</small>"},        //Small
        {"span"       , TagType::Inline, "</span>"},       //Span
        {"strike"     , TagType::Inline, "</strike>"},       //Strike
        {"strong"     , TagType::Inline, "</strong>"},       //Strong
        {"style"      , TagType::Other, "<style/>"},        //Style
        {"sub"        , TagType::Inline, "</sub>"},       //Sub
        {"sup"        , TagType::Inline, "</sup>"},       //Sup
        {"table"      , TagType::Other, "</table>"},        //Table
        {"tbody"      , TagType::Other, "</tbody>"},        //Tbody
        {"td"         , TagType::Inline, "</td>"},       //Td
        {"textarea"   , TagType::Inline, "</textarea>"},       //Textarea
        {"tfoot"      , TagType::Other, "</tfoot>"},        //Tfoot
        {"th"         , TagType::Inline, "</th>"},       //Th
        {"thead"      , TagType::Other, "</thead>"},        //Thead
        {"title"      , TagType::Other, "</title>"},        //Title
        {"tr"         , TagType::Other, "</tr>"},        //Tr
        {"tt"         , TagType::Inline, "</tt>"},       //Tt
        {"u"          , TagType::Inline, "</u>"},       //U
        {"ul"         , TagType::Other, "</ul>"},        //Ul
        {"var"        , TagType::Inline, "</var>"},       //Var
        {"wbr"        , TagType::NonClosing, ""},   //Wbr
        {"xml"        , TagType::Other, "</xml>"}        //Xml
    };



    HtmlTextWriter::HtmlTextWriter ()
    {
        tabString = DefaultTabString;
        indentLevel = 0;
        tabsPending = false;

        _isDescendant = false; //(GetType() != typeof(HtmlTextWriter));

        _attrCount = 0;
        _styleCount = 0;
        _endTagCount = 0;
        _inlineCount = 0;

        RegisterAttributes();
    }

    HtmlTextWriter::~HtmlTextWriter()
    {
    }

    void HtmlTextWriter::AddAttribute (HtmlTextWriterAttribute::Attr key, QString value)
    {
        QString attrName;
        bool encode;
        bool isUrl;
        QHash<HtmlTextWriterAttribute::Attr, AttrInfo>::iterator it;

        it = _attrNameLookup.find (key);
        if (it != _attrNameLookup.end())
        {
            attrName = (*it).name;
            encode = (*it).encode;
            isUrl = (*it).isUrl;

            AddAttribute (attrName, value, key, encode, isUrl);
        }
        else
        {
            assert (0);
        }

    }

    void HtmlTextWriter::AddAttribute (QString name, QString value)
    {
        AddAttribute (name, value, HtmlTextWriterAttribute::Unknown, false, false);
    }

    void HtmlTextWriter::AddAttribute (QString name, QString value, HtmlTextWriterAttribute::Attr key, bool encode, bool isUrl)
    {
        if (_attrCount >= MAX_ATTR_LIST)
        {
            assert (0);
        }
        else
        {
            RenderAttribute* attr = &_attrList[_attrCount];

            attr->name = name;
            attr->value = value;
            attr->key = key;
            attr->encode = encode;
            attr->isUrl = isUrl;
            _attrCount++;
        }
    }

    void HtmlTextWriter::AddStyleAttribute (HtmlTextWriterStyle::Style key, QString value)
    {
        using namespace HtmlTextWriterStyle;
        switch (key)
        {
            case TextAlign:
                styleAttribute += "text-align:" + value + ";";
                break;

            case BackgroundColor:
                styleAttribute += "background-color:" + value + ";";
                break;

            case Color:
                styleAttribute += "color:" + value + ";";
                break;

            case FontFamily:
                styleAttribute += "font-family:" + value + ";";
                break;

            case FontSize:
                styleAttribute += "font-size:" + value + ";";
                break;

            //rest is unimplemented
            default:
                assert (0);
                break;
#if 0 //UNIMPLEMENTED currently NOT USED
            case BackgroundImage:
            case BorderCollapse:
            case BorderColor:
            case BorderStyle:
            case BorderWidth:
            case FontStyle:
FontWeight:
Height:
TextDecoration:
Width:
ListStyleImage:
ListStyleType:
Cursor:
Direction:
Display:
Filter:
FontVariant:
Left:
Margin:
MarginBottom:
MarginLeft:
MarginRight:
MarginTop:
Overflow:
OverflowX:
OverflowY:
Padding:
PaddingBottom:
PaddingLeft:
PaddingRight:
PaddingTop:
Position:
VerticalAlign:
TextOverflow:
Top:
Visibility:
WhiteSpace:
ZIndex:
#endif
        }

    }

    void HtmlTextWriter::RenderBeginTag( HtmlTextWriterTag::Tag tag )
    {
        _tagKey = tag;
        bool renderTag = true;

        // gather information about this tag.
        TagInfoType* tagInfo = &(TagInfoTable[_tagKey]);
        TagType::TagEnum tagType = tagInfo->type;

        bool renderEndTag = renderTag && (tagType != TagType::NonClosing);
        QString endTag = renderEndTag? tagInfo->closingTag : "";

        // write the begin tag
        if (renderTag) {
            if (tabsPending) {
                OutputTabs();
            }

            Write(TagLeftChar);
            Write(tagInfo->name);

            QString styleValue;

            for (int i = 0; i < _attrCount; i++) {
                RenderAttribute* attr = &(_attrList[i]);
                if (attr->key == HtmlTextWriterAttribute::Style) {
                    // append style attribute in with other styles
                    styleValue += attr->value;
                }
                else {
                    Write(SpaceChar);
                    Write(attr->name);
                    if (!attr->value.isEmpty()) {
                        Write(EqualsDoubleQuoteString);

                        QString attrValue = attr->value;
                        if (attr->isUrl) {
                            if (attr->key != HtmlTextWriterAttribute::Href && attrValue.indexOf("javascript:") != 0) {
                                attrValue = QUrl::toPercentEncoding(attrValue);
                            }
                        }
#if 0 //UNIMPLEMENTED, currently NOT USED -- none of our stuff is encoded
                        if (attr->encode) {
                            WriteHtmlAttributeEncode(attrValue);
                        }
                        else {
                            Write(attrValue);
                        }
#else
                        Write (attrValue);
#endif
                        Write(DoubleQuoteChar);
                    }
                }
            }


//            if (_styleCount > 0 || styleValue != NULL) {
            if (!styleAttribute.isEmpty())
            {
                Write(SpaceChar);
                Write("style");
                Write(EqualsDoubleQuoteString);

#if 0 //UNIMPLEMENTED currently NOT USED
                CssTextWriter.WriteAttributes(writer, _styleList, _styleCount);
                if (styleValue != null) {
                    writer.Write(styleValue);
                }
#endif
                Write (styleAttribute);
                styleAttribute.clear();
                Write(DoubleQuoteChar);
            }

            if (tagType == TagType::NonClosing) {
                Write(SelfClosingTagEnd);
            }
            else {
                Write(TagRightChar);
            }
        }

#if 0 //UNIMPLEMENTED currently NOT USED
        string textBeforeContent = RenderBeforeContent();
        if (textBeforeContent != null) {
            if (tabsPending) {
                OutputTabs();
            }
            writer.Write(textBeforeContent);
        }
#endif
        // write text before the content
        if (renderEndTag) {
            if (tagType == TagType::Inline) {
                _inlineCount += 1;
            }
            else {
                // writeline and indent before rendering content
                WriteLine();
                indentLevel++;
            }
            // Manually build end tags for unknown tag types.
            if (endTag.isEmpty()) {
                endTag = EndTagLeftChars + _tagName + TagRightChar; //.ToString(CultureInfo.InvariantCulture);
            }
        }

#if 0 //UNIMPLEMENTED currently NOT USED
        if (_isDescendant) {
            // append text after the tag
            string textAfterTag = RenderAfterTag();
            if (textAfterTag != null) {
                endTag = (endTag == null) ? textAfterTag : textAfterTag + endTag;
            }

            // build end content and push it on stack to write in RenderEndTag
            // prepend text after the content
            string textAfterContent = RenderAfterContent();
            if (textAfterContent != null) {
                endTag = (endTag == null) ? textAfterContent : textAfterContent + endTag;
            }
        }
#endif

        // push end tag onto stack
        PushEndTag(endTag);

        // flush attribute and style lists for next tag
        _attrCount = 0;
        _styleCount = 0;
    }

    void HtmlTextWriter::RenderEndTag ()
    {
        QString endTag = PopEndTag();

        if (!endTag.isEmpty())
        {
            if (TagInfoTable [_tagKey].type == TagType::Inline)
            {
                _inlineCount -= 1;
                // Never inject crlfs at end of inline tags.
                //
                Write(endTag);
            }
            else
            {
                // unindent if not an inline tag
                WriteLine();
                indentLevel--;
                Write(endTag);
            }
        }
    }

    void HtmlTextWriter::Write (QString str)
    {
        if (tabsPending)
        {
            OutputTabs();
        }
        htmlString += str;
    }

    void HtmlTextWriter::WriteBreak()
    {
        Write ("<br />");
    }

    void HtmlTextWriter::WriteLine()
    {
        Write ("\n");
        tabsPending = true;
    }

    void HtmlTextWriter::WriteLine (QString line)
    {
        if (tabsPending) {
            OutputTabs();
        }
        Write(line);
        WriteLine();
        tabsPending = true;
    }

    void HtmlTextWriter::OutputTabs ()
    {
        if (tabsPending)
        {
            for (int i=0; i < indentLevel; i++)
            {
                htmlString += tabString;
            }
            tabsPending = false;
        }
    }

    QString HtmlTextWriter::PopEndTag ()
    {
        EndTagEntry et = endTagStack.pop ();
        _tagKey = et.tagKey;
        return et.endTagText;
    }

    void HtmlTextWriter::PushEndTag (QString& endTag)
    {
        EndTagEntry et;

        et.tagKey = _tagKey;
        et.endTagText = endTag;

        endTagStack.push (et);
    }

    void HtmlTextWriter::RegisterAttribute (char* attrName, HtmlTextWriterAttribute::Attr attribute, bool encode, bool isUrl = false)
    {
        AttrInfo ainfo;
        ainfo.name = attrName;
        ainfo.encode = encode;
        ainfo.isUrl = isUrl;

        _attrNameLookup [attribute] = ainfo;
    }

    void HtmlTextWriter::RegisterAttributes ()
    {
        RegisterAttribute("abbr",            HtmlTextWriterAttribute::Abbr,           true);
        RegisterAttribute("accesskey",       HtmlTextWriterAttribute::Accesskey,      true);
        RegisterAttribute("align",           HtmlTextWriterAttribute::Align,          false);
        RegisterAttribute("alt",             HtmlTextWriterAttribute::Alt,            true);
        RegisterAttribute("autocomplete",    HtmlTextWriterAttribute::AutoComplete,   false);
        RegisterAttribute("axis",            HtmlTextWriterAttribute::Axis,           true);
        RegisterAttribute("background",      HtmlTextWriterAttribute::Background,     true,     true);
        RegisterAttribute("bgcolor",         HtmlTextWriterAttribute::Bgcolor,        false);
        RegisterAttribute("border",          HtmlTextWriterAttribute::Border,         false);
        RegisterAttribute("bordercolor",     HtmlTextWriterAttribute::Bordercolor,    false);
        RegisterAttribute("cellpadding",     HtmlTextWriterAttribute::Cellpadding,    false);
        RegisterAttribute("cellspacing",     HtmlTextWriterAttribute::Cellspacing,    false);
        RegisterAttribute("checked",         HtmlTextWriterAttribute::Checked,        false);
        RegisterAttribute("class",           HtmlTextWriterAttribute::Class,          true);
        RegisterAttribute("cols",            HtmlTextWriterAttribute::Cols,           false);
        RegisterAttribute("colspan",         HtmlTextWriterAttribute::Colspan,        false);
        RegisterAttribute("content",         HtmlTextWriterAttribute::Content,        true);
        RegisterAttribute("coords",          HtmlTextWriterAttribute::Coords,         false);
        RegisterAttribute("dir",             HtmlTextWriterAttribute::Dir,            false);
        RegisterAttribute("disabled",        HtmlTextWriterAttribute::Disabled,       false);
        RegisterAttribute("for",             HtmlTextWriterAttribute::For,            false);
        RegisterAttribute("headers",         HtmlTextWriterAttribute::Headers,        true);
        RegisterAttribute("height",          HtmlTextWriterAttribute::Height,         false);
        RegisterAttribute("href",            HtmlTextWriterAttribute::Href,           true,      true);
        RegisterAttribute("id",              HtmlTextWriterAttribute::Id,             false);
        RegisterAttribute("longdesc",        HtmlTextWriterAttribute::Longdesc,       true,      true);
        RegisterAttribute("maxlength",       HtmlTextWriterAttribute::Maxlength,      false);
        RegisterAttribute("multiple",        HtmlTextWriterAttribute::Multiple,       false);
        RegisterAttribute("name",            HtmlTextWriterAttribute::Name,           false);
        RegisterAttribute("nowrap",          HtmlTextWriterAttribute::Nowrap,         false);
        RegisterAttribute("onclick",         HtmlTextWriterAttribute::Onclick,        true);
        RegisterAttribute("onchange",        HtmlTextWriterAttribute::Onchange,       true);
        RegisterAttribute("readonly",        HtmlTextWriterAttribute::ReadOnly,       false);
        RegisterAttribute("rel",             HtmlTextWriterAttribute::Rel,            false);
        RegisterAttribute("rows",            HtmlTextWriterAttribute::Rows,           false);
        RegisterAttribute("rowspan",         HtmlTextWriterAttribute::Rowspan,        false);
        RegisterAttribute("rules",           HtmlTextWriterAttribute::Rules,          false);
        RegisterAttribute("scope",           HtmlTextWriterAttribute::Scope,          false);
        RegisterAttribute("selected",        HtmlTextWriterAttribute::Selected,       false);
        RegisterAttribute("shape",           HtmlTextWriterAttribute::Shape,          false);
        RegisterAttribute("size",            HtmlTextWriterAttribute::Size,           false);
        RegisterAttribute("src",             HtmlTextWriterAttribute::Src,            true,      true);
        RegisterAttribute("style",           HtmlTextWriterAttribute::Style,          false);
        RegisterAttribute("tabindex",        HtmlTextWriterAttribute::Tabindex,       false);
        RegisterAttribute("target",          HtmlTextWriterAttribute::Target,         false);
        RegisterAttribute("title",           HtmlTextWriterAttribute::Title,          true);
        RegisterAttribute("type",            HtmlTextWriterAttribute::Type,           false);
        RegisterAttribute("usemap",          HtmlTextWriterAttribute::Usemap,         false);
        RegisterAttribute("valign",          HtmlTextWriterAttribute::Valign,         false);
        RegisterAttribute("value",           HtmlTextWriterAttribute::Value,          true);
        RegisterAttribute("vcard_name",      HtmlTextWriterAttribute::VCardName,      false);
        RegisterAttribute("width",           HtmlTextWriterAttribute::Width,          false);
        RegisterAttribute("wrap",            HtmlTextWriterAttribute::Wrap,           false);
    }

    QString& HtmlTextWriter::toString()
    {
        return htmlString;
    }

} // namespace Itenso.Rtf.Model
// -- EOF -------------------------------------------------------------------
