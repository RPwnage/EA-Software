<%@ page import="
    org.apache.sling.api.resource.*
"%>
<%@include file="/libs/foundation/global.jsp"%>
<% 
Resource res = null;
String overridePath = "/apps/originx/components/imported/directives/ocd/ocd/ocd.override.jsp";
String defaultPath = "/apps/originx/components/imported/directives/ocd/ocd/ocd.default.jsp";
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