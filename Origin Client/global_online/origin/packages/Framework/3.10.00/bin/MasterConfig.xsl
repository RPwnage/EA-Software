<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:msxsl="urn:schemas-microsoft-com:xslt"
    xmlns:framework="urn:Framework2"
    exclude-result-prefixes="msxsl framework"
>
  <xsl:output method="xml" indent="yes" omit-xml-declaration="no" encoding="utf-8"/>
  <xsl:strip-space elements="*"/>
  
  <!-- select all version attributes with a package element as their parent. Replace the contents with the value returned from the update-package-version function -->
  <xsl:template match="@version">
    <xsl:attribute name="version">
      <xsl:value-of select="framework:update_package_version(../@name, .)"/>
    </xsl:attribute>
  </xsl:template>

  <!-- Adding <ondemand> at the end of the file if not present. -->
  <xsl:template match="project[not(ondemand)]">
      <xsl:copy>
        <xsl:apply-templates select="@*"/>
        <xsl:apply-templates select="node()"/>
        <xsl:element name="ondemand">
          <xsl:text>true</xsl:text>
        </xsl:element>
      </xsl:copy>
  </xsl:template>
  
  <!-- Ensure that <ondemand> value is set to default (true) if empty. -->
  <xsl:template match="ondemand[not(text())]">
    <xsl:element name="ondemand">
      <xsl:text>true</xsl:text>
    </xsl:element>
  </xsl:template>

  <xsl:template match="@* | node()">
        <xsl:copy>
            <xsl:apply-templates select="@* | node()"/>
        </xsl:copy>
  </xsl:template>
</xsl:stylesheet>
