<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:template match="/">
    <xsl:apply-templates select="serverstatus//componentstatus/info/entry"/>
    <xsl:apply-templates select="serverstatus/memory/processresidentmemory"/>
    <xsl:apply-templates select="serverstatus/fibermanagerstatus/cpuusageforprocesspercent"/>
    <xsl:apply-templates select="serverstatus/version"/>
</xsl:template>

  <xsl:template match="entry">
    <xsl:value-of select="../../componentname/." />
    <xsl:text>.</xsl:text>
    <xsl:value-of select="@key"/>
    <xsl:text>= </xsl:text>
    <xsl:value-of select="."/>
    <xsl:text>&#xa;</xsl:text>
  </xsl:template>

  <xsl:template match="processresidentmemory">
    <xsl:text>blazeinstance.processresidentmemory= </xsl:text>
    <xsl:value-of select="."/>
    <xsl:text>&#xa;</xsl:text>
  </xsl:template>

  <xsl:template match="cpuusageforprocesspercent">
    <xsl:text>blazeinstance.cpuusageforprocesspercent= </xsl:text>
    <xsl:value-of select="."/>
    <xsl:text>&#xa;</xsl:text>
  </xsl:template>

  <xsl:template match="version">
    <xsl:text>blazeinstance.sourcelocation= </xsl:text>
    <xsl:value-of select="p4depotlocation"/>
    <xsl:text>&#xa;</xsl:text>
    <xsl:text>blazeinstance.buildtime= </xsl:text>
    <xsl:value-of select="buildtime"/>
    <xsl:text>&#xa;</xsl:text>
    <xsl:text>blazeinstance.version= </xsl:text>
    <xsl:value-of select="version"/>
    <xsl:text>&#xa;</xsl:text>
  </xsl:template>

</xsl:stylesheet>

