// HtmlTextWriterEnums.h
// make a class to mimic to .NET's HtmlTextWriter

#ifndef __HTMLTEXTWRITERENUMS_H
#define __HTMLTEXTWRITERENUMS_H


namespace RTF2HTML
{
    namespace HtmlTextWriterTag
    {
        enum Tag
        {
            // Summary:
            //     The string passed as an HTML tag is not recognized.
            Unknown = 0,
            //
            // Summary:
            //     The HTML a element.
            A = 1,
            //
            // Summary:
            //     The HTML acronym element.
            Acronym = 2,
            //
            // Summary:
            //     The HTML address element.
            Address = 3,
            //
            // Summary:
            //     The HTML area element.
            Area = 4,
            //
            // Summary:
            //     The HTML b element.
            B = 5,
            //
            // Summary:
            //     The HTML base element.
            Base = 6,
            //
            // Summary:
            //     The HTML basefont element.
            Basefont = 7,
            //
            // Summary:
            //     The HTML bdo element.
            Bdo = 8,
            //
            // Summary:
            //     The HTML bgsound element.
            Bgsound = 9,
            //
            // Summary:
            //     The HTML big element.
            Big = 10,
            //
            // Summary:
            //     The HTML blockquote element.
            Blockquote = 11,
            //
            // Summary:
            //     The HTML body element.
            Body = 12,
            //
            // Summary:
            //     The HTML br element.
            Br = 13,
            //
            // Summary:
            //     The HTML button element.
            Button = 14,
            //
            // Summary:
            //     The HTML caption element.
            Caption = 15,
            //
            // Summary:
            //     The HTML center element.
            Center = 16,
            //
            // Summary:
            //     The HTML cite element.
            Cite = 17,
            //
            // Summary:
            //     The HTML code element.
            Code = 18,
            //
            // Summary:
            //     The HTML col element.
            Col = 19,
            //
            // Summary:
            //     The HTML colgroup element.
            Colgroup = 20,
            //
            // Summary:
            //     The HTML dd element.
            Dd = 21,
            //
            // Summary:
            //     The HTML del element.
            Del = 22,
            //
            // Summary:
            //     The HTML dfn element.
            Dfn = 23,
            //
            // Summary:
            //     The HTML dir element.
            Dir = 24,
            //
            // Summary:
            //     The HTML div element.
            Div = 25,
            //
            // Summary:
            //     The HTML dl element.
            Dl = 26,
            //
            // Summary:
            //     The HTML dt element.
            Dt = 27,
            //
            // Summary:
            //     The HTML em element.
            Em = 28,
            //
            // Summary:
            //     The HTML embed element.
            Embed = 29,
            //
            // Summary:
            //     The HTML fieldset element.
            Fieldset = 30,
            //
            // Summary:
            //     The HTML font element.
            Font = 31,
            //
            // Summary:
            //     The HTML form element.
            Form = 32,
            //
            // Summary:
            //     The HTML frame element.
            Frame = 33,
            //
            // Summary:
            //     The HTML frameset element.
            Frameset = 34,
            //
            // Summary:
            //     The HTML H1 element.
            H1 = 35,
            //
            // Summary:
            //     The HTML H2 element.
            H2 = 36,
            //
            // Summary:
            //     The HTML H3 element.
            H3 = 37,
            //
            // Summary:
            //     The HTML H4 element.
            H4 = 38,
            //
            // Summary:
            //     The HTML H5 element.
            H5 = 39,
            //
            // Summary:
            //     The HTML H6 element.
            H6 = 40,
            //
            // Summary:
            //     The HTML head element.
            Head = 41,
            //
            // Summary:
            //     The HTML hr element.
            Hr = 42,
            //
            // Summary:
            //     The HTML html element.
            Html = 43,
            //
            // Summary:
            //     The HTML i element.
            I = 44,
            //
            // Summary:
            //     The HTML iframe element.
            Iframe = 45,
            //
            // Summary:
            //     The HTML img element.
            Img = 46,
            //
            // Summary:
            //     The HTML input element.
            Input = 47,
            //
            // Summary:
            //     The HTML ins element.
            Ins = 48,
            //
            // Summary:
            //     The HTML isindex element.
            Isindex = 49,
            //
            // Summary:
            //     The HTML kbd element.
            Kbd = 50,
            //
            // Summary:
            //     The HTML label element.
            Label = 51,
            //
            // Summary:
            //     The HTML legend element.
            Legend = 52,
            //
            // Summary:
            //     The HTML li element.
            Li = 53,
            //
            // Summary:
            //     The HTML link element.
            Link = 54,
            //
            // Summary:
            //     The HTML map element.
            Map = 55,
            //
            // Summary:
            //     The HTML marquee element.
            Marquee = 56,
            //
            // Summary:
            //     The HTML menu element.
            Menu = 57,
            //
            // Summary:
            //     The HTML meta element.
            Meta = 58,
            //
            // Summary:
            //     The HTML nobr element.
            Nobr = 59,
            //
            // Summary:
            //     The HTML noframes element.
            Noframes = 60,
            //
            // Summary:
            //     The HTML noscript element.
            Noscript = 61,
            //
            // Summary:
            //     The HTML object element.
            Object = 62,
            //
            // Summary:
            //     The HTML ol element.
            Ol = 63,
            //
            // Summary:
            //     The HTML option element.
            Option = 64,
            //
            // Summary:
            //     The HTML p element.
            P = 65,
            //
            // Summary:
            //     The HTML param element.
            Param = 66,
            //
            // Summary:
            //     The HTML pre element.
            Pre = 67,
            //
            // Summary:
            //     The HTML q element.
            Q = 68,
            //
            // Summary:
            //     The DHTML rt element, which specifies text for the ruby element.
            Rt = 69,
            //
            // Summary:
            //     The DHTML ruby element.
            Ruby = 70,
            //
            // Summary:
            //     The HTML s element.
            S = 71,
            //
            // Summary:
            //     The HTML samp element.
            Samp = 72,
            //
            // Summary:
            //     The HTML script element.
            Script = 73,
            //
            // Summary:
            //     The HTML select element.
            Select = 74,
            //
            // Summary:
            //     The HTML small element.
            Small = 75,
            //
            // Summary:
            //     The HTML span element.
            Span = 76,
            //
            // Summary:
            //     The HTML strike element.
            Strike = 77,
            //
            // Summary:
            //     The HTML strong element.
            Strong = 78,
            //
            // Summary:
            //     The HTML style element.
            Style = 79,
            //
            // Summary:
            //     The HTML sub element.
            Sub = 80,
            //
            // Summary:
            //     The HTML sup element.
            Sup = 81,
            //
            // Summary:
            //     The HTML table element.
            Table = 82,
            //
            // Summary:
            //     The HTML tbody element.
            Tbody = 83,
            //
            // Summary:
            //     The HTML td element.
            Td = 84,
            //
            // Summary:
            //     The HTML textarea element.
            Textarea = 85,
            //
            // Summary:
            //     The HTML tfoot element.
            Tfoot = 86,
            //
            // Summary:
            //     The HTML th element.
            Th = 87,
            //
            // Summary:
            //     The HTML thead element.
            Thead = 88,
            //
            // Summary:
            //     The HTML title element.
            Title = 89,
            //
            // Summary:
            //     The HTML tr element.
            Tr = 90,
            //
            // Summary:
            //     The HTML tt element.
            Tt = 91,
            //
            // Summary:
            //     The HTML u element.
            U = 92,
            //
            // Summary:
            //     The HTML ul element.
            Ul = 93,
            //
            // Summary:
            //     The HTML var element.
            Var = 94,
            //
            // Summary:
            //     The HTML wbr element.
            Wbr = 95,
            //
            // Summary:
            //     The HTML xml element.
            Xml = 96,
        };
    }

