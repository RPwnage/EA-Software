<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:msxsl="urn:schemas-microsoft-com:xslt" exclude-result-prefixes="msxsl">

  <xsl:output method="xml" indent="yes" version="1.0" encoding="utf-8"/>

  <xsl:template match="/">
    <doc>
      <xsl:apply-templates />
    </doc>
  </xsl:template>

  <xsl:template match="members">
    <members>
      <xsl:apply-templates />
    </members>
  </xsl:template>

  <xsl:template match="member">
    <member>
      <xsl:attribute name="name">
        <xsl:value-of select ="@name"/>
      </xsl:attribute>
      <xsl:apply-templates />
    </member>
  </xsl:template>

  <xsl:template match="summary | remarks | example | returns">
    <section>
      <title>
        <xsl:value-of select ="local-name()"/>
      </title>
      <content>
        <xsl:apply-templates />
      </content>
    </section>
  </xsl:template>

  <xsl:template match="param">
    <section>
      <title>
        param: <xsl:value-of select="@name"/>
      </title>
      <content>
        <xsl:apply-templates/>
      </content>
    </section>
  </xsl:template>

  <xsl:template match="code">
    <code>
      <xsl:apply-templates />
    </code>
  </xsl:template>

  <xsl:template match="para">
    <para>
      <xsl:apply-templates />
    </para>
  </xsl:template>

  <xsl:template match="see">
    <token>
      <xsl:value-of select="@cref"/>
    </token>
  </xsl:template>

  <xsl:template match="b">
    <legacyBold>
      <xsl:apply-templates />
    </legacyBold>
  </xsl:template>

  <xsl:template match="note">
    <alert class="note">
      <xsl:if test="@type!=''">
        <xsl:attribute name="class">
          <xsl:value-of select="@type"/>
        </xsl:attribute>
      </xsl:if>
      <xsl:apply-templates />
    </alert>
  </xsl:template>

  <xsl:template match="c">
    <codeInline>
      <xsl:value-of select="."/>
    </codeInline>
  </xsl:template>

  <xsl:template match="list[@type!='table']">
    <list class="bullet">
      <xsl:apply-templates select="item"/>
    </list>
  </xsl:template>

  <xsl:template match="list[@type!='table']/item">
    <listItem>
      <xsl:apply-templates/>
    </listItem>
  </xsl:template>

  <xsl:template match="list[@type='table']">
    <table>
      <xsl:apply-templates select="listheader"/>
      <xsl:apply-templates select="item"/>
    </table>
  </xsl:template>

  <xsl:template match="listheader">
    <tableHeader>
      <row>
        <xsl:apply-templates select="term | description"/>
      </row>
    </tableHeader>
  </xsl:template>

  <xsl:template match="item">
    <row>
      <xsl:apply-templates select="term | description"/>
    </row>
  </xsl:template>

  <xsl:template match="term | description">
    <entry>
      <para>
        <xsl:apply-templates/>
      </para>
    </entry>
  </xsl:template>
</xsl:stylesheet>
