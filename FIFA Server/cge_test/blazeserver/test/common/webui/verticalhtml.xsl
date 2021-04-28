<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:variable name="outermostElementName" select="name(/*)" />
<xsl:variable name="spaces">&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;</xsl:variable>

<xsl:template match="/">
  <html>
  <body>
  
  <!-- BSTODO: UNDER CONSTRUCTION, TO CLEAN UP.  Replace this with the genereic veritcalreport.xsl -->
  <h2><xsl:value-of select="$outermostElementName"/> <br/> <xsl:value-of select="//testid/id" /> at <xsl:value-of select="//*/rundate" /> </h2>
  <table border="1" align="left">
    <tr bgcolor="#9acd32">
    <th>Item</th>
    <th>Result</th>
    </tr>
    <xsl:for-each select="//./*">
      <tr>
        <td>
            <xsl:value-of select="substring($spaces, 0, 4*count(ancestor::*))"/>
            <xsl:value-of select="local-name()"/>
        </td>
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
                            <xsl:if test="preceding-sibling::*">:</xsl:if>
                            <xsl:value-of select="current()"/>
                        </xsl:when>
                        <xsl:otherwise><xsl:if test="preceding-sibling::*">,  </xsl:if></xsl:otherwise>
                </xsl:choose>
            </xsl:for-each>
        </td>
      </tr>
    </xsl:for-each>

    <!-- TODO: NOT SURE IF SOMETHING LIKE BELOW BLOCK NEEDED LATER, FOR COMPLEX CHAINS, ITS CURRENTLY SOMEWHAT BUGGED: -->
    <!-- <xsl:for-each select="//./*"> -->
      <!-- <tr> -->
      
        <!-- <xsl:for-each select="child::*"> -->
                 <!-- if not a multi descendant node, print the value now  -->
                <!-- <xsl:if test="not(descendant::*)"> -->
                  <!-- <tr> -->
                    <!-- <td> -->
                        <!-- <xsl:value-of select="substring($spaces, 0, 4*count(ancestor::*))"/> -->
                        <!-- <xsl:value-of select="local-name()"/> -->
                    <!-- </td> -->
                    <!-- <td> -->
                        <!-- <xsl:value-of select="current()"/> -->
                    <!-- </td> -->
                   <!-- </tr> -->
                <!-- </xsl:if> -->
                 <!-- handle multi descendant node, like properties, print only at leafs  -->
                <!-- <xsl:for-each select="descendant::*"> -->
                    <!-- <xsl:choose> -->
                         <!-- at leaf, print the value  -->
                        <!-- <xsl:when test="not(*)"> -->
                          <!-- <xsl:if test="not(preceding-sibling::*)"> -->
                            <!-- <td> -->
                                <!-- <xsl:value-of select="substring($spaces, 0, 4*count(ancestor::*))"/> -->
                                <!-- <xsl:value-of select="local-name()"/> -->
                            <!-- </td> -->
                            <!-- <td> -->
                                <!-- <xsl:value-of select="current()"/> -->
                            <!-- </td> -->
                          <!-- </xsl:if> -->
                        <!-- </xsl:when> -->
                    <!-- </xsl:choose> -->
                <!-- </xsl:for-each> -->
            
         <!-- </xsl:for-each> -->
      <!-- </tr> -->
    <!-- </xsl:for-each> -->
        
    
  </table>
  </body>
  </html>
</xsl:template>

</xsl:stylesheet>