    // Summary:
    //     Specifies the HTML attributes that an System.Web.UI.HtmlTextWriter or System.Web.UI.Html32TextWriter
    //     object writes to the opening tag of an HTML element when a Web request is
    //     processed.
    namespace HtmlTextWriterAttribute
    {
        enum Attr
        {
            Unknown = -1,
            // Summary:
            //     Specifies that the HTML accesskey attribute be written to the tag.
            Accesskey = 0,
            //
            // Summary:
            //     Specifies that the HTML align attribute be written to the tag.
            Align = 1,
            //
            // Summary:
            //     Specifies that the HTML alt attribute be written to the tag.
            Alt = 2,
            //
            // Summary:
            //     Specifies that the HTML background attribute be written to the tag.
            Background = 3,
            //
            // Summary:
            //     Specifies that the HTML bgcolor attribute be written to the tag.
            Bgcolor = 4,
            //
            // Summary:
            //     Specifies that the HTML border attribute be written to the tag.
            Border = 5,
            //
            // Summary:
            //     Specifies that the HTML bordercolor attribute be written to the tag.
            Bordercolor = 6,
            //
            // Summary:
            //     Specifies that the HTML cellpadding attribute be written to the tag.
            Cellpadding = 7,
            //
            // Summary:
            //     Specifies that the HTML cellspacing attribute be written to the tag.
            Cellspacing = 8,
            //
            // Summary:
            //     Specifies that the HTML checked attribute be written to the tag.
            Checked = 9,
            //
            // Summary:
            //     Specifies that the HTML class attribute be written to the tag.
            Class = 10,
            //
            // Summary:
            //     Specifies that the HTML cols attribute be written to the tag.
            Cols = 11,
            //
            // Summary:
            //     Specifies that the HTML colspan attribute be written to the tag.
            Colspan = 12,
            //
            // Summary:
            //     Specifies that the HTML disabled attribute be written to the tag.
            Disabled = 13,
            //
            // Summary:
            //     Specifies that the HTML for attribute be written to the tag.
            For = 14,
            //
            // Summary:
            //     Specifies that the HTML height attribute be written to the tag.
            Height = 15,
            //
            // Summary:
            //     Specifies that the HTML href attribute be written to the tag.
            Href = 16,
            //
            // Summary:
            //     Specifies that the HTML id attribute be written to the tag.
            Id = 17,
            //
            // Summary:
            //     Specifies that the HTML maxlength attribute be written to the tag.
            Maxlength = 18,
            //
            // Summary:
            //     Specifies that the HTML multiple attribute be written to the tag.
            Multiple = 19,
            //
            // Summary:
            //     Specifies that the HTML name attribute be written to the tag.
            Name = 20,
            //
            // Summary:
            //     Specifies that the HTML nowrap attribute be written to the tag.
            Nowrap = 21,
            //
            // Summary:
            //     Specifies that the HTML onchange attribute be written to the tag.
            Onchange = 22,
            //
            // Summary:
            //     Specifies that the HTML onclick attribute be written to the tag.
            Onclick = 23,
            //
            // Summary:
            //     Specifies that the HTML readonly attribute be written to the tag.
            ReadOnly = 24,
            //
            // Summary:
            //     Specifies that the HTML rows attribute be written to the tag.
            Rows = 25,
            //
            // Summary:
            //     Specifies that the HTML rowspan attribute be written to the tag.
            Rowspan = 26,
            //
            // Summary:
            //     Specifies that the HTML rules attribute be written to the tag.
            Rules = 27,
            //
            // Summary:
            //     Specifies that the HTML selected attribute be written to the tag.
            Selected = 28,
            //
            // Summary:
            //     Specifies that the HTML size attribute be written to the tag.
            Size = 29,
            //
            // Summary:
            //     Specifies that the HTML src attribute be written to the tag.
            Src = 30,
            //
            // Summary:
            //     Specifies that the HTML style attribute be written to the tag.
            Style = 31,
            //
            // Summary:
            //     Specifies that the HTML tabindex attribute be written to the tag.
            Tabindex = 32,
            //
            // Summary:
            //     Specifies that the HTML target attribute be written to the tag.
            Target = 33,
            //
            // Summary:
            //     Specifies that the HTML title attribute be written to the tag.
            Title = 34,
            //
            // Summary:
            //     Specifies that the HTML type attribute be written to the tag.
            Type = 35,
            //
            // Summary:
            //     Specifies that the HTML valign attribute be written to the tag.
            Valign = 36,
            //
            // Summary:
            //     Specifies that the HTML value attribute be written to the tag.
            Value = 37,
            //
            // Summary:
            //     Specifies that the HTML width attribute be written to the tag.
            Width = 38,
            //
            // Summary:
            //     Specifies that the HTML wrap attribute be written to the tag.
            Wrap = 39,
            //
            // Summary:
            //     Specifies that the HTML abbr attribute be written to the tag.
            Abbr = 40,
            //
            // Summary:
            //     Specifies that the HTML autocomplete attribute be written to the tag.
            AutoComplete = 41,
            //
            // Summary:
            //     Specifies that the HTML axis attribute be written to the tag.
            Axis = 42,
            //
            // Summary:
            //     Specifies that the HTML content attribute be written to the tag.
            Content = 43,
            //
            // Summary:
            //     Specifies that the HTML coords attribute be written to the tag.
            Coords = 44,
            //
            // Summary:
            //     Specifies that the HTML designerregion attribute be written to the tag.
            DesignerRegion = 45,
            //
            // Summary:
            //     Specifies that the HTML dir attribute be written to the tag.
            Dir = 46,
            //
            // Summary:
            //     Specifies that the HTML headers attribute be written to the tag.
            Headers = 47,
            //
            // Summary:
            //     Specifies that the HTML longdesc attribute be written to the tag.
            Longdesc = 48,
            //
            // Summary:
            //     Specifies that the HTML rel attribute be written to the tag.
            Rel = 49,
            //
            // Summary:
            //     Specifies that the HTML scope attribute be written to the tag.
            Scope = 50,
            //
            // Summary:
            //     Specifies that the HTML shape attribute be written to the tag.
            Shape = 51,
            //
            // Summary:
            //     Specifies that the HTML usemap attribute be written to the tag.
            Usemap = 52,
            //
            // Summary:
            //     Specifies that the HTML vcardname attribute be written to the tag.
            VCardName = 53,
        };
    }

