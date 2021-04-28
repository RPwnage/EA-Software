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
            var currentText = attrs.originTooltip;
            if (_.isEmpty(currentText)) {
                return;
            }
            
            var positionClass = attrs.originTooltipPlacement ? 'otktooltip-' + attrs.originTooltipPlacement : 'otktooltip-top';
            var elstr = '<div class="otktooltip otktooltip-isvisible ' + positionClass + '"><b class="otktooltip-arrow"></b><div class="otktooltip-inner">' + currentText + '</div></div>';
            if ((positionClass === 'otktooltip-top') || (positionClass === 'otktooltip-left')) {
                element.prepend(elstr);
            }
            else {
                element.append(elstr);
            }
            element.addClass('origin-tooltip');
            
            function updateWordWrapTag(newValue) {
                var breakcount = 32;
                if ((Origin.locale.locale().toLowerCase() === 'ko_kr') ||
                    (Origin.locale.locale().toLowerCase() === 'zh_tw') ||
                    (Origin.locale.locale().toLowerCase() === 'ja_jp')) {
                    breakcount = 12;
                }
                var $tooltipInner = $(element).find('.otktooltip-inner');
                $tooltipInner.text(newValue);
                if (newValue.length > breakcount) {
                    $tooltipInner.addClass('tooltip-wordwrap');
                } else {
                    $tooltipInner.removeClass('tooltip-wordwrap');
                }
            }

            element.on('tooltip:update', function(ignore, values) {
                if (currentText === values.old) {
                    updateWordWrapTag(values.new);
                    currentText = values.new;
                }
            });

            // set the initial word wrapping
            updateWordWrapTag(currentText);
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
