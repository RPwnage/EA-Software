<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:template match="/">
    <xsl:apply-templates select="serverstatus/database/connectionpools"/>
  </xsl:template>

  <xsl:template match="entry">
    <xsl:value-of select="dbconnpoolstatus/database"/>
    <xsl:text> </xsl:text>
    <xsl:value-of select="dbconnpoolstatus/name"/>
    <xsl:text> master </xsl:text>
    <xsl:value-of select="dbconnpoolstatus/master/host"/>
    <xsl:text> </xsl:text>
    <xsl:value-of select="dbconnpoolstatus/master/totalqueries + dbconnpoolstatus/master/totalmultiqueries"/>
    <xsl:text> </xsl:text>
    <xsl:value-of select="dbconnpoolstatus/master/totalerrors"/>
    <xsl:text>&#xa;</xsl:text>
    <xsl:for-each select="dbconnpoolstatus/slaves/dbinstancepoolstatus">
      <xsl:value-of select="../../database"/>
      <xsl:text> </xsl:text>
      <xsl:value-of select="../../name"/>
      <xsl:text> slave </xsl:text>
      <xsl:value-of select="host"/>
      <xsl:text> </xsl:text>
      <xsl:value-of select="totalqueries + totalmultiqueries"/>
      <xsl:text> </xsl:text>
      <xsl:value-of select="totalerrors"/>
      <xsl:text>&#xa;</xsl:text>
    </xsl:for-each>
  </xsl:template>

</xsl:stylesheet>

