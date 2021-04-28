<%@ page contentType="text/html" %>
<%@ page trimDirectiveWhitespaces="true" %>
<%@include file="/libs/foundation/global.jsp"%>
<%@include file="/apps/foundation/directive/global.jsp"%>
<%@include file="/apps/foundation/util/localizer.jsp"%>
<%@include file="/apps/foundation/util/targeting.jsp"%>
<%@include file="/apps/originx/components/management/imagetab/imagehandler.jsp"%>
<%
I18n i18n = loadTranslations(slingRequest, "originx.app.translations");
String[] attrArray = {"i18n/descriptionexpired"};
List<String> attributes = getAttributes(properties, attrArray, i18n);
%>

<% if (isGeoTargeted(slingRequest, currentNode, log)) { %>
<origin-ocd-ocd <% for (String s : attributes) { %><%=s%><% } %>><%@include file="/apps/foundation/directive/componentChildrenHandler.jsp"%></origin-ocd-ocd>
<% } %>