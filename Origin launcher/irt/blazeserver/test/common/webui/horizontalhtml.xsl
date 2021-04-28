<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:variable name="outermostElementName" select="name(/*)" />

<xsl:template match="/">
  <html>
  <body>
  <h2><xsl:value-of select="$outermostElementName"/> <br/> <xsl:value-of select="//testid/id" /> at <xsl:value-of select="//*/rundate" /> </h2>
  <table border="1" align="left">
    <tr bgcolor="#9acd32">
      <xsl:for-each select="//./results[1]/*">
        <th>
            <!-- at first level descendants, we print name of the field -->
            <xsl:value-of select="local-name()"/>
            
            <!-- after the first level descendants, need to handle things like properties maps, print the branch with delimiting character -->
            <xsl:for-each select="descendant::*">
                <xsl:choose>
                    <!-- at leaf, print a field separator between siblings. -->
                    <xsl:when test="not(*)">
                        <xsl:if test="preceding-sibling::*">:</xsl:if>
                        <xsl:if test="not(preceding-sibling::*)">/</xsl:if>
                    </xsl:when>
                    <xsl:otherwise>
                        <xsl:if test="preceding-sibling::*">,&#160;|</xsl:if>
                        <xsl:if test="not(preceding-sibling::*)">//<br/></xsl:if>
                    </xsl:otherwise>
                </xsl:choose>
                <xsl:value-of select="local-name()"/>
            </xsl:for-each>
        </th>
        
      </xsl:for-each>
    </tr>
    <xsl:for-each select="//./results">
    <tr>
      <xsl:for-each select="child::*">
        <td>
            <!-- if not a multi descendant node, print the value now -->
            <xsl:if test="not(descendant::*)">
              <xsl:value-of select="current()"/>
            </xsl:if>
            <!-- handle multi descendant node, like properties, print only at leafs -->
            <xsl:for-each select="descendant::*">
                <xsl:choose>
                    <!-- at leaf, print the value -->
                        <xsl:when test="not(*)">
                            <xsl:if test="preceding-sibling::*">: &#160;</xsl:if>
                            <xsl:value-of select="current()"/>
                            <!-- TODO: pad to keep things better aligned, something like below, but note longer strings using below wont work as they can get clipped.
                            <xsl:value-of select="substring(concat('&#160;_____', current()), string-length(current()) + 1, 15)"/> 
                            -->
                        </xsl:when>
                        <xsl:otherwise><xsl:if test="preceding-sibling::*">, &#160; | </xsl:if></xsl:otherwise>
                </xsl:choose>
            </xsl:for-each>
        </td>
      </xsl:for-each>
    </tr>
    </xsl:for-each>
  </table>
  </body>
  </html>
</xsl:template>

</xsl:stylesheet>