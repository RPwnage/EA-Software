<%@include file="/libs/foundation/global.jsp"%>
<div style="border: 1px solid #666; background: #FFF; padding: 5px;">
    <h3 class="otktitle-3">origin-home-recommended-action-trial</h3>
    <pre>
expireddatetime: Double Click to View The Timestamp
image: Double Click to View Image
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
youtubevideoid: Double Click to View The Video
offerid: Double Click to View The Offer

    </pre>
    <div style="margin: 20px">
        <cq:include path="items" resourceType="foundation/components/parsys"/>
    </div>
</div>