/**
 * @file tooltip.js
 */

(function() {
    'use strict';

    function OriginTooltipCtrl() {
    }

    function originTooltip() {

        function originTooltipLink(scope, element, attrs, ctrl) {
            ctrl = ctrl;

            // Build and add the tooltip markup to the element
            var str = attrs.originTooltip;
            var positionClass = attrs.originTooltipPlacement ? 'otktooltip-' + attrs.originTooltipPlacement : 'otktooltip-top';
            var elstr = '<div class="otktooltip otktooltip-isvisible ' + positionClass + '"><b class="otktooltip-arrow"></b><div class="otktooltip-inner">' + str + '</div></div>';
            if ((positionClass === 'otktooltip-top') || (positionClass === 'otktooltip-left')) {
                element.prepend(elstr);
            }
            else {
                element.append(elstr);
            }
            element.addClass('origin-tooltip');

            // Handle word wrapping
            if (str.length > 32) {
                var $tooltipInner = $(element).find('.otktooltip-inner');
                $tooltipInner.addClass('tooltip-wordwrap');
            }
        }

        return {
            restrict: 'A',
            controller: 'OriginTooltipCtrl',
            link: originTooltipLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originTooltip
     * @restrict A
     * @element ANY
     * @scope
     * @description
     *
     * Attribute directive controller for otktooltips
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *        <div origin-tooltip="Tooltip text" origin-tooltip-placement="top">
     *            <div>Hello World</div> <!-- element you want your tooltip on -->
     *        </div>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginTooltipCtrl', OriginTooltipCtrl)
        .directive('originTooltip', originTooltip);
}());
