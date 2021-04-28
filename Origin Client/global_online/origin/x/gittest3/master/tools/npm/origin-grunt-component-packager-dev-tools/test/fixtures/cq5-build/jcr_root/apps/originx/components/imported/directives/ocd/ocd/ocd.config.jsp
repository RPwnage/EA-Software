<%@ page contentType="text/html" %>
<%@ page trimDirectiveWhitespaces="true" %>
<%@include file="/libs/foundation/global.jsp"%>
<%@include file="/apps/foundation/directive/global.jsp"%>
<%@include file="/apps/foundation/util/localizer.jsp"%>
<%
I18n i18n = loadTranslations(slingRequest, "originx.app.translations");
String[] attrArray = {"i18n/descriptionexpired"};
List<String> attributes = getJsonAttributes(properties, attrArray, i18n);
String[] attributesArray = attributes.toArray(new String[attributes.size()]);
String attrAsString = org.apache.commons.lang3.StringUtils.join(attributesArray, ",");
%>
{"origin-ocd-ocd":{ <%=attrAsString%> <%@include file="/apps/foundation/json/componentChildrenHandler.jsp"%> }}