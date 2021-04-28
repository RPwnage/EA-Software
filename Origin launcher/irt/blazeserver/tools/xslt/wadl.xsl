<?xml version="1.0" encoding="UTF-8"?>
<!--
  wadl_documentation.xsl 
  
  \Version 2009-08-27 
  
  Modified for Blaze use.
  
  \Version 2008-12-09

  An XSLT stylesheet for generating HTML documentation from WADL,
  by Mark Nottingham <mnot@yahoo-inc.com>.

  Copyright (c) 2006-2008 Yahoo! Inc.
  
  This work is licensed under the Creative Commons Attribution-ShareAlike 2.5 
  License. To view a copy of this license, visit 
    http://creativecommons.org/licenses/by-sa/2.5/ 
  or send a letter to 
    Creative Commons
    543 Howard Street, 5th Floor
    San Francisco, California, 94105, USA
-->

<xsl:stylesheet 
 xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"
 xmlns:wadl="http://research.sun.com/wadl/2006/10"
 xmlns:xs="http://www.w3.org/2001/XMLSchema"
 xmlns:html="http://www.w3.org/1999/xhtml"
 xmlns:exsl="http://exslt.org/common"
 xmlns:ns="urn:namespace"
 extension-element-prefixes="exsl"
 xmlns="http://www.w3.org/1999/xhtml"
 exclude-result-prefixes="xsl wadl xs html ns"
>

    <xsl:output 
        method="html" 
        encoding="UTF-8" 
        indent="yes"
        doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN"
        doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"
    />

    <xsl:variable name="wadl-ns">http://research.sun.com/wadl/2006/10</xsl:variable>

    <xsl:variable name="resources">
        <xsl:apply-templates select="/wadl:application/wadl:resources" mode="expand"/>
    </xsl:variable>
  
    <xsl:template match="wadl:resources" mode="expand">
        <xsl:variable name="base">
            <xsl:choose>
                <xsl:when test="substring(@base, string-length(@base), 1) = '/'">
                    <xsl:value-of select="substring(@base, 1, string-length(@base) - 1)"/>
                </xsl:when>
                <xsl:otherwise><xsl:value-of select="@base"/></xsl:otherwise>
            </xsl:choose>
        </xsl:variable>
        <xsl:element name="resources" namespace="{$wadl-ns}">
            <xsl:for-each select="namespace::*">
                <xsl:variable name="prefix" select="name(.)"/>
                <xsl:if test="$prefix">
                    <xsl:attribute name="ns:{$prefix}"><xsl:value-of select="."/></xsl:attribute>
                </xsl:if>
            </xsl:for-each>
            <xsl:apply-templates select="@*|node()" mode="expand">
                <xsl:with-param name="base" select="$base"/>
            </xsl:apply-templates>            
        </xsl:element>
    </xsl:template>
    
    <xsl:template match="wadl:resource[@type]" mode="expand" priority="1">
        <xsl:param name="base"></xsl:param>
        <xsl:variable name="uri" select="substring-before(@type, '#')"/>
        <xsl:variable name="id" select="substring-after(@type, '#')"/>
        <xsl:element name="resource" namespace="{$wadl-ns}">
            <xsl:attribute name="path"><xsl:value-of select="@path"/></xsl:attribute>
            <xsl:choose>
                <xsl:when test="$uri">
                    <xsl:variable name="included" select="document($uri, /)"/>
                    <xsl:copy-of select="$included/descendant::wadl:resource_type[@id=$id]/@*"/>
                    <xsl:attribute name="id"><xsl:value-of select="@type"/>#<xsl:value-of select="@path"/></xsl:attribute>
                    <xsl:apply-templates select="$included/descendant::wadl:resource_type[@id=$id]/*" mode="expand">
                        <xsl:with-param name="base" select="$uri"/>
                    </xsl:apply-templates>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:copy-of select="//resource_type[@id=$id]/@*"/>
                    <xsl:attribute name="id"><xsl:value-of select="$base"/>#<xsl:value-of select="@type"/>#<xsl:value-of select="@path"/></xsl:attribute>
                    <xsl:apply-templates select="//wadl:resource_type[@id=$id]/*" mode="expand">
                        <xsl:with-param name="base" select="$base"/>                        
                    </xsl:apply-templates>
                </xsl:otherwise>
            </xsl:choose>
            <xsl:apply-templates select="node()" mode="expand">
                <xsl:with-param name="base" select="$base"/>
            </xsl:apply-templates>
        </xsl:element>
    </xsl:template>
    
    <xsl:template match="wadl:*[@href]" mode="expand">
        <xsl:param name="base"></xsl:param>
        <xsl:variable name="uri" select="substring-before(@href, '#')"/>
        <xsl:variable name="id" select="substring-after(@href, '#')"/>
        <xsl:element name="{local-name()}" namespace="{$wadl-ns}">
            <xsl:copy-of select="@*"/>
            <xsl:choose>
                <xsl:when test="$uri">
                    <xsl:attribute name="id"><xsl:value-of select="@href"/></xsl:attribute>
                    <xsl:variable name="included" select="document($uri, /)"/>
                    <xsl:apply-templates select="$included/descendant::wadl:*[@id=$id]/*" mode="expand">
                        <xsl:with-param name="base" select="$uri"/>
                    </xsl:apply-templates>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:attribute name="id"><xsl:value-of select="$base"/>#<xsl:value-of select="$id"/></xsl:attribute>
                    <!-- xsl:attribute name="id"><xsl:value-of select="generate-id()"/></xsl:attribute -->
                    <xsl:attribute name="element"><xsl:value-of select="//wadl:*[@id=$id]/@element"/></xsl:attribute>
                    <xsl:attribute name="mediaType"><xsl:value-of select="//wadl:*[@id=$id]/@mediaType"/></xsl:attribute>                    
                    <xsl:attribute name="status"><xsl:value-of select="//wadl:*[@id=$id]/@status"/></xsl:attribute>                    
                    <xsl:attribute name="name"><xsl:value-of select="//wadl:*[@id=$id]/@name"/></xsl:attribute>                    
                    <xsl:apply-templates select="//wadl:*[@id=$id]/*" mode="expand">
                        <xsl:with-param name="base" select="$base"/>
                    </xsl:apply-templates>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:element>
    </xsl:template>
    
    <xsl:template match="node()[@id]" mode="expand">
        <xsl:param name="base"></xsl:param>
        <xsl:element name="{local-name()}" namespace="{$wadl-ns}">
            <xsl:copy-of select="@*"/>
            <xsl:attribute name="id"><xsl:value-of select="$base"/>#<xsl:value-of select="@id"/></xsl:attribute>
            <!-- xsl:attribute name="id"><xsl:value-of select="generate-id()"/></xsl:attribute -->
            <xsl:apply-templates select="node()" mode="expand">
                <xsl:with-param name="base" select="$base"/>
            </xsl:apply-templates>
        </xsl:element>
    </xsl:template>
    
    <xsl:template match="@*|node()" mode="expand">
        <xsl:param name="base"></xsl:param>
        <xsl:copy>
            <xsl:apply-templates select="@*|node()" mode="expand">
                <xsl:with-param name="base" select="$base"/>
            </xsl:apply-templates>
        </xsl:copy>
    </xsl:template>

