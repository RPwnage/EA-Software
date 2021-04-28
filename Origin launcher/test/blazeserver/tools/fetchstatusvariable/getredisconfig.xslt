<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:template match="/">
    <xsl:apply-templates select="fetchcomponentconfiguration/configtdfs/entry/frameworkconfig/redisconfig/clusters"/>
  </xsl:template>

  <xsl:template match="entry">
    <xsl:for-each select="redisclusterconfig/nodes/nodes">
      <xsl:value-of select="../../../../entry/@key"/>
      <xsl:text> </xsl:text>
      <xsl:value-of select="."/>
      <xsl:text>&#xa;</xsl:text>
    </xsl:for-each>
  </xsl:template>

</xsl:stylesheet>

