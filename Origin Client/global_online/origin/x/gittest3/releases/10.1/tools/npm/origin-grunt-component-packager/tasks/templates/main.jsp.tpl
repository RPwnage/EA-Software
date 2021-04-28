<%@ page import="
    org.apache.sling.api.resource.*
"%>
<%@include file="/libs/foundation/global.jsp"%>
<%@include file="/apps/originx/components/management/imagetab/imagehandler.jsp"%>
<%
String directiveName = "%directive-name%";
currentNode.setProperty("directiveName", directiveName);
String[] directiveAttributes = {%directive-properties%};
currentNode.setProperty("directiveAttributes", directiveAttributes);
%>
%directive-images%
<%
currentNode.save(); 
Resource res = null;
String overridePath = "%overridePath%";
String defaultPath = "%defaultPath%";
String path = "";

try {
    res = resourceResolver.resolve(overridePath);
} catch (Exception e) {
    path = defaultPath;
} 
if (res == null || ResourceUtil.isNonExistingResource(res)) { 
    path = defaultPath;
} else {
    path = overridePath;
}
%>
<cq:include script="<%=path%>" />