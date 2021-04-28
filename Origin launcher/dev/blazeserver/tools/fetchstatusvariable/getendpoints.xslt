<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:param name="instance"/>
<xsl:param name="servicename"/>
<xsl:param name="channel"/>
<xsl:param name="protocol"/>
<xsl:param name="bindtype"/> <!-- 0=internal; 1=external -->
<xsl:param name="addrtype" select="0"/>
<xsl:template match="/">
    <xsl:if test="$instance='-'">
      <xsl:apply-templates select="serverlist//serverinfodata[servicenames/servicenames=$servicename]//serverendpointinfo[protocol=$protocol and channel=$channel and bindtype=$bindtype]//serveraddressinfo[type=$addrtype]//ip"/>
    </xsl:if>
    <xsl:if test="$instance!='-'">
      <xsl:apply-templates select="serverlist//serverinfodata[servicenames/servicenames=$servicename]//serverinstance[starts-with(instancename,$instance)]//serverendpointinfo[protocol=$protocol and channel=$channel and bindtype=$bindtype]//serveraddressinfo[type=$addrtype]//ip"/>
      <xsl:apply-templates select="serverlist//serverinfodata[servicenames/servicenames=$servicename]//masterinstance[starts-with(instancename,$instance)]//serverendpointinfo[protocol=$protocol and channel=$channel and bindtype=$bindtype]//serveraddressinfo[type=$addrtype]//ip"/>
    </xsl:if>
</xsl:template>
<xsl:template match="ip">
   <xsl:value-of select="../../../../../../../instancename/." />
    <xsl:text> </xsl:text>
    <xsl:value-of select="."/>
    <xsl:text> </xsl:text>
    <xsl:value-of select="../port"/>
    <xsl:text> </xsl:text>
    <xsl:value-of select="../../../../../../../inservice/."/>
    <xsl:text>&#xa;</xsl:text>
  </xsl:template>
</xsl:stylesheet>