<!-- debug $resources
    <xsl:template match="/">
    <xsl:copy-of select="$resources"/>
    </xsl:template>
-->
        
    <!-- collect grammars (TODO: walk over $resources instead) -->
    
    <xsl:variable name="grammars">
        <xsl:copy-of select="/wadl:application/wadl:grammars/*[not(namespace-uri()=$wadl-ns)]"/>
        <xsl:apply-templates select="/wadl:application/wadl:grammars/wadl:include[@href]" mode="include-grammar"/>
        <xsl:apply-templates select="/wadl:application/wadl:resources/descendant::wadl:resource[@type]" mode="include-href"/>
        <xsl:apply-templates select="exsl:node-set($resources)/descendant::wadl:*[@href]" mode="include-href"/>
    </xsl:variable>
    
    <xsl:template match="wadl:include[@href]" mode="include-grammar">
        <xsl:variable name="included" select="document(@href, /)/*"></xsl:variable>
        <xsl:element name="wadl:include">
            <xsl:attribute name="href"><xsl:value-of select="@href"/></xsl:attribute>
            <xsl:copy-of select="$included"/> <!-- FIXME: xml-schema includes, etc -->
        </xsl:element>
    </xsl:template>
    
    <xsl:template match="wadl:*[@href]" mode="include-href">
        <xsl:variable name="uri" select="substring-before(@href, '#')"/>
        <xsl:if test="$uri">
            <xsl:variable name="included" select="document($uri, /)"/>
            <xsl:copy-of select="$included/wadl:application/wadl:grammars/*[not(namespace-uri()=$wadl-ns)]"/>
            <xsl:apply-templates select="$included/descendant::wadl:include[@href]" mode="include-grammar"/>
            <xsl:apply-templates select="$included/wadl:application/wadl:resources/descendant::wadl:resource[@type]" mode="include-href"/>
            <xsl:apply-templates select="$included/wadl:application/wadl:resources/descendant::wadl:*[@href]" mode="include-href"/>
        </xsl:if>
    </xsl:template>
    
    <xsl:template match="wadl:resource[@type]" mode="include-href">
        <xsl:variable name="uri" select="substring-before(@type, '#')"/>
        <xsl:if test="$uri">
            <xsl:variable name="included" select="document($uri, /)"/>
            <xsl:copy-of select="$included/wadl:application/wadl:grammars/*[not(namespace-uri()=$wadl-ns)]"/>
            <xsl:apply-templates select="$included/descendant::wadl:include[@href]" mode="include-grammar"/>
            <xsl:apply-templates select="$included/wadl:application/wadl:resources/descendant::wadl:resource[@type]" mode="include-href"/>
            <xsl:apply-templates select="$included/wadl:application/wadl:resources/descendant::wadl:*[@href]" mode="include-href"/>
        </xsl:if>
    </xsl:template>
    
    <!-- main template -->
        
    <xsl:template match="/wadl:application">   
        <html>
            <head>
                <title>
                    <xsl:choose>
                        <xsl:when test="wadl:doc[@title]">
                            <xsl:value-of select="wadl:doc[@title][1]/@title"/>
                        </xsl:when>
                        <xsl:otherwise>Unknown Blaze Component</xsl:otherwise>
                    </xsl:choose>                 
                </title>
                <style type="text/css">
                  body {
                  font-family: arial, serif;
                  font-size: 0.85em;
                  margin: 2em 8em;
                  background-color: white;
                  background-image: url("gradient-down.png");
                  background-repeat: repeat-x;
                  }
                  .methods {
                  background-color: #DFEFFF;
                  padding: 1em;
                  }
                  .tdf {
                  background-color: #DFEFCF;
                  padding: 1em;
                  }
                  .notification {
                  background-color: #FFEEEE;
                  padding: 1em;
                  }
                  h1 {
                  font-size: 1.75em;
                  }
                  h2 {
                  border-bottom: 1px solid black;
                  margin-top: 1em;
                  margin-bottom: 0.5em;
                  font-size: 1.75em;
                  }
                  h3 {
                  color: #202937;
                  font-size: 1.5em;
                  margin-top: 1em;
                  margin-bottom: 0em;
                  }
                  h4 {
                  margin-top: 0em;
                  font-size: 1.1em;
                  padding: 0em;
                  }
                  h5 {
                  margin-bottom: 0.5em;
                  padding: 0em;
                  font-size: 1.2em;
                  border-bottom: 2px solid white;
                  }
                  h6 {
                  color: #223322;
                  font-size: 1.5em;
                  margin-top: 1em;
                  margin-bottom: 0em;
                  }
                  dd {
                  margin-left: 1em;
                  }
                  tt {
                  font-size: 1.2em;
                  }
                  table {
                  margin-bottom: 0.5em;
                  background-color: transparent;
                  }
                  th {
                  text-align: left;
                  font-weight: normal;
                  color: black;
                  border-bottom: 1px solid black;
                  padding: 3px 6px;
                  }
                  td {
                  padding: 3px 6px;
                  vertical-align: top;
                  background-color: transparent;
                  font-size: 0.9em;
                  }
                  td p {
                  margin: 0px;
                  }
                  ul {
                  padding-left: 1.75em;
                  }
                  p + ul, p + ol, p + dl {
                  margin-top: 0em;
                  margin-bottom: 0.5em;
                  }
                  p {
                  margin-top: 0em;
                  margin-bottom: 0.5em;
                  }
                  .optional {
                  font-weight: normal;
                  opacity: 0.75;
                  }
                </style>
            </head>
          <body>
            <h1>
              <a href="index.wadl.html">Blaze WAL</a> :: 
              <xsl:choose>
                <xsl:when test="wadl:doc[@title]">
                  <xsl:value-of select="wadl:doc[@title][1]/@title"/>
                </xsl:when>
                <xsl:otherwise>Unknown Blaze Component</xsl:otherwise>
              </xsl:choose>
            </h1>

            <p>
              <i>
                This document serves as a reference for the Web Application Layer.
                For a full guide to using the Web Application Layer,
                <a href="http://online.ea.com/confluence/display/tech/Web+Access+Layer"> see Confluence</a>
              </i>
              <br/>
            </p>
            
            <p>
              <xsl:apply-templates select="wadl:doc"/>
            </p>

            <p>
              <b>Base URI: </b>
              <tt>
                <xsl:value-of select="wadl:resources/@base"/>
              </tt>
            </p>

            <xsl:choose>
              <xsl:when test="wadl:doc[@title='index']">
                <ul>
                  <li>
                    <b>Components</b>
                    <xsl:apply-templates select="exsl:node-set($resources)" mode="index"/>
                  </li>
                </ul>
              </xsl:when>
              <xsl:otherwise>
                <ul>
                  <li>
                    <a href="#resources">
                      <b>RPC's</b>
                    </a>
                    <xsl:apply-templates select="exsl:node-set($resources)" mode="toc"/>
                  </li>
                  <li>
                    <a href="#notifications">
                      <b>Notifications</b>
                    </a>
                    <ul>
                      <xsl:apply-templates select="wadl:notification" mode="toc"/>
                    </ul>
                  </li>
                  <li>
                    <a href="#representations">
                      <b>TDF's</b>
                    </a>
                    <ul>
                      <xsl:apply-templates select="wadl:tdftypes/wadl:representation" mode="toc"/>
                    </ul>
                  </li>
                  <!--
                        <xsl:if test="descendant::wadl:fault">
                            <li><a href="#faults">Faults</a>
                                <ul>
                                    <xsl:apply-templates select="exsl:node-set($resources)/descendant::wadl:fault" mode="toc"/>
                                </ul>
                            </li>
                        </xsl:if>
                      -->
                </ul>
                <h2 id="resources">RPC's</h2>
                <xsl:apply-templates select="exsl:node-set($resources)" mode="list"/>
                <h2 id="notifications">Notifications</h2>
                <xsl:apply-templates select="wadl:notification"/>
                <h2 id="representations">TDF's</h2>

                <!-- Apply processing to each sub-element of an element-->
                <!--   <xsl:for-each select="wadl:resources/wadl:resource">
                      <p>
                        0
                        <xsl:value-of select="path"/>
                        a
                        <xsl:value-of select="@path"/>
                      </p>
                    </xsl:for-each>
                  -->
                <!-- Apply a template to child element tdftypes-->
                <xsl:apply-templates select="wadl:tdftypes"/>
                <!--
                  
                    <xsl:if test="exsl:node-set($resources)/descendant::wadl:fault"><h2 id="faults">Faults</h2>
                        <xsl:apply-templates select="exsl:node-set($resources)/descendant::wadl:fault" mode="list"/>
                    </xsl:if>
                    -->
              </xsl:otherwise>
            </xsl:choose>
          </body>
        </html>
    </xsl:template>

    <!-- Table of Contents -->

    <xsl:template match="wadl:resources" mode="toc">
        <xsl:variable name="base">
            <xsl:choose>
                <xsl:when test="substring(@base, string-length(@base), 1) = '/'">
                    <xsl:value-of select="substring(@base, 1, string-length(@base) - 1)"/>
                </xsl:when>
                <xsl:otherwise><xsl:value-of select="@base"/></xsl:otherwise>
            </xsl:choose>
        </xsl:variable>
        <ul>
            <xsl:apply-templates select="wadl:resource" mode="toc">
                <xsl:with-param name="context"><xsl:value-of select="$base"/></xsl:with-param>
            </xsl:apply-templates>
        </ul>        
    </xsl:template>

    <xsl:template match="wadl:resources" mode="index">
      <ul>
        <xsl:apply-templates select="wadl:resource" mode="index"/>
      </ul>
    </xsl:template>

    <xsl:template match="wadl:resource" mode="index">
      <xsl:variable name="name">
        <xsl:value-of select="@path"/>
      </xsl:variable>
      <li>
        <a href="{$name}.wadl.html">
          <xsl:value-of select="$name"/>
        </a>
        <xsl:if test="wadl:resource">
          <ul>
            <xsl:apply-templates select="wadl:resource" mode="index"/>
          </ul>
        </xsl:if>
      </li>
    </xsl:template>
  
    <xsl:template match="wadl:resource" mode="toc">
        <xsl:param name="context"/>
        <xsl:variable name="id"><xsl:call-template name="get-id"/></xsl:variable>
        <xsl:variable name="name"><xsl:value-of select="@path"/></xsl:variable>
        <li><a href="#{$id}"><xsl:value-of select="$name"/></a>
        <xsl:if test="wadl:resource">
            <ul>
                <xsl:apply-templates select="wadl:resource" mode="toc">
                    <xsl:with-param name="context" select="$name"/>
                </xsl:apply-templates>
            </ul>
        </xsl:if>
        </li>
    </xsl:template>

    <xsl:template match="wadl:representation" mode="toc">
      <xsl:variable name="anchor">
        <xsl:call-template name="representation-name-simple"/>
      </xsl:variable>
      <li>
        <a href="#{$anchor}">
          <xsl:call-template name="representation-name-simple"/>
        </a>
      </li>
    </xsl:template>

  <xsl:template match="wadl:notification" mode="toc">
    <xsl:variable name="anchor">Event<xsl:call-template name="representation-name-simple"/></xsl:variable>
    <li>
      <a href="#{$anchor}">
        <xsl:call-template name="representation-name-simple"/>
      </a>
    </li>
  </xsl:template>
  
  <xsl:template match="wadl:resources" mode="list">
        <xsl:variable name="base">
            <xsl:choose>
                <xsl:when test="substring(@base, string-length(@base), 1) = '/'">
                    <xsl:value-of select="substring(@base, 1, string-length(@base) - 1)"/>
                </xsl:when>
                <xsl:otherwise><xsl:value-of select="@base"/></xsl:otherwise>
            </xsl:choose>
        </xsl:variable>
        <xsl:apply-templates select="wadl:resource" mode="list"/>
    </xsl:template>
    
    <xsl:template match="wadl:resource" mode="list">
        <xsl:param name="context"/>
        <xsl:variable name="href" select="@id"/>
        <xsl:choose>
            <xsl:when test="preceding::wadl:resource[@id=$href]"/>
            <xsl:otherwise>
                <xsl:variable name="id"><xsl:call-template name="get-id"/></xsl:variable>
                <xsl:variable name="name">
                    <xsl:value-of select="$context"/>/<xsl:value-of select="@path"/>
                    <xsl:for-each select="wadl:param[@style='matrix']">
                        <span class="optional">;<xsl:value-of select="@name"/>=...</span>
                    </xsl:for-each>
                </xsl:variable>
                <div class="resource">
                    <h3 id="{$id}">
                        <xsl:choose>
                            <xsl:when test="wadl:doc[@title]"><xsl:value-of select="wadl:doc[@title][1]/@title"/></xsl:when>
                            <xsl:otherwise>
                                <xsl:copy-of select="$name"/>
                                <xsl:for-each select="wadl:method[1]/wadl:request/wadl:param[@style='query']">
                                    <xsl:choose>
                                        <xsl:when test="@required='true'">
                                            <xsl:choose>
                                                <xsl:when test="preceding-sibling::wadl:param[@style='query']">&amp;</xsl:when>
                                                <xsl:otherwise>?</xsl:otherwise>
                                            </xsl:choose>
                                            <xsl:value-of select="@name"/>
                                        </xsl:when>
                                        <xsl:otherwise>
                                            <span class="optional">
                                                <xsl:choose>
                                                    <xsl:when test="preceding-sibling::wadl:param[@style='query']">&amp;</xsl:when>
                                                    <xsl:otherwise>?</xsl:otherwise>
                                                </xsl:choose>
                                                <xsl:value-of select="@name"/>
                                            </span>
                                        </xsl:otherwise>
                                    </xsl:choose>
                                </xsl:for-each>
                            </xsl:otherwise>
                        </xsl:choose>
                        
                    </h3>
                    <xsl:apply-templates select="wadl:doc"/>
                    <xsl:apply-templates select="." mode="param-group">
                        <xsl:with-param name="prefix">resource-wide</xsl:with-param>
                        <xsl:with-param name="style">template</xsl:with-param>
                    </xsl:apply-templates>
                    <xsl:apply-templates select="." mode="param-group">                        
                        <xsl:with-param name="prefix">resource-wide</xsl:with-param>
                        <xsl:with-param name="style">matrix</xsl:with-param>
                    </xsl:apply-templates>                    
                    <div class="methods">
                        <xsl:apply-templates select="wadl:method"/>
                    </div>
                </div>
                <xsl:apply-templates select="wadl:resource" mode="list">
                    <xsl:with-param name="context" select="$name"/>
                </xsl:apply-templates>                
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>
            
    <xsl:template match="wadl:method">
        <xsl:variable name="id"><xsl:call-template name="get-id"/></xsl:variable>
        <div class="method">
            <h4 id="{$id}">HTTP Method: <xsl:value-of select="@name"/></h4>
            <xsl:apply-templates select="wadl:doc"/>                
            <xsl:apply-templates select="wadl:request"/>
            <xsl:apply-templates select="wadl:response"/>
        </div>
    </xsl:template>

    <xsl:template match="wadl:request">
        <h5>Input Params</h5>
        <xsl:apply-templates select="." mode="param-group-request">
            <xsl:with-param name="prefix">request</xsl:with-param>
            <xsl:with-param name="style">query</xsl:with-param>
        </xsl:apply-templates>
        <xsl:apply-templates select="." mode="param-group-request">
            <xsl:with-param name="prefix">request</xsl:with-param>
            <xsl:with-param name="style">header</xsl:with-param>
        </xsl:apply-templates> 
    </xsl:template>

    <xsl:template match="wadl:response">
        <h5>Response Representation</h5>
        <xsl:apply-templates select="." mode="param-group-response">
            <xsl:with-param name="prefix">response</xsl:with-param>
            <xsl:with-param name="style">header</xsl:with-param>
        </xsl:apply-templates> 
        <xsl:if test="wadl:representation">
            <xsl:apply-templates select="wadl:representation" mode="response"/>
        </xsl:if>
        <xsl:if test="wadl:fault">
          <h5>Potential Faults</h5>
          <table>
            <tr>
              <th>Error Code</th>
              <th>Description</th>
            </tr>
            <xsl:apply-templates select="wadl:fault"/>
          </table>
        </xsl:if>
    </xsl:template>

    <xsl:template match="wadl:fault">
      <tr>
        <td>
          <xsl:value-of select="wadl:doc/@title"/>
        </td>
        <td>
          <xsl:apply-templates select="wadl:doc"/>
        </td>
      </tr>
    </xsl:template>
    
    <xsl:template match="wadl:representation">
      <xsl:variable name="anchor">
        <xsl:call-template name="representation-name-simple"/>
      </xsl:variable>
      <h6 id="{$anchor}">
        <xsl:call-template name="representation-name-simple"/>
      </h6>
      <div class="tdf">
        <xsl:apply-templates select="wadl:doc"/>
        <xsl:if test="@element or wadl:param">
          <xsl:if test="@element">
            <xsl:call-template name="get-element">
              <xsl:with-param name="context" select="."/>
              <xsl:with-param name="qname" select="@element"/>
            </xsl:call-template>
          </xsl:if>
          <xsl:apply-templates select="." mode="param-group">
            <xsl:with-param name="style">plain</xsl:with-param>
          </xsl:apply-templates>
          <xsl:apply-templates select="." mode="param-group">
            <xsl:with-param name="style">header</xsl:with-param>
          </xsl:apply-templates>
        </xsl:if>
      </div>
    </xsl:template>

    <xsl:template match="wadl:tdftypes">
      <xsl:apply-templates select="wadl:representation"/>
    </xsl:template>

  <xsl:template match="wadl:notification">
    <xsl:variable name="anchor">Event<xsl:call-template name="representation-name-simple"/></xsl:variable>
    <h6 id="{$anchor}">
      <xsl:call-template name="representation-name-simple"/>
    </h6>    
    <div class="notification">
      <p>
        <b>ID</b> = <xsl:value-of select="@id"/>
      </p>
      <xsl:apply-templates select="wadl:doc"/>
      <xsl:apply-templates select="wadl:response"/>
    </div>
  </xsl:template>
  
    <xsl:template match="wadl:representation" mode="response">
      <xsl:variable name="anchor">
        <xsl:call-template name="representation-name-simple"/>
      </xsl:variable>
      <p>
        <a href="#{$anchor}">
          <xsl:call-template name="representation-name"/>

        </a>
      </p>
      <xsl:apply-templates select="wadl:doc"/>
      <xsl:if test="@element or wadl:param">
        <xsl:if test="@element">
          <xsl:call-template name="get-element">
             <xsl:with-param name="context" select="."/>
             <xsl:with-param name="qname" select="@element"/>
          </xsl:call-template>
       </xsl:if>
            <xsl:apply-templates select="." mode="param-group-response">
              <xsl:with-param name="style">plain</xsl:with-param>
            </xsl:apply-templates>
            <xsl:apply-templates select="." mode="param-group-response">
              <xsl:with-param name="style">header</xsl:with-param>
            </xsl:apply-templates>
      </xsl:if>
    </xsl:template>

    <xsl:template match="wadl:*" mode="param-group">
        <xsl:param name="style"/>
        <xsl:param name="prefix"></xsl:param>
        <xsl:if test="ancestor-or-self::wadl:*/wadl:param[@style=$style]">
        <table>
            <tr>
                <th>Element</th>
                <th>Type</th>
                <th>Description</th>
           </tr>
            <xsl:apply-templates select="ancestor-or-self::wadl:*/wadl:param[@style=$style]"/>
          </table>
        </xsl:if>
    </xsl:template>

    <xsl:template match="wadl:*" mode="param-group-request">
        <xsl:param name="style"/>
        <xsl:param name="prefix"></xsl:param>
        <xsl:if test="ancestor-or-self::wadl:*/wadl:param[@style=$style]">
          <table>
            <tr>
              <th>Parameter</th>
              <th>Type</th>
              <th>Default</th>
              <th>Description</th>
            </tr>
            <xsl:apply-templates select="ancestor-or-self::wadl:*/wadl:param[@style=$style]" mode="request"/>
        </table>
      </xsl:if>
    </xsl:template>

    <xsl:template match="wadl:*" mode="param-group-response">
      <xsl:param name="style"/>
      <xsl:param name="prefix"></xsl:param>
      <xsl:if test="ancestor-or-self::wadl:*/wadl:param[@style=$style]">
        <table>
          <tr>
            <th>Element</th>
            <th>Type</th>
            <th>XPath</th>
            <th>Description</th>
          </tr>
          <xsl:apply-templates select="ancestor-or-self::wadl:*/wadl:param[@style=$style]" mode="response"/>
        </table>
      </xsl:if>
    </xsl:template>

  <xsl:template match="wadl:param" mode="request">
    <tr>
      <td>
        <p>
          <strong>
            <xsl:value-of select="@name"/>
          </strong>
        </p>
      </td>
      <td>
        <p>
          <xsl:call-template name="param-type"/>
          <xsl:if test="@required='true'">
            <small> (required)</small>
          </xsl:if>
          <xsl:if test="@repeating='true'">
            <small> (repeating)</small>
          </xsl:if>
        </p>
      </td>
      <td>
        <p>
          <xsl:if test="@default">
            <tt>
              <xsl:value-of select="@default"/>
            </tt>
          </xsl:if>
        </p>
      </td>
      <td>
        <xsl:apply-templates select="wadl:doc"/>
        <xsl:if test="wadl:option">
          <xsl:call-template name="param-option"/>
        </xsl:if>
      </td>
    </tr>
  </xsl:template>

  <xsl:template match="wadl:param">
    <tr>
      <td>
        <p>
          <strong>
            <xsl:value-of select="@name"/>
          </strong>
        </p>
      </td>
      <td>
        <p>
          <xsl:call-template name="param-type"/>
        </p>
      </td>
      <td>
        <xsl:apply-templates select="wadl:doc"/>
        <xsl:if test="wadl:option">
          <xsl:call-template name="param-option"/>
        </xsl:if>
      </td>
    </tr>
  </xsl:template>

  <xsl:template match="wadl:param" mode="response">
    <tr>
      <td>
        <p>
          <strong>
            <xsl:value-of select="@name"/>
          </strong>
        </p>
      </td>
      <td>
        <p>
          <xsl:call-template name="param-type"/>
        </p>
      </td>
      <td>
        <tt>
          <xsl:value-of select="@path"/>
        </tt>
      </td>
      <td>
        <xsl:apply-templates select="wadl:doc"/>
        <xsl:if test="wadl:option">
          <xsl:call-template name="param-option"/>
        </xsl:if>
      </td>
    </tr>
  </xsl:template>

    <xsl:template name="param-option">
      <xsl:choose>
        <xsl:when test="wadl:option[@type='enum']">
          Possible values:
        </xsl:when>
        <xsl:when test="wadl:option[@type='bitfield']">
          Bit fields:
        </xsl:when>
        <xsl:otherwise>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:choose>
        <xsl:when test="wadl:option[wadl:doc]">
          <dl>
            <xsl:apply-templates select="wadl:option" mode="option-doc"/>
          </dl>
        </xsl:when>
        <xsl:otherwise>
          <dl>
            <xsl:apply-templates select="wadl:option"/>
          </dl>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:template>
  
    <xsl:template name="param-type">
      <xsl:choose>
        <xsl:when test="@struct='true'">
          <!-- Complex Type -->
          <xsl:choose>
            <xsl:when test="contains(@type,'list:')">
              <xsl:variable name="anchor">
                <xsl:value-of select="substring-after(@type,'list:')"/>
              </xsl:variable>
              list:<a href="#{$anchor}"><xsl:value-of select="substring-after(@type,'list:')"/></a>
            </xsl:when>
            <xsl:when test="contains(@type,'map:')">
              <xsl:variable name="anchor">
                <xsl:value-of select="substring-after(@type,',')"/>
              </xsl:variable>
              <xsl:value-of select="substring-before(@type,',')"/>,<a href="#{$anchor}"><xsl:value-of select="substring-after(@type,',')"/></a>
            </xsl:when>
            <xsl:otherwise>
              <xsl:variable name="anchor">
                <xsl:value-of select="@type"/>
              </xsl:variable>
              <a href="#{$anchor}">
                <xsl:value-of select="@type"/>
              </a>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:otherwise>
          <xsl:choose>
            <xsl:when test="wadl:option">
              <i>
                <xsl:value-of select="@type"/> <!-- enum -->
              </i>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="@type"/> <!-- simple type -->
            </xsl:otherwise>
          </xsl:choose>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:template>
  
    <xsl:template match="wadl:link">
        <li>
            Link: <a href="#{@resource_type}"><xsl:value-of select="@rel"/></a>            
        </li>
    </xsl:template>

    <xsl:template match="wadl:option">
        <li>
            <tt><xsl:value-of select="@value"/></tt>
            <xsl:if test="ancestor::wadl:param[1]/@default=@value"> <small> (default)</small></xsl:if>
        </li>
    </xsl:template>

    <xsl:template match="wadl:option" mode="option-doc">
      <xsl:choose>
        <xsl:when test="@type='bitfield'">
          <dt>
            <tt>
              <xsl:if test="wadl:doc[@title]">
                <xsl:value-of select="wadl:doc[@title][1]/@title"/>
              </xsl:if>
            </tt>
          </dt>
            <dd>
              Number of bits:
              <xsl:value-of select="@value"/>
            </dd>
          </xsl:when>
          <xsl:otherwise>
            <dt>
              <tt>
                <xsl:if test="wadl:doc[@title]">
                  <xsl:value-of select="wadl:doc[@title][1]/@title"/>
                </xsl:if>
              </tt>
              <xsl:text> </xsl:text>
              <xsl:value-of select="@value"/>
              <xsl:if test="ancestor::wadl:param[1]/@default=@value">
                <small> (default)</small>
              </xsl:if>
            </dt>
          </xsl:otherwise>
        </xsl:choose>
        <dd>
          <xsl:apply-templates select="wadl:doc"/>
        </dd>
    </xsl:template>    

    <xsl:template match="wadl:doc">
        <xsl:param name="inline">0</xsl:param>
        <!-- skip WADL elements -->
        <xsl:choose>
            <xsl:when test="node()[1]=text() and $inline=0">
                <p>
                    <xsl:apply-templates select="node()" mode="copy"/>
                </p>
            </xsl:when>
            <xsl:otherwise>
                <xsl:apply-templates select="node()" mode="copy"/>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>
    
    <!-- utilities -->

    <xsl:template name="get-id">
        <xsl:choose>
            <xsl:when test="@id"><xsl:value-of select="@id"/></xsl:when>
            <xsl:otherwise><xsl:value-of select="generate-id()"/></xsl:otherwise>
        </xsl:choose>
    </xsl:template>

    <xsl:template name="get-namespace-uri">
        <xsl:param name="context" select="."/>
        <xsl:param name="qname"/>
        <xsl:variable name="prefix" select="substring-before($qname,':')"/>
        <xsl:variable name="qname-ns-uri" select="$context/namespace::*[name()=$prefix]"/>
        <!-- nasty hack to get around libxsl's refusal to copy all namespace nodes when pushing nodesets around -->
        <xsl:choose>
            <xsl:when test="$qname-ns-uri">
                <xsl:value-of select="$qname-ns-uri"/>                
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="exsl:node-set($resources)/*[1]/attribute::*[namespace-uri()='urn:namespace' and local-name()=$prefix]"/>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>
  
    <xsl:template name="get-element">
        <xsl:param name="context" select="."/>
        <xsl:param name="qname"/>
        <xsl:variable name="ns-uri">
            <xsl:call-template name="get-namespace-uri">
                <xsl:with-param name="context" select="$context"/>
                <xsl:with-param name="qname" select="$qname"/>
            </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="localname" select="substring-after($qname, ':')"/>
        <xsl:variable name="definition" select="exsl:node-set($grammars)/descendant::xs:element[@name=$localname][ancestor-or-self::*[@targetNamespace=$ns-uri]]"/>
        <xsl:variable name='source' select="$definition/ancestor-or-self::wadl:include[1]/@href"/>
        <p><em>Source: <a href="{$source}"><xsl:value-of select="$source"/></a></em></p>
        <pre><xsl:apply-templates select="$definition" mode="encode"/></pre>
    </xsl:template>

    <xsl:template name="link-qname">
        <xsl:param name="context" select="."/>
        <xsl:param name="qname"/>
        <xsl:variable name="ns-uri">
            <xsl:call-template name="get-namespace-uri">
                <xsl:with-param name="context" select="$context"/>
                <xsl:with-param name="qname" select="$qname"/>
            </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="localname" select="substring-after($qname, ':')"/>
        <xsl:choose>
            <xsl:when test="$ns-uri='http://www.w3.org/2001/XMLSchema'">
                <a href="http://www.w3.org/TR/xmlschema-2/#{$localname}"><xsl:value-of select="$localname"/></a>
            </xsl:when>
            <xsl:otherwise>
                <xsl:variable name="definition" select="exsl:node-set($grammars)/descendant::xs:*[@name=$localname][ancestor-or-self::*[@targetNamespace=$ns-uri]]"/>                
                <a href="{$definition/ancestor-or-self::wadl:include[1]/@href}" title="{$definition/descendant::xs:documentation/descendant::text()}"><xsl:value-of select="$localname"/></a>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>

    <xsl:template name="expand-qname">
        <xsl:param name="context" select="."/>
        <xsl:param name="qname"/>
        <xsl:variable name="ns-uri">
            <xsl:call-template name="get-namespace-uri">
                <xsl:with-param name="context" select="$context"/>
                <xsl:with-param name="qname" select="$qname"/>
            </xsl:call-template>
        </xsl:variable>
        <xsl:text>{</xsl:text>
        <xsl:value-of select="$ns-uri"/>
        <xsl:text>} </xsl:text> 
        <xsl:value-of select="substring-after($qname, ':')"/>
    </xsl:template>

    <xsl:template name="representation-name-simple">
      <xsl:value-of select="wadl:doc[@title][1]/@title"/>
    </xsl:template>
    
    <xsl:template name="representation-name">
        <xsl:variable name="expanded-name">
            <xsl:call-template name="expand-qname">
                <xsl:with-param select="@element" name="qname"/>
            </xsl:call-template>
        </xsl:variable>
        <xsl:choose>
            <xsl:when test="wadl:doc[@title]">
                <xsl:value-of select="wadl:doc[@title][1]/@title"/>
                <xsl:if test="@status or @mediaType or @element"> (</xsl:if>
                <xsl:if test="@status">Status Code </xsl:if><xsl:value-of select="@status"/>
                <xsl:if test="@status and @mediaType"> - </xsl:if>
                <xsl:value-of select="@mediaType"/>
                <xsl:if test="(@status or @mediaType) and @element"> - </xsl:if>
                <xsl:if test="@element">
                    <abbr title="{$expanded-name}"><xsl:value-of select="@element"/></abbr>
                </xsl:if>
                <xsl:if test="@status or @mediaType or @element">)</xsl:if>
            </xsl:when>
            <xsl:otherwise>
                <xsl:if test="@status">Status Code </xsl:if><xsl:value-of select="@status"/>
                <xsl:if test="@status and @mediaType"> - </xsl:if>
                <xsl:value-of select="@mediaType"/>
                <xsl:if test="@element"> (</xsl:if>
                <abbr title="{$expanded-name}"><xsl:value-of select="@element"/></abbr>
                <xsl:if test="@element">)</xsl:if>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>


  <!-- entity-encode markup for display -->

    <xsl:template match="*" mode="encode">
        <xsl:text>&lt;</xsl:text>
        <xsl:value-of select="name()"/><xsl:apply-templates select="attribute::*" mode="encode"/>
        <xsl:choose>
            <xsl:when test="*|text()">
                <xsl:text>&gt;</xsl:text>
                <xsl:apply-templates select="*|text()" mode="encode" xml:space="preserve"/>
                <xsl:text>&lt;/</xsl:text><xsl:value-of select="name()"/><xsl:text>&gt;</xsl:text>                
            </xsl:when>
            <xsl:otherwise>
                <xsl:text>/&gt;</xsl:text>
            </xsl:otherwise>
        </xsl:choose>    
    </xsl:template>            
    
    <xsl:template match="@*" mode="encode">
        <xsl:text> </xsl:text><xsl:value-of select="name()"/><xsl:text>="</xsl:text><xsl:value-of select="."/><xsl:text>"</xsl:text>
    </xsl:template>    
    
    <xsl:template match="text()" mode="encode">
        <xsl:value-of select="." xml:space="preserve"/>
    </xsl:template>    

    <!-- copy HTML for display -->
    
    <xsl:template match="html:*" mode="copy">
        <!-- remove the prefix on HTML elements -->
        <xsl:element name="{local-name()}">
            <xsl:for-each select="@*">
                <xsl:attribute name="{local-name()}"><xsl:value-of select="."/></xsl:attribute>
            </xsl:for-each>
            <xsl:apply-templates select="node()" mode="copy"/>
        </xsl:element>
    </xsl:template>
    
    <xsl:template match="@*|node()[namespace-uri()!='http://www.w3.org/1999/xhtml']" mode="copy">
        <!-- everything else goes straight through -->
        <xsl:copy>
            <xsl:apply-templates select="@*|node()" mode="copy"/>
        </xsl:copy>
    </xsl:template>
  
</xsl:stylesheet>
