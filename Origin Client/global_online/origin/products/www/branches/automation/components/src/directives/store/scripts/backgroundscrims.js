/**
 * @file store/scripts/backgroundscrims.js
 */
(function(){
    'use strict';

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    function originStoreBackgroundscrims($compile, CSSUtilFactory) {

        function originStoreBackgroundscrimsLink(scope, element) {
            var leftScrim = angular.element('<div class="origin-backgroundimage-scrim origin-telemetry-backgroundimage-scrim origin-backgroundimage-scrim-left" origin-style="gradientLeft"></div>'),
                rightScrim = angular.element('<div class="origin-backgroundimage-scrim origin-telemetry-backgroundimage-scrim origin-backgroundimage-scrim-right" origin-style="gradientRight"></div>'),
                bottomScrim = angular.element('<div class="origin-backgroundimage-scrim origin-telemetry-backgroundimage-scrim origin-backgroundimage-scrim-bottom" origin-style="gradientBottom"></div>'),
                hasBottomScrim = scope.bottomScrim && scope.bottomScrim === 'true';

            function setScrimGradients(backgroundColor) {
                var rgbaFull = CSSUtilFactory.hexToRgba(backgroundColor, 1),
                    rgbaTransparent = CSSUtilFactory.hexToRgba(backgroundColor, 0);
                scope.gradientLeft = CSSUtilFactory.createLinearGradient(rgbaFull, 0, rgbaTransparent, 100);
                scope.gradientRight = CSSUtilFactory.createLinearGradient(rgbaTransparent, 0, rgbaFull, 100);
                if (hasBottomScrim){
                    scope.gradientBottom = CSSUtilFactory.createLinearGradient(rgbaTransparent, 0, rgbaFull, 100, 'top to bottom');
                }
            }

            element.append($compile(leftScrim)(scope));
            element.append($compile(rightScrim)(scope));
            if (hasBottomScrim){
                element.append($compile(bottomScrim)(scope));
            }

            setScrimGradients(scope.backgroundColor);

            var stopWatching = scope.$watch('backgroundColor', function (newBackgroundColor) {
                if (newBackgroundColor) {
                    stopWatching();
                    setScrimGradients(newBackgroundColor);
                }
            });
        }

        return {
            restrict: 'A',
            link: originStoreBackgroundscrimsLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreBackgroundscrims
     * @restrict A
     * @element ANY
     *
     * @param {string} background-color Hex value of the default background color for all image scrims.
     * @param {BooleanEnumeration} bottom-scrim should this background image have a bottom scrim
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-backgroundimage origin-store-backgroundscrims background-color='{{ backgroundColor }}' image='{{ ::imageSrc }}'> </origin-store-backgroundimage>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreBackgroundscrims', originStoreBackgroundscrims);
}());
