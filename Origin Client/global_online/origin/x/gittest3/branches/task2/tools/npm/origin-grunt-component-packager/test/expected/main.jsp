<%@ page import="
    org.apache.sling.api.resource.*
"%>
<%@include file="/libs/foundation/global.jsp"%>
<%@include file="/apps/originx/components/management/imagetab/imagehandler.jsp"%>
<%
String directiveName = "origin-home-recommended-action-trial";
currentNode.setProperty("directiveName", directiveName);
String[] directiveAttributes = {"startdate","starttime","expireddatetime","image","i18n/title","i18n/subtitle","i18n/description","i18n/titleexpired","i18n/subtitleexpired","i18n/descriptionexpired","href","baz","priority","trialtype"};
currentNode.setProperty("directiveAttributes", directiveAttributes);
%>
<%
String[] imageTabs = {"image"};
currentNode.setProperty("imageTabs", imageTabs);
%>
<%
currentNode.save(); 
Resource res = null;
String overridePath = "/apps/originx/components/imported/directives/scripts/origin-home-recommended-action-trial/origin-home-recommended-action-trial.override.jsp";
String defaultPath = "/apps/originx/components/imported/directives/scripts/origin-home-recommended-action-trial/origin-home-recommended-action-trial.default.jsp";
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