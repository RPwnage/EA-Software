
/**
 * @file /scripts/lineclamp.js
 */
(function(){
    'use strict';

    var DESCENDER_MARGIN = 2; // extra space to allow for descenders in font (letters such as g/y etc.)

    function originLineclamp(CSSUtilFactory, FeatureDetectionFactory) {

        function originLineclampLink(scope, element) {
            var lineHeight = parseInt(element.css('line-height'), 10),
                isCSSLineclampSupported = FeatureDetectionFactory.browserSupports('line-clamp');

            element.addClass('origin-lineclamp');

            /**
             * Adds CSS rules necessary for line-clamping, including webkit-specific if supported.
             * Will only set if dependent values are available
             * @param {Number} Number of lines to limit text content to
             */
            function setStyles(numberOfLines) {
                if (numberOfLines && lineHeight) {
                    element.css('height', numberOfLines * lineHeight + DESCENDER_MARGIN);

                    // only set -webkit-line-clamp on browsers which support it
                    if (isCSSLineclampSupported) {
                        element.css(CSSUtilFactory.addVendorPrefixes('line-clamp', numberOfLines + '')); //coercing number to string to avoid jQuery "px" append
                    }
                }
            }

            /**
             * Reset styling to remove line-clamp rules
             */
            function resetStyles() {
                element.css('height', 'auto');
                if (isCSSLineclampSupported) {
                    element.css(CSSUtilFactory.addVendorPrefixes('line-clamp', ''));
                }
            }

            scope.$watch('originLineclamp', function(newVal, oldVal) {
                if (newVal !== oldVal) {
                    if (newVal) {
                        setStyles(newVal);
                    } else {
                        resetStyles();
                    }
                }
            });
        }

        return {
            restrict: 'A',
            scope: {
                originLineclamp: '='
            },
            link: originLineclampLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originLineclamp
     * @restrict A
     * @element ANY
     * @scope
     * @description Limits text content inside to specified number of lines.
     *              Auto-calculates based on line-height, which must be a px value.
     *              If value is zero or null, line-clamping will be disabled.
     *
     * @param {Number} number-of-lines how many lines to limit the content to
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <div origin-lineclamp="2">Lorem ipsum dolor sit amet, consectetur adipiscing elit.</div>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originLineclamp', originLineclamp);
}());