    namespace HtmlTextWriterStyle
    {
        // Summary:
        //     Specifies the HTML styles available to an System.Web.UI.HtmlTextWriter or
        //     System.Web.UI.Html32TextWriter object output stream.
        enum Style
        {
            // Summary:
            //     Specifies the HTML backgroundcolor style.
            BackgroundColor = 0,
            //
            // Summary:
            //     Specifies the HTML backgroundimage style.
            BackgroundImage = 1,
            //
            // Summary:
            //     Specifies the HTML bordercollapse style.
            BorderCollapse = 2,
            //
            // Summary:
            //     Specifies the HTML bordercolor style.
            BorderColor = 3,
            //
            // Summary:
            //     Specifies the HTML borderstyle style.
            BorderStyle = 4,
            //
            // Summary:
            //     Specifies the HTML borderwidth style.
            BorderWidth = 5,
            //
            // Summary:
            //     Specifies the HTML color style.
            Color = 6,
            //
            // Summary:
            //     Specifies the HTML fontfamily style.
            FontFamily = 7,
            //
            // Summary:
            //     Specifies the HTML fontsize style.
            FontSize = 8,
            //
            // Summary:
            //     Specifies the HTML fontstyle style.
            FontStyle = 9,
            //
            // Summary:
            //     Specifies the HTML fontheight style.
            FontWeight = 10,
            //
            // Summary:
            //     Specifies the HTML height style.
            Height = 11,
            //
            // Summary:
            //     Specifies the HTML textdecoration style.
            TextDecoration = 12,
            //
            // Summary:
            //     Specifies the HTML width style.
            Width = 13,
            //
            // Summary:
            //     Specifies the HTML liststyleimage style.
            ListStyleImage = 14,
            //
            // Summary:
            //     Specifies the HTML liststyletype style.
            ListStyleType = 15,
            //
            // Summary:
            //     Specifies the HTML cursor style.
            Cursor = 16,
            //
            // Summary:
            //     Specifies the HTML direction style.
            Direction = 17,
            //
            // Summary:
            //     Specifies the HTML display style.
            Display = 18,
            //
            // Summary:
            //     Specifies the HTML filter style.
            Filter = 19,
            //
            // Summary:
            //     Specifies the HTML fontvariant style.
            FontVariant = 20,
            //
            // Summary:
            //     Specifies the HTML left style.
            Left = 21,
            //
            // Summary:
            //     Specifies the HTML margin style.
            Margin = 22,
            //
            // Summary:
            //     Specifies the HTML marginbottom style.
            MarginBottom = 23,
            //
            // Summary:
            //     Specifies the HTML marginleft style.
            MarginLeft = 24,
            //
            // Summary:
            //     Specifies the HTML marginright style.
            MarginRight = 25,
            //
            // Summary:
            //     Specifies the HTML margintop style.
            MarginTop = 26,
            //
            // Summary:
            //     Specifies the HTML overflow style.
            Overflow = 27,
            //
            // Summary:
            //     Specifies the HTML overflowx style.
            OverflowX = 28,
            //
            // Summary:
            //     Specifies the HTML overflowy style.
            OverflowY = 29,
            //
            // Summary:
            //     Specifies the HTML padding style.
            Padding = 30,
            //
            // Summary:
            //     Specifies the HTML paddingbottom style.
            PaddingBottom = 31,
            //
            // Summary:
            //     Specifies the HTML paddingleft style.
            PaddingLeft = 32,
            //
            // Summary:
            //     Specifies the HTML paddingright style.
            PaddingRight = 33,
            //
            // Summary:
            //     Specifies the HTML paddingtop style.
            PaddingTop = 34,
            //
            // Summary:
            //     Specifies the HTML position style.
            Position = 35,
            //
            // Summary:
            //     Specifies the HTML textalign style.
            TextAlign = 36,
            //
            // Summary:
            //     Specifies the HTML verticalalign style.
            VerticalAlign = 37,
            //
            // Summary:
            //     Specifies the HTML textoverflow style.
            TextOverflow = 38,
            //
            // Summary:
            //     Specifies the HTML top style.
            Top = 39,
            //
            // Summary:
            //     Specifies the HTML visibility style.
            Visibility = 40,
            //
            // Summary:
            //     Specifies the HTML whitespace style.
            WhiteSpace = 41,
            //
            // Summary:
            //     Specifies the HTML zindex style.
            ZIndex = 42,
        };
    }
} // namespace Itenso.Rtf.Model
#endif //__HTMLTEXTWRITERENUMS_H
// -- EOF -------------------------------------------------------------------
