<%@include file="/libs/foundation/global.jsp"%>
<div style="border: 1px solid #666; background: #FFF; padding: 5px;">
    <h3 class="otktitle-3">%directive-name%</h3>
    <pre>
%directive-display-properties%
    </pre>
    <div style="margin: 20px">
        <cq:include path="items" resourceType="foundation/components/parsys"/>
    </div>
</div>