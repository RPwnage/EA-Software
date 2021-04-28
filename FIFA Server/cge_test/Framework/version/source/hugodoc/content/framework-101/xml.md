---
title: XML Introduction
weight: 7
---

Framework build scripts are written in XML, so it is important to have an understanding of XML before working with build scripts.
This page contains a concise guide of what we expect our users to know about XML.
If you want to read more about XML another good tutorial is [XML for Absolute Beginner - A Short Tutorial.](http://www.perfectxml.com/articles/xml/begin.asp) 

<a name="About"></a>
## What is XML? ##

Extensible Markup Language (XML) is a language that defines a set of rules for encoding data in a way that is both human and machine readable.
Like HTML, XML is one of the standards defined by the W3C.
There are many document formats that use XML syntax, such as XHTML, SVG, RSS, etc.

###### An example of XML ######

```xml
<?xml version="1.0" ?>
<!--My first XML document -->
<User>
 <FirstName>Stan</FirstName>
 <LastName>Smith</LastName>
 <Type>Admin</Type>
 <BirthDate>11/04/1974</BirthDate>
</User>
```
<a name="structure"></a>
## Document Structure ##

XML has a hierarchical structure, where all element have a parent-child relationship with other elements.
There is always a single root element and this comprises the body of the document.
However, in addition to body the document can also start with an optional prolog element (&lt;?xml version=&quot;1.0&quot; ?&gt;) that describes the format of the document.

There is always only a single root element, also called the document element.
All other elements are children of this element.

All elements have a start tag (&lt;Name&gt;) and an end tag (&lt;/Name&gt;).
Within these tags the element may contain text data or nested child elements.
An element that does not contain content can be declared with a single element provided it closes itself with a slash at the end (&lt;Person name=&#39;Mike&#39;/&gt;).

XML elements can also have one or more attributes (&lt;Player Id=&quot;2&quot;&gt;).
All attributes are always enclosed within quotation marks (either &#39; or &quot;).

It is also important to remember that XML documents are case-sensitive, and the case of elements and attributes can change the meaning of the data you are describing.

<a name="SpecialCharacters"></a>
## Special Characters ##

There are some (actually five) characters that have special meaning. They are &lt;, &gt;, &#39;, &quot;, and &amp;.
If you wish to use any of these directly (and not for markup), you should escape them using&lt;![CDATA[&lt;, &gt;, &#39;, &quot;, and &amp;]]&gt;respectively.

Another thing to watch out for is fancy unicode characters like curved quotes (“ ”) or various types of dashes.
Occasionally users may copy a line of XML from an email, website or other test editor that uses the curved quotes instead of the standard quotes.
XML will not treat them the same as regular quotes and you will likely get an error.

<a name="Comments"></a>
## Comments ##

Comments in XML are defined by a start tag (&lt;!––) and end tag (––&gt;).
Comments can be declared anywhere between elements, but you cannot use them within a tag to comment out specific attributes.
Comments also have a restriction that they cannot contain dashes unless they are separated by a space.

<a name="CDATA"></a>
## Embedding code fragments using CDATA ##

You can embed fragments of other programming languages in an XML document. But to do so, you have to embed them in [CDATA.](https://en.wikipedia.org/wiki/CDATA) elements.
This is because languages like C#/C++ have characters such as &#39;&lt;&#39; or &#39;&amp;&#39; that are reserved in XML. Otherwise, you&#39;ll have XML syntax error.

