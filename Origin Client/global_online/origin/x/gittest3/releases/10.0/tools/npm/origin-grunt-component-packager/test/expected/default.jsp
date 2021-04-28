<%@include file="/libs/foundation/global.jsp"%>
<div style="border: 1px solid #666; background: #FFF; padding: 5px;">
    <h3 class="otktitle-3">origin-home-recommended-action-trial</h3>
    <pre>
startdate: <%=properties.get("startdate")%>
starttime: <%=properties.get("starttime")%>
expireddatetime: <%=properties.get("expireddatetime")%>
Image(s): Double Click to View Images
title: <%=properties.get("i18n/title")%>
subtitle: <%=properties.get("i18n/subtitle")%>
description: <%=properties.get("i18n/description")%>
titleexpired: <%=properties.get("i18n/titleexpired")%>
subtitleexpired: <%=properties.get("i18n/subtitleexpired")%>
descriptionexpired: <%=properties.get("i18n/descriptionexpired")%>
href: <%=properties.get("href")%>
baz: <%=properties.get("baz")%>
priority: <%=properties.get("priority")%>
trialtype: <%=properties.get("trialtype")%>

    </pre>
    <div style="margin: 20px">
        <cq:include path="items" resourceType="foundation/components/parsys"/>
    </div>
</div>