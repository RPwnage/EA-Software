<%@include file="/libs/foundation/global.jsp"%>

<div style="border: 1px solid #666; background: #FFF; padding: 5px;">
    <h3 class="otktitle-3">origin-ocd-ocd</h3>
    <pre>
descriptionexpired: <%=properties.get("i18n/descriptionexpired")%>

    </pre>
    <div style="margin: 20px">
        <cq:include path="items" resourceType="foundation/components/parsys"/>
    </div>
</div>