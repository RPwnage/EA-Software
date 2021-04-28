<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:template match="/">
    <xsl:apply-templates select="componentmetrics//commandmetricinfo[count > 0]" />
  </xsl:template>
  <xsl:template match="commandmetricinfo">
    <xsl:if test="count(errorcountmap) > 0">
      <xsl:for-each  select="errorcountmap/entry">
    <xsl:value-of select="../../componentname" />
    <xsl:text> </xsl:text>
    <xsl:value-of select="../../commandname"/>
    <xsl:text> </xsl:text>
    <xsl:value-of select="../../count"/>
    <xsl:text> </xsl:text>
    <xsl:value-of select="../../failcount"/>
    <xsl:text> </xsl:text>
    <xsl:value-of select="@key"/>
    <xsl:text> </xsl:text>
    <xsl:value-of select="."/>
    <xsl:text>&#xa;</xsl:text>
      </xsl:for-each>
    </xsl:if>
    <xsl:if test="count(errorcountmap) = 0">
    <xsl:value-of select="componentname" />
    <xsl:text> </xsl:text>
    <xsl:value-of select="commandname"/>
    <xsl:text> </xsl:text>
    <xsl:value-of select="count"/>
    <xsl:text> 0 NO_ERROR 0</xsl:text>
    <xsl:text>&#xa;</xsl:text>
    </xsl:if>
  </xsl:template>
</xsl:stylesheet>

